/*
Copyright (c) 2016 InternetWide.org and the ARPA2.net project
All rights reserved. See file LICENSE for exact terms (2-clause BSD license).

Adriaan de Groot <groot@kde.org>
*/

#include "parserpp.h"
#include "bindingpp.h"

#include "condition.h"
#include "driver.h"
#include "generator.h"
#include "parser.h"
#include "squeal.h"
#include "variable.h"

#include <logger.h>
#include <jsoniterator.h>

class SquealOpener
{
public:
	struct squeal* m_sql;

	SquealOpener() : m_sql(nullptr) {}
	SquealOpener(struct parser* prs) : m_sql(nullptr) { open(prs); }
	~SquealOpener()
	{
		if (m_sql)
		{
			squeal_close(m_sql);
		}
	}

	bool open(struct parser* prs)
	{
		auto& log = SteamWorks::Logging::getLogger("steamworks.pulleyscript");
		auto stream = log.debugStream();
		hash_t h = prs->scanhash;
		stream << "Opening SQL for ";
		SteamWorks::Logging::log_hex(stream, (uint8_t *)&h, sizeof(h));

		m_sql = squeal_open(prs->scanhash, gentab_count (prs->gentab), drvtab_count (prs->drvtab));
		return m_sql != nullptr;
	}

	void close()
	{
		if (m_sql)
		{
			auto& log = SteamWorks::Logging::getLogger("steamworks.pulleyscript");
			log.debugStream() << "Closing SQL.";

			squeal_close(m_sql);
			// Don't unlink, leave DB for debugging purposes.
			m_sql = nullptr;
		}
	}
} ;


class SteamWorks::PulleyScript::Parser::Private
{
private:
	struct parser m_prs;
	SquealOpener m_sql;

	using generator_variablenames_t = std::vector<std::string>;
	std::unique_ptr<std::vector<generator_variablenames_t> > m_variables_per_generator;

	bool m_valid;
	State m_state;

	// Helper in find_subscriptions()
	std::vector<varnum_t> variables_for_generator(gennum_t g);


public:
	Private() : m_valid(false), m_state(Parser::State::Initial)
	{
		if (pulley_parser_init(&m_prs))
		{
			m_valid = true;
		}
	}
	~Private()
	{
		m_sql.close();

		if (is_valid())
		{
			pulley_parser_cleanup_syntax(&m_prs);
			pulley_parser_cleanup_semantics(&m_prs);
		}
		m_valid = false;
		m_state = Parser::State::Initial;  // Not really
	}

	const struct parser* parser() const { return &m_prs; }

	bool is_valid() const { return m_valid; }

	Parser::State state() const { return m_state; }

	std::string state_string() const
	{
		const char *s = nullptr;

		switch (m_state)
		{
		case State::Initial:
			s = "Initial";
			break;
		case State::Parsing:
			s = "Parsing";
			break;
		case State::Analyzed:
			s = "Analyzed";
			break;
		case State::Ready:
			s = "Ready";
			break;
		case State::Broken:
			s = "Broken";
			break;
		}

		if (!s)
		{
			s = "Unknown";
		}

		return std::string(s);
	}

	bool can_parse() const
	{
		if ((m_state == Parser::State::Initial) || (m_state == Parser::State::Parsing))
		{
			return true;
		}
		else
		{
			auto& log = SteamWorks::Logging::getLogger("steamworks.pulleyscript");
			log.errorStream() << "Parser cannot parse Script from state " << state_string();
			return false;
		}
	}

	bool can_analyze() const
	{
		if (m_state == Parser::State::Parsing)
		{
			return true;
		}
		else
		{
			auto& log = SteamWorks::Logging::getLogger("steamworks.pulleyscript");
			log.errorStream() << "Parser cannot do analysis from state " << state_string();
			return false;
		}
	}

	bool can_generate_sql() const
	{
		if (m_state == Parser::State::Analyzed)
		{
			return true;
		}
		else
		{
			auto& log = SteamWorks::Logging::getLogger("steamworks.pulleyscript");
			log.errorStream() << "Parser cannot generate SQL from state " << state_string();
			return false;
		}
	}

	int read_file(FILE* input)
	{
		if (!can_parse())
		{
			return 1;
		}

		int prsret = pulley_parser_file (&m_prs, input);
		if (prsret)
		{
			auto& log = SteamWorks::Logging::getLogger("steamworks.pulleyscript");
			log.errorStream() << "Could not read file input (" << prsret << ')';
			return prsret;
		}

		m_state = Parser::State::Parsing;
		return 0;
	}

