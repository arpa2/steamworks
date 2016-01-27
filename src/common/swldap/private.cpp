/*
Copyright (c) 2014-2016 InternetWide.org and the ARPA2.net project
All rights reserved. See file LICENSE for exact terms (2-clause BSD license).

Adriaan de Groot <groot@kde.org>
*/

#include "connection.h"

namespace Steamworks
{

namespace LDAP
{

void copy_entry(::LDAP* ldaphandle, ::LDAPMessage* entry, Result map)
{
	if (!map)
	{
		return;
	}

	// TODO: guard against memory leaks
	BerElement* berp(nullptr);
	char *attr = ldap_first_attribute(ldaphandle, entry, &berp);

	while (attr != nullptr)
	{
		berval** values = ldap_get_values_len(ldaphandle, entry, attr);
		auto values_len = ldap_count_values_len(values);
		if (values_len == 0)
		{
			// TODO: can this happen?
			picojson::value v_attr;
			map->emplace(attr, v_attr);  // null
		}
		else if (values_len > 1)
		{
			// FIXME: decode ber-values
			picojson::value::array v_array;
			v_array.reserve(values_len);
			for (decltype(values_len) i=0; i<values_len; i++)
			{
				picojson::value v_attr(values[i]->bv_val);
				v_array.emplace_back(v_attr);
			}
		}
		else
		{
			// FIXME: decode ber-values
			picojson::value v_attr(values[0]->bv_val);
			map->emplace(attr, v_attr);
		}
		ldap_value_free_len(values);
		attr = ldap_next_attribute(ldaphandle, entry, berp);
	}

	if (berp)
	{
		ber_free(berp, 0);
	}
}

}  // namespace LDAP
}  // namespace Steamworks
