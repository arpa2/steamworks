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
 * Internal callback functions for part of sync.
 */
static int search_entry_f(ldap_sync_t* ls, LDAPMessage* msg, struct berval* entryUUID, ldap_sync_refresh_t phase)
{
	Steamworks::Logging::Logger& log = Steamworks::Logging::getLogger("steamworks.ldap.sync");

	log.debugStream() << "Entry: " << ldap_get_dn(ls->ls_ld, msg);
	return 0;
}

static int search_reference_f(ldap_sync_t* ls, LDAPMessage* msg)
{
    Steamworks::Logging::Logger& log = Steamworks::Logging::getLogger("steamworks.ldap.sync");

    log.debugStream() << "Reference: " << ldap_get_dn(ls->ls_ld, msg);
    return 0;
}

static int search_intermediate_f(ldap_sync_t* ls, LDAPMessage* msg, BerVarray syncUUIDs, ldap_sync_refresh_t phase)
{
    Steamworks::Logging::Logger& log = Steamworks::Logging::getLogger("steamworks.ldap.sync");

    log.debugStream() << "Intermediate: " << ldap_get_dn(ls->ls_ld, msg);
    return 0;
}

static int search_result_f(ldap_sync_t* ls, LDAPMessage* msg, int refreshDeletes)
{
    Steamworks::Logging::Logger& log = Steamworks::Logging::getLogger("steamworks.ldap.sync");

    log.debugStream() << "Result: " << ldap_get_dn(ls->ls_ld, msg);
    return 0;
}

/**
 * Internals of a sync.
 */
class Steamworks::LDAP::SyncRepl::Private
{
private:
	std::string m_base, m_filter;
	::ldap_sync_t m_syncrepl;
public:
	Private(const std::string& base, const std::string& filter) :
		m_base(base),
		m_filter(filter)
	{
		ldap_sync_initialize(&m_syncrepl);
		m_syncrepl.ls_base = const_cast<char *>(m_base.c_str());
		m_syncrepl.ls_scope = LDAP_SCOPE_SUBTREE;
		m_syncrepl.ls_filter = const_cast<char *>(m_filter.c_str());
		m_syncrepl.ls_attrs = nullptr;  // All
		m_syncrepl.ls_timelimit = 0;  // No limit
		m_syncrepl.ls_sizelimit = 0;  // No limit
		m_syncrepl.ls_timeout = 2;  // Non-blocking on ldap_sync_poll()
		m_syncrepl.ls_search_entry = search_entry_f;
		m_syncrepl.ls_search_reference = search_reference_f;
		m_syncrepl.ls_intermediate = search_intermediate_f;
		m_syncrepl.ls_search_result = search_result_f;
		m_syncrepl.ls_private = nullptr;  // Private data for this sync
		m_syncrepl.ls_ld = nullptr;  // Done in execute()
	}

	const std::string& base() const { return m_base; }
	const std::string& filter() const { return m_filter; }
	::ldap_sync_t* sync() { return &m_syncrepl; }
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
		log.debugStream() << " .. control " << (void *)*ctls << " " << ((*ctls)->ldctl_oid ? (*ctls)->ldctl_oid : "null");
		ctls++;
		ctl_count++;
	}

	return ctl_count;
}


void Steamworks::LDAP::SyncRepl::execute(Connection& conn, Result results)
{
	::LDAP* ldaphandle = handle(conn);

	Steamworks::Logging::Logger& log = Steamworks::Logging::getLogger("steamworks.ldap");

	// TODO: settings for timeouts?
	struct timeval tv;
	tv.tv_sec = 2;
	tv.tv_usec = 0;

	::ldap_sync_t* syncrepl = d->sync();
	syncrepl->ls_ld = ldaphandle;
	int r = ldap_sync_init(syncrepl, LDAP_SYNC_REFRESH_AND_PERSIST);
	if (r)
	{
		log.errorStream() << "Sync setup result " << r << " " << ldap_err2string(r);
		return;
	}

	for (unsigned int i=0; i<10; i++)
	{
		r = ldap_sync_poll(syncrepl);
		if (r)
		{
			log.errorStream() << "Sync poll result " << r << " " << ldap_err2string(r);
			break;
		}
	}
}