	int structural_analysis()
	{
		if (!can_analyze())
		{
			return 1;
		}

		pulley_parser_hash(&m_prs);

		int prsret = 0;

		// Use conditions to drive variable partitions;
		// collect shared_varpartitions for them.
		// This enables
		//  - var_vars2partitions()
		//  - var_partitions2vars()
		cndtab_drive_partitions (m_prs.cndtab);
		vartab_collect_varpartitions (m_prs.vartab);

		// Collect the things a driver needs for its work
		drvtab_collect_varpartitions (m_prs.drvtab);
		drvtab_collect_conditions (m_prs.drvtab);
		drvtab_collect_generators (m_prs.drvtab);
		drvtab_collect_cogenerators (m_prs.drvtab);
		drvtab_collect_genvariables (m_prs.drvtab);
		drvtab_collect_guards (m_prs.drvtab);

		// Collect the things a generator needs for its work.
		// This enables
		//  - gen_share_driverouts()	XXX already done
		//  - gen_share_conditions()	XXX never used (yet)
		//  - gen_share_generators()	XXX never used (yet)
		//NONEED// gen_push_driverout() => gen_share_driverouts()
		//NONEED// gen_push_condition() => gen_share_conditions()
		//NONEED// gen_push_generator() => gen_share_generators()

		m_state = State::Analyzed;
		return prsret;
	}

	int setup_sql()
	{
		if (!can_generate_sql())
		{
			return 1;
		}
		if (!m_sql.open(&m_prs))
		{
			m_state = State::Broken;
			return 1;
		}

		auto& log = SteamWorks::Logging::getLogger("steamworks.pulleyscript");

		if (squeal_have_tables(m_sql.m_sql, m_prs.gentab, 0) != 0)
		{
			log.errorStream() << "Could not create SQL tables for script.";
			m_state = State::Broken;
			return 1;
		}

		if (squeal_configure(m_sql.m_sql) != 0)
		{
			log.errorStream() << "Could not prepare SQL statements for drv_all.";
			m_state = State::Broken;
			return 1;
		}

		if (squeal_configure_generators(m_sql.m_sql, m_prs.gentab, m_prs.drvtab) != 0)
		{
			log.errorStream() << "Could not configure generator SQL statements.";
			m_state = State::Broken;
			return 1;
		}

		log.debugStream() << "SQL table definitions generated.";

		// This bit copied out of the compiler;
		gennum_t g, gentabcnt = gentab_count (m_prs.gentab);
		bitset_t *drv;
		bitset_iter_t di;
		drvnum_t d;
		for (g=0; g<gentabcnt; g++) {
			drv = gen_share_driverout (m_prs.gentab, g);
			log.debugStream() << "Found " << bitset_count(drv) << " drivers for generator " << g;
			bitset_iterator_init (&di, drv);
			while (bitset_iterator_next_one (&di, NULL)) {
				d = bitset_iterator_bitnum (&di);
				log.debugStream() << "Generating for generator " << g << ", driver " << d;
			}
		}

		// m_sql.close();
		m_state = State::Ready;
		return 0;
	}

	// Extract filter-expressions
	std::forward_list< std::string > find_subscriptions();

	// Remove an entry from the middle-end (post-SQL)
	void remove_entry(const std::string& uuid);
	void add_entry(const std::string& uuid, const picojson::object& data);

	const generator_variablenames_t& variable_names(gennum_t generator)
	{
		return m_variables_per_generator->at(generator);
	}
} ;

SteamWorks::PulleyScript::Parser::Parser() :
	d(new Private)
{
}

SteamWorks::PulleyScript::Parser::~Parser()
{
}

SteamWorks::PulleyScript::Parser::State SteamWorks::PulleyScript::Parser::state() const
{
	return d->state();
}

std::string SteamWorks::PulleyScript::Parser::state_string() const
{
	return d->state_string();
}

int SteamWorks::PulleyScript::Parser::read_file(const char* filename)
{
	if (!d->can_parse())
	{
		return 1;
	}

	auto& log = SteamWorks::Logging::getLogger("steamworks.pulleyscript");
	FILE *fh = fopen(filename, "r");
	if (!fh) {
		log.errorStream() << "Failed to open " << filename;
		return 1;
	}
	log.debugStream() << "Loading " << filename;

	int r = read_file(fh);
	fclose(fh);
	return r;
}

int SteamWorks::PulleyScript::Parser::read_file(FILE* input)
{
	return d->read_file(input);
}

int SteamWorks::PulleyScript::Parser::structural_analysis()
{
	if (state() != State::Parsing)
	{
		auto& log = SteamWorks::Logging::getLogger("steamworks.pulleyscript");
		log.errorStream() << "Can only analyze once, after parsing something.";
		return 1;
	}

	return d->structural_analysis();
}

int SteamWorks::PulleyScript::Parser::setup_sql()
{
	return d->setup_sql();
}

std::forward_list< std::string > SteamWorks::PulleyScript::Parser::find_subscriptions()
{
	return d->find_subscriptions();
}

