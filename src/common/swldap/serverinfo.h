/*
Copyright (c) 2014-2016 InternetWide.org and the ARPA2.net project
All rights reserved. See file LICENSE for exact terms (2-clause BSD license).

Adriaan de Groot <groot@kde.org>
*/

#ifndef SWLDAP_SERVERINFO_H
#define SWLDAP_SERVERINFO_H

#include <string>

#include "../logger.h"
#include "../jsonresponse.h"
#include "connection.h"

namespace SteamWorks {
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

	virtual void execute(Connection&, Result result=nullptr);
	// Special case: when the connection is being constructed and
	//  we already have a handle, but not a complete Connection object.
	void execute(::LDAP*);

	/**
	 * Log interesting fields to the given @p log at @p level (e.g. DEBUG).
	 *
	 * TODO: stream operator?
	 */
	void log(SteamWorks::Logging::Logger& log, SteamWorks::Logging::LogLevel level) const;

	/// Gets the API version number (usually 3), or -1 on error.
	int get_version() const { return is_valid() ? m_info.ldapai_info_version : -1; }
} ;

/**
 * Class representing Server controls (which ones are available).
 * This information is only filled in after an execute() is done.
 */
class ServerControlInfo : public Action
{
private:
	class Private;
	std::unique_ptr<Private> d;

public:
	ServerControlInfo();
	~ServerControlInfo();

	virtual void execute(Connection&, Result result=nullptr);

	/// TODO: logging?
	bool is_available(const std::string& oid) const;

	static const char SC_SYNC[];            // 1.3.6.1.4.1.4203.1.9.1.1         RFC4533  Sync Request Control
	static const char SC_SORTREQUEST[];     // 1.2.840.113556.1.4.473           RFC2891  Sorting
	static const char SC_SORTRESPONSE[];    // 1.2.840.113556.1.4.474           RFC2891  Sorting
} ;

/**
 * Check that a given @p connection supports the given server control @p control
 * (an OID string). Returns true if it does; otherwise, false and an explanation
 * is given in @p response.
 */
bool require_server_control(ConnectionUPtr& connection, const char *control, JSON::Object response, Logging::Logger& log);
/**
 * Convenience-function for require_server_control(..., SC_SYNC, ...)
 */
bool require_syncrepl(ConnectionUPtr& connection, JSON::Object response, Logging::Logger& log);


/**
 * Class representing a query for type information related to all
 * objectclasses. Returns the complete subschema-definition, which
 * isn't all that useful because it needs parsing to find out what
 * types apply to what attributes.
 */
class TypeInfo : public Action
{
private:
	class Private;
	std::unique_ptr<Private> d;

public:
	TypeInfo();
	~TypeInfo();

	virtual void execute(Connection&, Result result=nullptr);
} ;

} // namespace LDAP
} // namespace Steamworks

#endif
