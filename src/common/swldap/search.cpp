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
public:
	Private(const std::string& base, const std::string& filter) :
		m_base(base),
		m_filter(filter)
	{
	}

	const std::string& base() const { return m_base; }
	const std::string& filter() const { return m_filter; }
} ;

Steamworks::LDAP::Search::Search(const std::string& base, const std::string& filter) :
	Action(true),
	d(new Private(base, filter))
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
		LDAP_SCOPE_SUBTREE,
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
		Steamworks::Logging::Logger& log = Steamworks::Logging::getLogger("steamworks.ldap");
		log.debugStream() << "Update dn=" << m_dn << " attr " << name << "=" << value;
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

void Steamworks::LDAP::Update::execute(Connection&, Result result)
{
	// TODO: actually do an update
	Steamworks::Logging::Logger& log = Steamworks::Logging::getLogger("steamworks.ldap");

	log.debugStream() << "Update execute:" << d->name() << " #changes:" << d->size();
}
