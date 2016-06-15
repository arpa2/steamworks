/*
Copyright (c) 2014-2016 InternetWide.org and the ARPA2.net project
All rights reserved. See file LICENSE for exact terms (2-clause BSD license).

Adriaan de Groot <groot@kde.org>
*/

#include "sync.h"

#include "serverinfo.h"
#include "private.h"

#include "picojson.h"

#include <iomanip>

static void dump_uuid(Steamworks::Logging::LoggerStream& log, struct berval* uuid)
{
	log << std::hex << std::setfill('0') << std::setw(2);
	for (int i = 0; i<uuid->bv_len; i++)
	{
		log << (int(uuid->bv_val[i]) & 0xff);
	}
}

static char hex[] = "0123456789ABCDEF";

static void dump_uuid(std::string& s, struct berval* uuid)
{
	static_assert(sizeof(hex) == 17, "Hex != 16");  // Allow for trailing NUL byte

	for (int i = 0; i<uuid->bv_len; i++)
	{
		char c = uuid->bv_val[i];
		char c0 = hex[(c & 0xf0) >> 4];
		char c1 = hex[c & 0x0f];
		s[i*2] = c0;
		s[i*2+1] = c1;
	}
}

static void dump_object(Steamworks::Logging::Logger& log, const picojson::object& d)
{
	for (auto& kv: d)
	{
		log.debugStream() << " .. k " << kv.first;
	}
}

/**
 * Internals of a sync.
 */
class DITCore
{
private:
	std::map<std::string, picojson::object> m_dit;  // uuid to object (name/value pairs)
public:
	/** Clear the (cached) DIT */
	void clear()
	{
		m_dit.clear();
	}

	void search_entry_f(::LDAP* ldap, ::LDAPMessage* msg, struct berval* entryUUID, ldap_sync_refresh_t phase)
	{
		Steamworks::Logging::Logger& log = Steamworks::Logging::getLogger("steamworks.ldap.sync");

		std::string key(entryUUID->bv_len * 2, '0');
		dump_uuid(key, entryUUID);

		std::string dn(ldap_get_dn(ldap, msg));

		if (m_dit.count(key))
		{
			log.debugStream() << "Known entry " << key;
			// TODO: update it
		}
		else
		{
			log.debugStream() << "New entry   " << key;
			m_dit.insert(std::make_pair(key, picojson::object()));
			auto& new_v = m_dit.at(key);  // Reference in the map
			Steamworks::LDAP::copy_entry(ldap, msg, &new_v);
			if (!new_v.count("dn"))
			{
				picojson::value v(dn);
				new_v.emplace(std::string("dn"), v);
			}
			// dump_object(log, new_v);
		}
	}

	void dump(Steamworks::LDAP::Result result) const
	{
		Steamworks::Logging::Logger& log = Steamworks::Logging::getLogger("steamworks.ldap.sync");
		for (auto& d: m_dit)
		{
			log.debugStream() << "Dumping " << d.first;
			picojson::value v(d.second);
			result->emplace(d.first.c_str(), v);
		}
	}
} ;



class Steamworks::LDAP::SyncRepl::Private
{
private:
	std::string m_base, m_filter;
	::ldap_sync_t m_syncrepl;
	DITCore m_dit;

public:
	Private(const std::string& base, const std::string& filter);

	const std::string& base() const { return m_base; }
	const std::string& filter() const { return m_filter; }
	::ldap_sync_t* sync() { return &m_syncrepl; }
	const DITCore& dit() const { return m_dit; }
} ;


/**
 * Internal (C-style) callback functions for part of sync.
 */
static int search_entry_f(ldap_sync_t* ls, LDAPMessage* msg, struct berval* entryUUID, ldap_sync_refresh_t phase)
{
	Steamworks::Logging::Logger& log = Steamworks::Logging::getLogger("steamworks.ldap.sync");

	auto stream = log.debugStream();
	stream << "Entry: " << ldap_get_dn(ls->ls_ld, msg) << " UUID ";
	dump_uuid(stream, entryUUID);
	reinterpret_cast<DITCore*>(ls->ls_private)->search_entry_f(ls->ls_ld, msg, entryUUID, phase);
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

	if (syncUUIDs)
	{
		auto stream = log.debugStream();
		stream << "UUIDs: " << (void *)syncUUIDs << ' ';
		dump_uuid(stream, syncUUIDs);
	}

	return 0;
}

static int search_result_f(ldap_sync_t* ls, LDAPMessage* msg, int refreshDeletes)
{
	Steamworks::Logging::Logger& log = Steamworks::Logging::getLogger("steamworks.ldap.sync");

	log.debugStream() << "Result: " << ldap_get_dn(ls->ls_ld, msg);
	return 0;
}


/**
 * SyncRepl implementation uses C-style functions above.
 */
Steamworks::LDAP::SyncRepl::Private::Private(const std::string& base, const std::string& filter):
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
	m_syncrepl.ls_private = &m_dit;  // Private data for this sync
	m_syncrepl.ls_ld = nullptr;  // Done in execute()
}




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

	log.debugStream() << "SyncRepl setup for base='" << d->base() << "' filter='" << d->filter() << "'";

	::ldap_sync_t* syncrepl = d->sync();
	syncrepl->ls_ld = ldaphandle;
	int r = ldap_sync_init(syncrepl, LDAP_SYNC_REFRESH_AND_PERSIST);
	if (r)
	{
		log.errorStream() << "Sync setup result " << r << " " << ldap_err2string(r);
		return;
	}

	poll(conn);
}

void Steamworks::LDAP::SyncRepl::poll(Connection& conn)
{
	::LDAP* ldaphandle = handle(conn);
	::ldap_sync_t* syncrepl = d->sync();
	syncrepl->ls_ld = ldaphandle;
	int r = ldap_sync_poll(syncrepl);
	if (r)
	{
		Steamworks::Logging::Logger& log = Steamworks::Logging::getLogger("steamworks.ldap");
		log.errorStream() << "Sync poll result " << r << " " << ldap_err2string(r);
	}
}

void Steamworks::LDAP::SyncRepl::dump_dit(Result result)
{
	d->dit().dump(result);
}
