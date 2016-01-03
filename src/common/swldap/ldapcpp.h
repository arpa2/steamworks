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

#include "picojson.h"

namespace Steamworks
{

namespace LDAP
{
class Connection;

/**
 * (Synchronous) search. This does not actually do the search,
 * but represents the search and hangs on to its results.
 */
class Search
{
friend class Connection;
private:
	class Private;
	std::unique_ptr<Private> d;
	bool valid;
public:
	Search(const std::string& base, const std::string& filter);
	~Search();
	bool is_valid() const { return valid; }
} ;

/**
 * (Synchronous) update. This does not actually do an update,
 * but represents the change to be made. Updates change zero
 * or more attributes of a given dn.
 */
class Update
{
friend class Connection;
private:
	class Private;
	std::unique_ptr<Private> d;
	bool valid;
public:
	using Attributes=std::map<const std::string, const std::string>;
	Update(const std::string& dn);  // Empty update (not valid)
	Update(const std::string& dn, const Attributes& attr);  // Update one attribute
	Update(const picojson::value& json);  // Update multiple attributes
	~Update();
	bool is_valid() const { return valid; }

	bool add_update(const std::string& name, const std::string& value);
} ;

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


	void execute(const Search&, picojson::value::object* results=nullptr);
	void execute(const Update&, picojson::value::object* results=nullptr);
} ;

}  // namespace LDAP
}  // namespace