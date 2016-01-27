/*
Copyright (c) 2014-2016 InternetWide.org and the ARPA2.net project
All rights reserved. See file LICENSE for exact terms (2-clause BSD license).

Adriaan de Groot <groot@kde.org>
*/

#ifndef SWLDAP_SERVERINFO_H
#define SWLDAP_SERVERINFO_H

#include <string>

#include "../logger.h"
#include "connection.h"

namespace Steamworks {
namespace LDAP {

/**
 * Class representing the LDAP API information (version, vendor, etc.)
 * retrieved with ldap_get_option(LDAP_OPT_API_INFO). Creating an object of
 * this type retrieves the information; it is freed on destruction. Ownership
 * of any pointers in the info-structure remains with the _APIInfo object.
 */
class APIInfo : public Action
{
private:
	LDAPAPIInfo m_info;

public:
	/** Get the API information from the given @p ldaphandle */
	APIInfo();
	~APIInfo();

	virtual void execute(::LDAP*);

	/**
	 * Log interesting fields to the given @p log at @p level (e.g. DEBUG).
	 *
	 * TODO: stream operator?
	 */
	void log(Steamworks::Logging::Logger& log, Steamworks::Logging::LogLevel level) const;

	/// Gets the API version number (usually 3), or -1 on error.
	int get_version() const { return is_valid() ? m_info.ldapai_info_version : -1; }
} ;

class ServerControlInfo : public Action
{
private:
	std::string m_oid;
	bool m_available;

public:
	ServerControlInfo(const std::string& oid);
	~ServerControlInfo();

	virtual void execute(::LDAP*);

	/// TODO: logging?

	/// Is the OID for this ServerControlInfo available on the server?
	int is_available() const { return m_available; }
	/// Gets the OID that this ServerControlInfo checked for.
	std::string oid() const { return m_oid; }
} ;

} // namespace LDAP
} // namespace Steamworks

#endif
