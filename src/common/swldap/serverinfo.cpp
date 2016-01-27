/*
Copyright (c) 2014-2016 InternetWide.org and the ARPA2.net project
All rights reserved. See file LICENSE for exact terms (2-clause BSD license).

Adriaan de Groot <groot@kde.org>
*/

#include "serverinfo.h"

#include "private.h"

Steamworks::LDAP::APIInfo::APIInfo()
{
}

void Steamworks::LDAP::APIInfo::execute(::LDAP* ldaphandle)
{
	m_info.ldapai_info_version = LDAP_API_INFO_VERSION;
	int r = ldap_get_option(ldaphandle, LDAP_OPT_API_INFO, &m_info);
	if (r) return;
	m_valid = true;
}

void Steamworks::LDAP::APIInfo::execute(Steamworks::LDAP::Connection& conn, Result _)
{
	execute(handle(conn));
}


Steamworks::LDAP::APIInfo::~APIInfo()
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


void Steamworks::LDAP::APIInfo::log(Steamworks::Logging::Logger& log, Steamworks::Logging::LogLevel level) const
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

Steamworks::LDAP::ServerControlInfo::ServerControlInfo(const std::string& oid) :
	m_available(false),
	m_oid(oid)
{
}

void Steamworks::LDAP::ServerControlInfo::execute(Connection& conn, Result result)
{
	::LDAP* ldaphandle = handle(conn);

	Steamworks::Logging::Logger& log = Steamworks::Logging::getLogger("steamworks.ldap");

	log.debugStream() << "Check for server control " << m_oid;

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
		// TODO: does res need freeing here?
		return;
	}
	else
	{
	}

	copy_search_result(ldaphandle, res, result, log);

	ldap_msgfree(res);

}

Steamworks::LDAP::ServerControlInfo::~ServerControlInfo()
{
}
