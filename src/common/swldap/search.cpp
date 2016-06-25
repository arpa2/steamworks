/*
Copyright (c) 2014, 2015 InternetWide.org and the ARPA2.net project
All rights reserved. See file LICENSE for exact terms (2-clause BSD license).

Adriaan de Groot <groot@kde.org>
*/

#include "search.h"
#include "private.h"

#include "picojson.h"

/**
 * Internals of a search. A search holds a base dn for the search
 * and a filter expression. When executed, it returns an array
 * of objects found.
 */
class Steamworks::LDAP::Search::Private
{
private:
	std::string m_base, m_filter;
	LDAPScope m_scope;

public:
	Private(const std::string& base, const std::string& filter, LDAPScope scope) :
		m_base(base),
		m_filter(filter),
		m_scope(scope)
	{
	}

	const std::string& base() const { return m_base; }
	const std::string& filter() const { return m_filter; }
	LDAPScope scope() const { return m_scope; }
} ;

Steamworks::LDAP::Search::Search(const std::string& base, const std::string& filter, LDAPScope scope) :
	Action(true),
	d(new Private(base, filter, scope))
{
}

Steamworks::LDAP::Search::~Search()
{
}

void Steamworks::LDAP::Search::execute(Connection& conn, Result results)
{
	::LDAP* ldaphandle = handle(conn);

	Steamworks::Logging::Logger& log = Steamworks::Logging::getLogger("steamworks.ldap");

	// TODO: settings for timeouts?
	struct timeval tv;
	tv.tv_sec = 2;
	tv.tv_usec = 0;

	LDAPMessage* res;
	int r = ldap_search_ext_s(ldaphandle,
		d->base().c_str(),
		d->scope(),
		d->filter().c_str(),
		nullptr,  // attrs
		0,
		server_controls(conn),
		client_controls(conn),
		&tv,
		1024*1024,
		&res);
	if (r)
	{
		log.errorStream() << "Search result " << r << " " << ldap_err2string(r);
		ldap_msgfree(res);  // Should be freed regardless of the return value
		return;
	}

	copy_search_result(ldaphandle, res, results, log);

	ldap_msgfree(res);
}




/**
 * Internals of an update.
 */
class Steamworks::LDAP::Update::Private
{
private:
	std::string m_dn;
	Steamworks::LDAP::Update::Attributes m_map;

public:
	Private(const std::string& dn) :
		m_dn(dn)
	{
	};
	void update(const Steamworks::LDAP::Update::Attributes& attrs)
	{
		m_map.insert(attrs.cbegin(), attrs.cend());
	}
	void update(const std::string& name, const std::string& value)
	{
		m_map.emplace<>(name, value);
	}
	void remove(const std::string& name)
	{
		m_map.erase(name);
	}

	size_t size() const
	{
		return m_map.size();
	}

	const std::string& name() const
	{
		return m_dn;
	}

	Steamworks::LDAP::Update::Attributes::const_iterator begin() const
	{
		return m_map.begin();
	}

	Steamworks::LDAP::Update::Attributes::const_iterator end() const
	{
		return m_map.end();
	}
} ;

class LDAPMods
{
private:
	::ldapmod** m_mods;
	size_t m_size;
	unsigned int m_count;

public:
	LDAPMods(size_t n) :
		m_mods((::ldapmod**)calloc(n+1, sizeof(::ldapmod*))),
		m_size(n),
		m_count(0)
	{
	}

	~LDAPMods()
	{
		for (unsigned int i=0; i<m_count; i++)
		{
			free(m_mods[i]->mod_vals.modv_strvals);
			free(m_mods[i]);
		}
		free(m_mods);
		m_mods = nullptr;
	}

	void replace(const std::string& attr, const std::string& val)
	{
		::ldapmod* mod;

		if (m_count < m_size)
		{
			// TODO: check for allocation failures
			mod = m_mods[m_count++] = (::ldapmod*)malloc(sizeof(::ldapmod));
			mod->mod_op = LDAP_MOD_REPLACE;
			mod->mod_type = const_cast<char *>(attr.c_str());
			mod->mod_vals.modv_strvals = (char**)calloc(2, sizeof(char *));
			mod->mod_vals.modv_strvals[0] = const_cast<char *>(val.c_str());
			mod->mod_vals.modv_strvals[1] = nullptr;
		}
	}

