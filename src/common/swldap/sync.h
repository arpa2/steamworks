/*
Copyright (c) 2014-2016 InternetWide.org and the ARPA2.net project
All rights reserved. See file LICENSE for exact terms (2-clause BSD license).

Adriaan de Groot <groot@kde.org>
*/

/**
 * LDAP operations in a C++ jacket.
 *
 * Sync -- as in SyncRepl according to RFC4533 -- actions on an LDAP connection.
 */

#ifndef SWLDAP_SEARCH_H
#define SWLDAP_SEARCH_H

#include <string>

#include "connection.h"

namespace Steamworks
{

namespace LDAP
{
/**
 * (Synchronous) sync.
 */
class SyncRepl: public Action
{
private:
	class Private;
	std::unique_ptr<Private> d;

public:
	SyncRepl(const std::string& base, const std::string& filter);
	~SyncRepl();

	virtual void execute(Connection&, Result result=nullptr);
	void poll(Connection&);
	void resync();

	/** Debugging, dump the DIT entries stored in this SyncRepl into @p result */
	void dump_dit(Result result);
} ;

}  // namespace LDAP
}  // namespace

#endif