std::vector<varnum_t> SteamWorks::PulleyScript::Parser::Private::variables_for_generator(gennum_t g)
{
	bitset *b = gen_share_variables(m_prs.gentab, g);
	bitset_iter it;

	std::vector<varnum_t> names;
	names.reserve(bitset_count(b));

	unsigned int varcount = 0;

	bitset_iterator_init(&it, b);
	while (bitset_iterator_next_one (&it, NULL))
	{
		varnum_t v = bitset_iterator_bitnum (&it);
		if (var_get_kind(m_prs.vartab, v) != VARKIND_VARIABLE)
		{
			continue;
		}
		names.push_back(v);
		varcount++;
	}

	return names;
}

std::forward_list< std::string > SteamWorks::PulleyScript::Parser::Private::find_subscriptions()
{
	auto& log = SteamWorks::Logging::getLogger("steamworks.pulleyscript");
	log.debugStream() << "Finding parser subscriptions:";

	varnum_t world = var_find(m_prs.vartab, "world", VARKIND_VARIABLE);
	if (world == VARNUM_BAD)
	{
		log.warnStream() << "Script does not pull from world.";
		return std::forward_list<std::string>();
	}

	m_variables_per_generator.reset(new std::vector<generator_variablenames_t>);

	std::forward_list<std::string> filterexps;
	gennum_t count = gentab_count(m_prs.gentab);
	for (gennum_t i=0; i<count; i++)
	{
		varnum_t v = gen_get_source(m_prs.gentab, i);
		std::string* filterexp = nullptr;

		if (v != world)
		{
			// Only look at expressions pulling from world
			;
		}
		else
		{
			filterexps.emplace_front();
			filterexp = &filterexps.front();
		}

		varnum_t b = gen_get_binding(m_prs.gentab, i);
		struct var_value* value = var_share_value(m_prs.vartab, b);

		auto bound_varnums = variables_for_generator(i);  // Variables on the right-hand side of binding
		m_variables_per_generator->emplace_back(bound_varnums.size());  // New vector of names

		explain_binding(m_prs.vartab, value->typed_blob.str, value->typed_blob.len, filterexp, bound_varnums, m_variables_per_generator->back());
	}

	for (gennum_t g=0; g<count; g++) {
		log.debugStream() << "Variable names for generator " << g;
		for (const auto& f : m_variables_per_generator->at(g))
		{
			log.debugStream() << "    " << f;
		}
	}

	return filterexps;
}

void SteamWorks::PulleyScript::Parser::remove_entry(const std::string& uuid)
{
	if (state() != State::Ready)
	{
		auto& log = SteamWorks::Logging::getLogger("steamworks.pulleyscript");
		log.errorStream() << "Pulley setup was incomplete or failed (" << d->state_string() << "). " << "Cannot remove entry.";
		return;
	}

	d->remove_entry(uuid);
}

void SteamWorks::PulleyScript::Parser::add_entry(const std::string& uuid, const picojson::object& data)
{
	if (state() != State::Ready)
	{
		auto& log = SteamWorks::Logging::getLogger("steamworks.pulleyscript");
		log.errorStream() << "Pulley setup was incomplete or failed (" << d->state_string() << "). " << "Cannot add entry.";
		return;
	}

	d->add_entry(uuid, data);
}

void SteamWorks::PulleyScript::Parser::Private::remove_entry(const std::string& uuid)
{
	auto& log = SteamWorks::Logging::getLogger("steamworks.pulleyscript");
	log.debugStream() << "Removing entry:" << uuid;

	gennum_t count = gentab_count(m_prs.gentab);
	for (gennum_t i=0; i<count; i++)
	{
		hash_t h = gen_get_hash(m_prs.gentab, i);

		auto stream = log.debugStream();
		stream << "  .. generator " << i << " hash ";
		SteamWorks::Logging::log_hex(stream, (uint8_t *)&h, sizeof(h));

		squeal_delete_forks(m_sql.m_sql, i, uuid.c_str());
	}
}

void SteamWorks::PulleyScript::Parser::Private::add_entry(const std::string& uuid, const picojson::object& data)
{
	auto& log = SteamWorks::Logging::getLogger("steamworks.pulleyscript");
   	log.debugStream() << "Adding entry:" << uuid;

	gennum_t count = gentab_count(m_prs.gentab);
	for (gennum_t i=0; i<count; i++)
	{
		hash_t h = gen_get_hash(m_prs.gentab, i);

		auto stream = log.debugStream();
		stream << "  .. generator " << i << " hash ";
		SteamWorks::Logging::log_hex(stream, (uint8_t *)&h, sizeof(h));

		for (const auto& f : variable_names(i))
		{
			log.debugStream() << "  .. generate with " << f;
		}

		struct squeal_blob blobs[variable_names(i).size()];
		MultiIterator it(data, variable_names(i));

		while (!it.is_done())
		{
			MultiIterator::value_t v = it.next();
			unsigned int blobnum = 0;
			for (const auto& f : v)
			{
				blobs[blobnum].data = (void *)f.c_str();
				blobs[blobnum].size = f.length();
				blobnum++;
			}

			squeal_insert_fork(m_sql.m_sql, i, uuid.c_str(), variable_names(i).size(), blobs);
		}
	}
}
