/*
Copyright (c) 2014-2016 InternetWide.org and the ARPA2.net project
All rights reserved. See file LICENSE for exact terms (2-clause BSD license).

Adriaan de Groot <groot@kde.org>
*/

/**
 * Private implementation details.
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

void copy_entry(::LDAP* ldaphandle, ::LDAPMessage* entry, Result map);

}  // namespace LDAP
}  // namespace Steamworks

#endif