	::ldapmod** c_ptr() const
	{
		return m_mods;
	}
} ;

Steamworks::LDAP::Update::Update(const std::string& dn) :
	Action(false),
	d(new Private(dn))
{

}

Steamworks::LDAP::Update::Update(const std::string& dn, const Steamworks::LDAP::Update::Attributes& attr) :
	Action(attr.size() > 0),
	d(new Private(dn))
{
	d->update(attr);
}

Steamworks::LDAP::Update::Update(const picojson::value& json) :
	Action(false),  // For now
	d(nullptr)
{
	if (!json.is<picojson::value::object>())
	{
		return;
	}

	std::string dn = json.get("dn").to_str();
	if (!dn.empty())
	{
		d.reset(new Private(dn));
	}
	else
	{
		return;  // no dn? remain invalid
	}

	const picojson::object& o = json.get<picojson::object>();
	if (o.size() > 1)  // "dn" plus one more
	{
		for (auto i: o)
		{
			if (i.first != "dn")
			{
				d->update(i.first, i.second.to_str());
			}
		}
		m_valid = true;
	}
}

Steamworks::LDAP::Update::~Update()
{
}

void Steamworks::LDAP::Update::execute(Connection& conn, Result result)
{
	// TODO: actually do an update
	Steamworks::Logging::Logger& log = Steamworks::Logging::getLogger("steamworks.ldap");
	if (!m_valid)
	{
		log.warnStream() << "Can't execute invalid update.";
		return;
	}

	log.debugStream() << "Update execute:" << d->name() << " #changes:" << d->size();

	// TODO: here we assume each JSON-change maps to  one LDAP modification
	LDAPMods mods(d->size());
	for (auto i = d->begin(); i != d->end(); ++i)
	{
		log.debugStream() << "  A=" << i->first << " V=" << i->second;
		mods.replace(i->first, i->second);
	}

	int r = ldap_modify_ext_s(
		handle(conn),
		d->name().c_str(),
		mods.c_ptr(),
		server_controls(conn),
		client_controls(conn)
		);
	log.debugStream() << "Result " << r << " " << (r ? ldap_err2string(r) : "OK");
}


/**
 * Addition as a variation on updates.
 */
Steamworks::LDAP::Addition::Addition(const picojson::value& v): Update(v)
{
}


void Steamworks::LDAP::Addition::execute(Connection& conn, Result result)
{
	// TODO: actually do an update
	Steamworks::Logging::Logger& log = Steamworks::Logging::getLogger("steamworks.ldap");
	if (!m_valid)
	{
		log.warnStream() << "Can't execute invalid addition.";
		return;
	}

	log.debugStream() << "Addition execute:" << d->name() << " #changes:" << d->size();

	// TODO: here we assume each JSON-change maps to  one LDAP modification
	LDAPMods mods(d->size());
	for (auto i = d->begin(); i != d->end(); ++i)
	{
		log.debugStream() << "  A=" << i->first << " V=" << i->second;
		mods.replace(i->first, i->second);
	}

	int r = ldap_add_ext_s(
		handle(conn),
		d->name().c_str(),
		mods.c_ptr(),
		server_controls(conn),
		client_controls(conn)
		);
	log.debugStream() << "Result " << r << " " << (r ? ldap_err2string(r) : "OK");
}




/** Internals of a Remove (delete) action.
 */
class Steamworks::LDAP::Remove::Private
{
private:
	std::string m_dn;

public:
	Private(const std::string& dn) :
		m_dn(dn)
	{
	};

	const std::string& name() const
	{
		return m_dn;
	}
} ;

Steamworks::LDAP::Remove::Remove(const std::string& dn) :
	Action(!dn.empty()),
	d(new Private(dn))
{
}

Steamworks::LDAP::Remove::~Remove()
{
}

void Steamworks::LDAP::Remove::execute(Connection& conn, Result result)
{
	Steamworks::Logging::Logger& log = Steamworks::Logging::getLogger("steamworks.ldap");
	if (!is_valid())
	{
		log.warnStream() << "Can't execute invalid removal.";
		return;
	}

	log.debugStream() << "Removal execute:" << d->name();

	int r = ldap_delete_ext_s(
		handle(conn),
		d->name().c_str(),
		server_controls(conn),
		client_controls(conn)
		);
	log.debugStream() << "Result " << r << " " << (r ? ldap_err2string(r) : "OK");
}
