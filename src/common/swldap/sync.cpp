/*
Copyright (c) 2014-2016 InternetWide.org and the ARPA2.net project
All rights reserved. See file LICENSE for exact terms (2-clause BSD license).

Adriaan de Groot <groot@kde.org>
*/

#include "sync.h"

#include "serverinfo.h"
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

// Side-effect: log the controls
static unsigned int _count_controls(Steamworks::Logging::Logger& log, ::LDAPControl** ctls, const char *message)
{
	log.debugStream() << message << " controls are set:";

	unsigned int ctl_count = 0;
	while (*ctls)
	{
		log.debugStream() << " .. control " << (void *)*ctls << " " << (*ctls)->ldctl_oid;
		ctls++;
		ctl_count++;
	}

	return ctl_count;
}

void Steamworks::LDAP::SyncRepl::execute(Connection& conn, Result results)
{
	::LDAP* ldaphandle = handle(conn);

	Steamworks::Logging::Logger& log = Steamworks::Logging::getLogger("steamworks.ldap");


	::LDAPControl** ctl = server_controls(conn);
	unsigned int ctl_count = _count_controls(log, ctl, "Server (1)");

	// TODO: check if syncrepl is already enabled

	// Allocate room for an extra ptr (+2 because of the NULL ptr at the end which wasn't counted).
	::LDAPControl** qctl = (::LDAPControl**)calloc(ctl_count+2, sizeof(::LDAPControl*));

	// Copy controls and double-null-terminate
	::LDAPControl** qctl_i = qctl;
	::LDAPControl** ctl_i = ctl;
	while (*ctl_i)
	{
		*qctl_i++ = *ctl_i++;
	}
	*qctl_i = NULL;  // This is where we'll add the new control
	*(qctl_i+1) = NULL;

	int r = ldap_control_create(ServerControlInfo::SC_SYNC, 1, NULL, 0, qctl_i);
	if (r)
	{
		log.errorStream() << "Control result " << r << " " << ldap_err2string(r);
		// ignore
	}

	(void) _count_controls(log, qctl, "Server (2)");

	// TODO: settings for timeouts?
	struct timeval tv;
	tv.tv_sec = 2;
	tv.tv_usec = 0;

	LDAPMessage* res;
	r = ldap_search_ext_s(ldaphandle,
		d->base().c_str(),
		LDAP_SCOPE_SUBTREE,
		d->filter().c_str(),
		nullptr,  // attrs
		0,
		qctl,
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
