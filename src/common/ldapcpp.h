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
#include <memory>
#include <string>

namespace Steamworks
{

namespace LDAP
{

/**
 * Connection to an LDAP server.
 */
class Connection
{
private:
	class Private;
	std::unique_ptr<Private> d;
	bool valid;

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

}  // namespace LDAP
}  // namespace
