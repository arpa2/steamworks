/*
Copyright (c) 2014, 2015 InternetWide.org and the ARPA2.net project
All rights reserved. See file LICENSE for exact terms (2-clause BSD license).

Adriaan de Groot <groot@kde.org>
*/

#include "search.h"

#include "picojson.h"

void Steamworks::LDAP::copy_entry(::LDAP* ldaphandle, ::LDAPMessage* entry, picojson::value::object& map)
{
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
			map.emplace(attr, v_attr);  // null
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
			map.emplace(attr, v_attr);
		}
		ldap_value_free_len(values);
		attr = ldap_next_attribute(ldaphandle, entry, berp);
	}

	if (berp)
	{
		ber_free(berp, 0);
	}
}

/**
 * Internals of a search. A search holds a base dn for the search
 * and a filter expression. When executed, it returns an array
 * of objects found.
 */
class Steamworks::LDAP::Search::Private
{
private:
	std::string m_base, m_filter;
public:
	Private(const std::string& base, const std::string& filter) :
		m_base(base),
		m_filter(filter)
	{
	}

	const std::string& base() const { return m_base; }
	const std::string& filter() const { return m_filter; }
} ;

Steamworks::LDAP::Search::Search(const std::string& base, const std::string& filter) :
	Action(true),
	d(new Private(base, filter))
{
}

Steamworks::LDAP::Search::~Search()
{
}

void Steamworks::LDAP::Search::execute(Connection& conn, Result results)
{
	::LDAP* ldaphandle = handle(conn);

	Steamworks::Logging::Logger& log = Steamworks::Logging::getLogger("steamworks.ldap");

	// TODO: settings for timeouts?
	struct timeval tv;
	tv.tv_sec = 2;
	tv.tv_usec = 0;

	LDAPMessage *res;
	int r = ldap_search_ext_s(ldaphandle,
		d->base().c_str(),
		LDAP_SCOPE_SUBTREE,
		d->filter().c_str(),
		nullptr,  // attrs
		0,
		server_controls(conn),
		client_controls(conn),
		&tv,
		1024*1024,
		&res);
	log.infoStream() << "Search returned " << r << " messages count=" << ldap_count_messages(ldaphandle, res);

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

				copy_entry(ldaphandle, entry, placed_map);
			}

			entry = ldap_next_entry(ldaphandle, entry);
		}
	}
	log.infoStream() << " .. search OK.";

	ldap_msgfree(res);
}



#if 0
/**
 * Internals of an update.
 */
class Steamworks::LDAP::Update::Private
{
private:
	std::string m_dn;
	Steamworks::LDAP::Update::Attributes m_map;
public:
	Private(const std::string& dn) :
		m_dn(dn)
	{
	};
	void update(const Steamworks::LDAP::Update::Attributes& attrs)
	{
		m_map.insert(attrs.cbegin(), attrs.cend());
	}
	void update(const std::string& name, const std::string& value)
	{
		m_map.emplace<>(name, value);
		Steamworks::Logging::Logger& log = Steamworks::Logging::getLogger("steamworks.ldap");
		log.debugStream() << "Update dn=" << m_dn << " attr " << name << "=" << value;
	}
	void remove(const std::string& name)
	{
		m_map.erase(name);
	}
} ;

Steamworks::LDAP::Update::Update(const std::string& dn) :
	d(new Private(dn)),
	valid(false)
{

}

Steamworks::LDAP::Update::Update(const std::string& dn, const Steamworks::LDAP::Update::Attributes& attr) :
	d(new Private(dn)),
	valid(attr.size() > 0)
{
	d->update(attr);
}

Steamworks::LDAP::Update::Update(const picojson::value& json) :
	d(nullptr),
	valid(false)
{
	if (!json.is<picojson::value::object>())
	{
		return;
	}
	std::string dn = json.get("dn").to_str();
	if (!dn.empty())
	{
		d.reset(new Private(dn));
	}
	else
	{
		return;  // no dn? remain invalid
	}
	const picojson::object& o = json.get<picojson::object>();
	if (o.size() > 1)  // "dn" plus one more
	{
		for (auto i: o)
		{
			if (i.first != "dn")
			{
				d->update(i.first, i.second.to_str());
			}
		}
		valid = true;
	}
}

Steamworks::LDAP::Update::~Update()
{
}
#endif
