/*
Copyright (c) 2014-2016 InternetWide.org and the ARPA2.net project
All rights reserved. See file LICENSE for exact terms (2-clause BSD license).

Adriaan de Groot <groot@kde.org>
*/

#include "serverinfo.h"

#include "private.h"

#include <unordered_set>

SteamWorks::LDAP::APIInfo::APIInfo()
{
}

void SteamWorks::LDAP::APIInfo::execute(::LDAP* ldaphandle)
{
	m_info.ldapai_info_version = LDAP_API_INFO_VERSION;
	int r = ldap_get_option(ldaphandle, LDAP_OPT_API_INFO, &m_info);
	if (r) return;
	m_valid = true;
}

void SteamWorks::LDAP::APIInfo::execute(SteamWorks::LDAP::Connection& conn, Result _)
{
	execute(handle(conn));
}


SteamWorks::LDAP::APIInfo::~APIInfo()
{
	if (!m_valid) return;

	ldap_memfree(m_info.ldapai_vendor_name);
	if (m_info.ldapai_extensions)
	{
		for (char **s = m_info.ldapai_extensions; *s; s++)
		{
			ldap_memfree(*s);
		}
		ldap_memfree(m_info.ldapai_extensions);
	}
}


void SteamWorks::LDAP::APIInfo::log(SteamWorks::Logging::Logger& log, SteamWorks::Logging::LogLevel level) const
{
	if (!m_valid)
	{
		log.getStream(level) << "LDAP API invalid";
		return;
	}
	log.getStream(level) << "LDAP API version " << m_info.ldapai_info_version;
	log.getStream(level) << "LDAP API vendor  " << (m_info.ldapai_vendor_name ? m_info.ldapai_vendor_name : "<unknown>");
	if (m_info.ldapai_extensions)
	{
		for (char **s = m_info.ldapai_extensions; *s; s++)
		{
			log.getStream(level) << "LDAP extension   " << *s;
		}
	}
}

class SteamWorks::LDAP::ServerControlInfo::Private
{
private:
	using OIDSet = std::unordered_set<std::string>;

	OIDSet m_available_oids;

public:
	bool is_available(const std::string& oid) const { return m_available_oids.count(oid); }

	void add(const std::string& oid) { m_available_oids.insert(oid); }
	void clear() { m_available_oids.clear(); }
	void extract_controls(Result_t& r);
} ;

void SteamWorks::LDAP::ServerControlInfo::Private::extract_controls(Result_t& result)
{
	SteamWorks::Logging::Logger& log = SteamWorks::Logging::getLogger("steamworks.ldap");

	picojson::value a = result.at("").get("supportedControl");
	if (!a.is<picojson::array>())
	{
		return;
	}

	const picojson::array& arr = a.get<picojson::array>();
	for (auto i = arr.begin(); i != arr.end(); ++i)
	{
		std::string s(i->to_str());
		log.debugStream() << "Found supported server control " << s;
		m_available_oids.emplace(s);
	}
}



SteamWorks::LDAP::ServerControlInfo::ServerControlInfo() :
	d(new Private())
{
}

void SteamWorks::LDAP::ServerControlInfo::execute(Connection& conn, Result result)
{
	::LDAP* ldaphandle = handle(conn);

	SteamWorks::Logging::Logger& log = SteamWorks::Logging::getLogger("steamworks.ldap");

	log.debugStream() << "Check for server controls.";

	// TODO: settings for timeouts?
	struct timeval tv;
	tv.tv_sec = 2;
	tv.tv_usec = 0;

	const char *attrs[] = {LDAP_ALL_OPERATIONAL_ATTRIBUTES, nullptr};

	LDAPMessage* res;
	int r = ldap_search_ext_s(ldaphandle,
		"",
		LDAP_SCOPE_BASE,
		"(objectclass=*)",
		const_cast<char **>(attrs),
		0,
		server_controls(conn),
		client_controls(conn),
		&tv,
		1024*1024,
		&res);
	if (r)
	{
		log.errorStream() << "Search result " << r << " " << ldap_err2string(r);
		ldap_msgfree(res);
		return;
	}
	else
	{
		d->clear();
	}

	if (result)
	{
		copy_search_result(ldaphandle, res, result, log);
		d->extract_controls(*result);
	}
	else
	{
		// One to throw away.
		std::unique_ptr<Result_t> _result(new Result_t);
		copy_search_result(ldaphandle, res, _result.get(), log);
		d->extract_controls(*_result);
	}

	ldap_msgfree(res);
}

SteamWorks::LDAP::ServerControlInfo::~ServerControlInfo()
{
}

bool SteamWorks::LDAP::ServerControlInfo::is_available(const std::string& oid) const
{
	return d->is_available(oid);
}

const char SteamWorks::LDAP::ServerControlInfo::SC_SYNC[] = LDAP_CONTROL_SYNC;
const char SteamWorks::LDAP::ServerControlInfo::SC_SORTREQUEST[] = LDAP_CONTROL_SORTREQUEST;
const char SteamWorks::LDAP::ServerControlInfo::SC_SORTRESPONSE[] = LDAP_CONTROL_SORTRESPONSE;

bool SteamWorks::LDAP::require_server_control(ConnectionUPtr& connection, const char *control, JSON::Object response, Logging::Logger& log)
{
	SteamWorks::LDAP::ServerControlInfo info;
	info.execute(*connection);

	if (!info.is_available(control))
	{
		connection.reset(nullptr);
		log.warnStream() << "Server at " << connection->get_uri() << " does not support SyncRepl.";
		SteamWorks::JSON::simple_output(response, 501, "Server does not support SyncRepl", LDAP_UNAVAILABLE_CRITICAL_EXTENSION);
		return false;
	}

	return true;
}

bool SteamWorks::LDAP::require_syncrepl(SteamWorks::LDAP::ConnectionUPtr& connection, SteamWorks::JSON::Object response, SteamWorks::Logging::Logger& log)
{
	return require_server_control(connection, ServerControlInfo::SC_SYNC, response, log);
}


class SteamWorks::LDAP::TypeInfo::Private
{
private:
	std::string m_objectclass;

public:
	Private()
	{
	}
} ;

SteamWorks::LDAP::TypeInfo::TypeInfo() :
	Action(true),
	d(new Private())
{
}

SteamWorks::LDAP::TypeInfo::~TypeInfo()
{
}

void SteamWorks::LDAP::TypeInfo::execute(Connection& conn, Result result)
{
	::LDAP* ldaphandle = handle(conn);

	SteamWorks::Logging::Logger& log = SteamWorks::Logging::getLogger("steamworks.ldap");

	log.debugStream() << "Check for typeinfo.";

	// TODO: settings for timeouts?
	struct timeval tv;
	tv.tv_sec = 2;
	tv.tv_usec = 0;

	const char *attrs[] = {"objectClasses", nullptr};

	// TODO: cn=Subschema is OpenLDAP-specific and specifically warned-against
	//       at http://www.openldap.org/faq/data/cache/1366.html
	LDAPMessage* res;
	int r = ldap_search_ext_s(ldaphandle,
		"cn=Subschema",
		LDAP_SCOPE_BASE,
		"(objectclass=*)",
		const_cast<char **>(attrs),
		0,
		server_controls(conn),
		client_controls(conn),
		&tv,
		1024*1024,
		&res);
	if (r)
	{
		log.errorStream() << "Typeinfo result " << r << " " << ldap_err2string(r);
		ldap_msgfree(res);
		return;
	}

	if (result)
	{
		copy_search_result(ldaphandle, res, result, log);
	}
	ldap_msgfree(res);
}
