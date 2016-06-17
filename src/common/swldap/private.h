/*
Copyright (c) 2014-2016 InternetWide.org and the ARPA2.net project
All rights reserved. See file LICENSE for exact terms (2-clause BSD license).

Adriaan de Groot <groot@kde.org>
*/

/**
 * Private implementation details, also support functions.
 */

#ifndef SWLDAP_PRIVATE_H
#define SWLDAP_PRIVATE_H

#include <string>

#include "../logger.h"
#include "picojson.h"

#include "connection.h"

namespace Steamworks
{

namespace LDAP
{

/**
 * Copy the DIT entry contained in @p entry into the JSON object @p results,
 * with attribute names as keys in @p results (e.g. "dn" is a key) and
 * null, string or array values for each attribute.
 */
void copy_entry(::LDAP* ldaphandle, ::LDAPMessage* entry, Result results);
/**
 * Copy the DIT entries contained in @p res into the JSON object @p results,
 * logging to @p log as needed. The DN of each entry is used as key in
 * @p results, and each value is a JSON object filled as described in
 * copy_entry().
 */
void copy_search_result(::LDAP* ldaphandle, ::LDAPMessage* res, Result results, Logging::Logger& log);

}  // namespace LDAP
}  // namespace Steamworks

#endif
