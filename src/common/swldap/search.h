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

#ifndef SWLDAP_SEARCH_H
#define SWLDAP_SEARCH_H

#include <string>

#include "../logger.h"
#include "picojson.h"

#include "connection.h"

namespace Steamworks
{

namespace LDAP
{
void copy_entry(::LDAP* ldaphandle, ::LDAPMessage* entry, picojson::value::object& map);


/**
 * (Synchronous) search. This does not actually do the search,
 * but represents the search and hangs on to its results.
 */
class Search : public Action
{
private:
	class Private;
	std::unique_ptr<Private> d;
public:
	Search(const std::string& base, const std::string& filter);
	~Search();

	virtual void execute(Connection&, Result result=nullptr);
} ;

/**
 * (Synchronous) update. Updates change zero
 * or more attributes of a given dn.
 */
class Update : public Action
{
friend class Connection;
private:
	class Private;
	std::unique_ptr<Private> d;
public:
	using Attributes=std::map<const std::string, const std::string>;

	Update(const std::string& dn);  // Empty update (not valid)
	Update(const std::string& dn, const Attributes& attr);  // Update one attribute
	Update(const picojson::value& json);  // Update multiple attributes
	~Update();

	virtual void execute(Connection&, Result result=nullptr);
} ;

}  // namespace LDAP
}  // namespace

#endif
