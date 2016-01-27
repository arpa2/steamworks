/*
Copyright (c) 2014-2016 InternetWide.org and the ARPA2.net project
All rights reserved. See file LICENSE for exact terms (2-clause BSD license).

Adriaan de Groot <groot@kde.org>
*/

#include <ldap.h>

#include "../logger.h"
#include "serverinfo.h"


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

void Steamworks::LDAP::ServerControlInfo::execute(::LDAP* ldaphandle)
{
	// TODO: search root DSE
}

Steamworks::LDAP::ServerControlInfo::~ServerControlInfo()
{
}
