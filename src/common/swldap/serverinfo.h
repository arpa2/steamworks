/*
Copyright (c) 2014-2016 InternetWide.org and the ARPA2.net project
All rights reserved. See file LICENSE for exact terms (2-clause BSD license).

Adriaan de Groot <groot@kde.org>
*/

#include <ldap.h>

#include "../logger.h"


namespace Steamworks {
namespace LDAP {

/**
 * Class representing the LDAP API information (version, vendor, etc.)
 * retrieved with ldap_get_option(LDAP_OPT_API_INFO). Creating an object of
 * this type retrieves the information; it is freed on destruction. Ownership
 * of any pointers in the info-structure remains with the _APIInfo object.
 */
class APIInfo
{
private:
	LDAPAPIInfo m_info;
	bool m_valid;

public:
	/** Get the API information from the given @p ldaphandle */
	APIInfo(::LDAP* ldaphandle);
	~APIInfo();

	/**
	 * Log interesting fields to the given @p log at @p level (e.g. DEBUG).
	 *
	 * TODO: stream operator?
	 */
	void log(Steamworks::Logging::Logger& log, Steamworks::Logging::LogLevel level) const;

	/// Is the API information valid (e.g. successfully retrieved)?
	bool is_valid() const { return m_valid; }
	/// Gets the API version number (usually 3), or -1 on error.
	int get_version() const { return is_valid() ? m_info.ldapai_info_version : -1; }
} ;

class ServerControlInfo
{
private:
	std::string m_oid;
	bool m_valid;
	bool m_available;

public:
	ServerControlInfo(::LDAP* ldaphandle, const std::string& oid);
	~ServerControlInfo();

	/// TODO: logging?

	/// Is the API information valid (e.g. successfully retrieved)?
	bool is_valid() const { return m_valid; }
	/// Gets the API version number (usually 3), or -1 on error.
	int is_available() const { return m_available; }
	/// Gets the OID that this ServerControlInfo checked for.
	std::string oid() const { return is_valid() ? std::string() : m_oid; }
} ;

} // namespace LDAP
} // namespace Steamworks
