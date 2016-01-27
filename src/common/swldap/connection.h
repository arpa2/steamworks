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

namespace Steamworks
{

namespace LDAP
{
class Action;

/**
 * Connection to an LDAP server.
 */
class Connection
{
friend class Action;

private:
	class Private;
	std::unique_ptr<Private> d;
	bool valid;

protected:
	::LDAP* handle() const;

public:
	/**
	 * Connect to an LDAP server given by the URI.
	 * All connections use TLS, regardless of the protocol
	 * in the URI, so it is preferable to use "ldap:" URIs
	 * over other methods.
	 */
	Connection(const std::string& uri);
	~Connection();
	bool is_valid() const { return valid; }
} ;


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


public:
	Action() : m_valid(false)
	{
	}

	bool is_valid() const { return m_valid; }

	void execute(Connection& c)  // FIXME: results? arguments?
	{
		return execute(c.handle());
	}

	virtual void execute(::LDAP*) = 0;  // FIXME: results? arguments?
} ;


}  // namespace LDAP
}  // namespace

#endif
