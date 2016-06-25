/*
Copyright (c) 2016 InternetWide.org and the ARPA2.net project
All rights reserved. See file LICENSE for exact terms (2-clause BSD license).

Adriaan de Groot <groot@kde.org>
*/

#include "parserpp.h"

#include "parser.h"
#include "condition.h"
#include "driver.h"
#include "variable.h"

#include <logger.h>

class SteamWorks::PulleyScript::Parser::Private
{
private:
	struct parser m_prs;
	bool m_valid;

public:
	State m_state;

	Private() : m_valid(false), m_state(Parser::State::Initial)
	{
		if (pulley_parser_init(&m_prs))
		{
			m_valid = true;
		}
	}
	~Private()
	{
		if (is_valid())
		{
			pulley_parser_cleanup_syntax(&m_prs);
			pulley_parser_cleanup_semantics(&m_prs);
		}
		m_valid = false;
		m_state = Parser::State::Initial;  // Not really
	}

	bool is_valid() const { return m_valid; }

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
	return d->m_state;
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
