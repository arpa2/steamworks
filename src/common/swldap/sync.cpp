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

static int search_entry_f(ldap_sync_t* ls, LDAPMessage* msg, struct berval* entryUUID, ldap_sync_refresh_t phase)
{
	Steamworks::Logging::Logger& log = Steamworks::Logging::getLogger("steamworks.ldap.sync");

	log.debugStream() << "Entry: " << ldap_get_dn(ls->ls_ld, msg);
	return 0;
}

static int search_reference_f(        ldap_sync_t                     *ls,
        LDAPMessage                     *msg )
{
	Steamworks::Logging::Logger& log = Steamworks::Logging::getLogger("steamworks.ldap.sync");

	log.debugStream() << "Reference: " << ldap_get_dn(ls->ls_ld, msg);
	return 0;
}

static int search_intermediate_f(        ldap_sync_t                     *ls,
        LDAPMessage                     *msg,
        BerVarray                       syncUUIDs,
        ldap_sync_refresh_t             phase )
{
	Steamworks::Logging::Logger& log = Steamworks::Logging::getLogger("steamworks.ldap.sync");

	log.debugStream() << "Intermediate: " << ldap_get_dn(ls->ls_ld, msg);
	return 0;
}

static int search_result_f(        ldap_sync_t                     *ls,
        LDAPMessage                     *msg,
        int                             refreshDeletes )
{
	Steamworks::Logging::Logger& log = Steamworks::Logging::getLogger("steamworks.ldap.sync");

	log.debugStream() << "Result: " << ldap_get_dn(ls->ls_ld, msg);
	return 0;
}



void Steamworks::LDAP::SyncRepl::execute(Connection& conn, Result results)
{
	::LDAP* ldaphandle = handle(conn);

	Steamworks::Logging::Logger& log = Steamworks::Logging::getLogger("steamworks.ldap");

	// TODO: settings for timeouts?
	struct timeval tv;
	tv.tv_sec = 2;
	tv.tv_usec = 0;

	::ldap_sync_t syncrepl;
	ldap_sync_initialize(&syncrepl);
	syncrepl.ls_base = const_cast<char *>(d->base().c_str());
	syncrepl.ls_scope = LDAP_SCOPE_SUBTREE;
	syncrepl.ls_filter = const_cast<char *>(d->filter().c_str());
	syncrepl.ls_attrs = nullptr;  // All
	syncrepl.ls_timelimit = 0;  // No limit
	syncrepl.ls_sizelimit = 0;  // No limit
	syncrepl.ls_timeout = 2;  // Non-blocking on ldap_sync_poll()
	syncrepl.ls_search_entry = search_entry_f;
	syncrepl.ls_search_reference = search_reference_f;
	syncrepl.ls_intermediate = search_intermediate_f;
	syncrepl.ls_search_result = search_result_f;
	syncrepl.ls_private = nullptr;  // Private data for this sync
	syncrepl.ls_ld = ldaphandle;

	int r = ldap_sync_init_refresh_and_persist(&syncrepl);
	if (r)
	{
		log.errorStream() << "Sync setup result " << r << " " << ldap_err2string(r);
		return;
	}

	for (unsigned int i=0; i<10; i++)
	{
		r = ldap_sync_poll(&syncrepl);
		if (r)
		{
			log.errorStream() << "Sync poll result " << r << " " << ldap_err2string(r);
			break;
		}
	}
}
