/*
Copyright (c) 2014-2016 InternetWide.org and the ARPA2.net project
All rights reserved. See file LICENSE for exact terms (2-clause BSD license).

Adriaan de Groot <groot@kde.org>
*/

#include "sync.h"
#include "private.h"

#include "picojson.h"

/**
 * Internals of a sync.
 */
class Steamworks::LDAP::SyncRepl::Private
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

Steamworks::LDAP::SyncRepl::SyncRepl(const std::string& base, const std::string& filter) :
	Action(true),
	d(new Private(base, filter))
{
}

Steamworks::LDAP::SyncRepl::~SyncRepl()
{
}

void Steamworks::LDAP::SyncRepl::execute(Connection& conn, Result results)
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

	ldap_msgfree(res);
}
