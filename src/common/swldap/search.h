/*
Copyright (c) 2014, 2015 InternetWide.org and the ARPA2.net project
All rights reserved. See file LICENSE for exact terms (2-clause BSD license).

Adriaan de Groot <groot@kde.org>
*/

/**
 * LDAP operations in a C++ jacket.
 *
 * Search and Update on an LDAP server. Both actions may be kept around and
 * execute()d one of more times (each time will run the search or query,
 * returning current results as needed).
 */

#ifndef SWLDAP_SEARCH_H
#define SWLDAP_SEARCH_H

#include <string>

#include "../logger.h"
#include "picojson.h"

#include "connection.h"

namespace SteamWorks
{

namespace LDAP
{
using LDAPScope = decltype(LDAP_SCOPE_SUBTREE);  // Probably int

/**
 * (Synchronous) search. The search places results in the result parameter of
 * execute(), in JSON form.
 */
class Search : public Action
{
private:
	class Private;
	std::unique_ptr<Private> d;
public:
	Search(const std::string& base, const std::string& filter, LDAPScope scope=ScopeSubtree);
	~Search();

	virtual void execute(Connection&, Result result=nullptr);

	typedef enum {
		ScopeSubtree = LDAP_SCOPE_SUBTREE,
		ScopeBase = LDAP_SCOPE_BASE
	} LDAPScope_e;
} ;

/**
 * (Synchronous) update. Updates change zero
 * or more attributes of a given dn.
 */
class Update : public Action
{
friend class Addition;
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

/**
 * (Synchronous) addition. Like an update,
 * but adds a new entry to the DIT with an
 * all-new DN.
 */
class Addition : public Update
{
public:
	Addition(const picojson::value& v);

	virtual void execute(Connection&, Result result=nullptr);
} ;

/**
 * (Synchronous) delete. Deletes a single DIT entry
 * identified by a dn.
 */
class Remove : public Action
{
private:
	class Private;
	std::unique_ptr<Private> d;
public:
	Remove(const std::string& dn);
	~Remove();

	virtual void execute(Connection&, Result result=nullptr);
} ;


}  // namespace LDAP
}  // namespace

#endif
