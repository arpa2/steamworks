/*
Copyright (c) 2016 InternetWide.org and the ARPA2.net project
All rights reserved. See file LICENSE for exact terms (2-clause BSD license).

Adriaan de Groot <groot@kde.org>
*/

/**
 * LDAP operations in a C++ jacket.
 *
 * Update a single DIT entry, with or without precondition-checking,
 * with or without automatic rollback, and optionally with explicit
 * rollback.
 */

#ifndef SWLDAP_UPDATE_H
#define SWLDAP_UPDATE_H

#include <string>

#include "connection.h"

namespace SteamWorks
{

namespace LDAP
{
/**
 * (Synchronous) update action.
 */
class Update: public Action
{
private:
	class Private;
	std::unique_ptr<Private> d;

public:
	/**
	 * Do an update on the object names by the dn in the @p entry.
	 * No precondition is imposed; the existing object is queried and
	 * then modified (or added if needed).
	 */
	Update(picojson::object& entry, bool automatic_rollback=true);
	~Update();

	bool has_automatic_rollback() const;
	void set_automatic_rollback(bool automatic_rollback);

	virtual void execute(SteamWorks::LDAP::Connection & conn, Result result) override;
} ;

class UpdateGroup: public Action
{
private:
	class Private;
	std::unique_ptr<Private> d;

public:
	UpdateGroup();
	~UpdateGroup();

	void add_update(const Update&);

	virtual void execute(SteamWorks::LDAP::Connection & conn, Result result) override;
} ;

}  // namespace LDAP
}  // namespace Steamworks

#endif
