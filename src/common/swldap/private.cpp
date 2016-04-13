/*
Copyright (c) 2014-2016 InternetWide.org and the ARPA2.net project
All rights reserved. See file LICENSE for exact terms (2-clause BSD license).

Adriaan de Groot <groot@kde.org>
*/

#include "private.h"

#include "serverinfo.h"

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
			picojson::value v_attr(v_array);
			map->emplace(attr, v_attr);
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

void copy_search_result(::LDAP* ldaphandle, ::LDAPMessage* res, Result results, Steamworks::Logging::Logger& log)
{
	log.infoStream() << "Search message count=" << ldap_count_messages(ldaphandle, res);

	auto count = ldap_count_entries(ldaphandle, res);
	log.infoStream() << " .. entries count=" << count;
	if (count)
	{
		LDAPMessage *entry = ldap_first_entry(ldaphandle, res);
		while (entry != nullptr)
		{
			std::string dn(ldap_get_dn(ldaphandle, entry));
			log.infoStream() << " .. entry dn=" << dn;

			if (results)
			{
				picojson::value::object object;
				picojson::value v_object(object);
				results->emplace(dn, v_object);

				picojson::value& placed_obj = results->at(dn);
				picojson::value::object& placed_map = placed_obj.get<picojson::value::object>();

				picojson::value v(dn);
				placed_map.emplace(std::string("dn"), v);

				copy_entry(ldaphandle, entry, &placed_map);
			}

			entry = ldap_next_entry(ldaphandle, entry);
		}
	}
	log.infoStream() << " .. search OK.";
}

}  // namespace LDAP
}  // namespace Steamworks
