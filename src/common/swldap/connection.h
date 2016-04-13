/*
Copyright (c) 2014, 2015 InternetWide.org and the ARPA2.net project
All rights reserved. See file LICENSE for exact terms (2-clause BSD license).

Adriaan de Groot <groot@kde.org>
*/

/**
 * LDAP operations in a C++ jacket.
 *
 * The primary class here is Connection, which represents a connection
 * to an LDAP server. The connection may be invalid. Connections are
 * explicitly disconnected when destroyed.
 */

#ifndef SWLDAP_CONNECTION_H
#define SWLDAP_CONNECTION_H

#include <ldap.h>

#include <memory>
#include <string>

#include "picojson.h"

#include "../logger.h"
#include "../jsonresponse.h"

namespace Steamworks
{

namespace LDAP
{
class Action;

using Result_t = picojson::value::object;
using Result = Result_t*;

/**
 * Connection to an LDAP server.
 */
class Connection
{
friend class Action;

private:
	class Private;
	std::unique_ptr<Private> d;

protected:
	::LDAP* handle() const;
	::LDAPControl** client_controls() const;
	::LDAPControl** server_controls() const;

public:
	/**
	 * Connect to an LDAP server given by the URI.
	 * All connections use TLS, regardless of the protocol
	 * in the URI, so it is preferable to use "ldap:" URIs
	 * over other methods.
	 */
	Connection(const std::string& uri);
	~Connection();
	bool is_valid() const;
	std::string get_uri() const;
} ;

using ConnectionUPtr = std::unique_ptr<Steamworks::LDAP::Connection>;

/**
 * Try to connect to an LDAP server at the given URI.
 * The @p connection is reset regardless; if connection fails,
 * returns false and a response-code and -explanation in @p response.
 */
bool do_connect(ConnectionUPtr& connection, const std::string& uri, JSON::Object response, Logging::Logger& log);

/**
 * Base class for actions that are created (once) and executed (possibly more than once)
 * against an LDAP connection.
 */
class Action
{
protected:
	bool m_valid;

	Action(bool a) : m_valid(a)
	{
	}

	::LDAP* handle(Connection& conn) const { return conn.handle(); }
	::LDAPControl** client_controls(Connection& conn) const { return conn.client_controls(); }
	::LDAPControl** server_controls(Connection& conn) const { return conn.server_controls(); }

public:
	Action() : m_valid(false)
	{
	}

	bool is_valid() const { return m_valid; }

	virtual void execute(Connection& conn, Result result=nullptr) = 0;
} ;


}  // namespace LDAP
}  // namespace

#endif
