/*
Copyright (c) 2014, 2015 InternetWide.org and the ARPA2.net project
All rights reserved. See file LICENSE for exact terms (2-clause BSD license).

Adriaan de Groot <groot@kde.org>
*/

#include <ldap.h>

#include "ldapcpp.h"
#include "logger.h"

/**
 * Internal class representing the LDAP API information (version, vendor, etc.)
 * retrieved with ldap_get_option(LDAP_OPT_API_INFO). Creating an object of
 * this type retrieves the information; it is freed on destruction. Ownership
 * of any pointers in the info-structure remains with the _APIInfo object.
 */
class _APIInfo
{
private:
	LDAPAPIInfo info;
	bool valid;

public:
	/** Get the API information from the given @p ldaphandle */
	_APIInfo(LDAP* ldaphandle) :
		valid(false)
	{
		info.ldapai_info_version = LDAP_API_INFO_VERSION;
		int r = ldap_get_option(ldaphandle, LDAP_OPT_API_INFO, &info);
		if (r) return;
		valid = true;
	}

	~_APIInfo()
	{
		if (!valid) return;

		ldap_memfree(info.ldapai_vendor_name);
		if (info.ldapai_extensions)
		{
			for (char **s = info.ldapai_extensions; *s; s++)
			{
				ldap_memfree(*s);
			}
			ldap_memfree(info.ldapai_extensions);
		}
	}

	/**
	 * Log interesting fields to the given @p log at @p level (e.g. DEBUG).
	 */
	void log(Steamworks::Logging::Logger& log, Steamworks::Logging::LogLevel level) const
	{
		if (!valid)
		{
			log.getStream(level) << "LDAP API invalid";
			return;
		}
		log.getStream(level) << "LDAP API version " << info.ldapai_info_version;
		log.getStream(level) << "LDAP API vendor  " << (info.ldapai_vendor_name ? info.ldapai_vendor_name : "<unknown>");
		if (info.ldapai_extensions)
		{
			for (char **s = info.ldapai_extensions; *s; s++)
			{
				log.getStream(level) << "LDAP extension   " << *s;
			}
		}
	}

	bool is_valid() const { return valid; }
	/// Gets the API version number (usually 3), or -1 on error.
	int get_version() const { return is_valid() ? info.ldapai_info_version : -1; }
} ;


/**
 * Internals of a search.
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

/**
 * Actual implementation of the connection parts.
 * Holds on to the pointers that the OpenLDAP
 * C-API uses for a connection.
 */
class Steamworks::LDAP::Connection::Private
{
private:
	std::string uri;
	::LDAP* ldaphandle;
	::LDAPControl* serverctl;
	::LDAPControl* clientctl;
	bool valid;

public:
	Private(const std::string& uri) :
		uri(uri),
		ldaphandle(nullptr),
		serverctl(nullptr),
		clientctl(nullptr),
		valid(false)
	{
		Steamworks::Logging::Logger& log = Steamworks::Logging::getLogger("steamworks.ldap");
		log.debugStream() << "LDAP connect to '" << uri << "'";

		int r = 0;

		r = ldap_initialize(&ldaphandle, uri.c_str());
		if (r)
		{
			log.errorStream() << "Could not initialize LDAP. Error " << r;
			if (ldaphandle)
			{
				ldap_memfree(ldaphandle);
			}
			return;
		}
		if (!ldaphandle)
		{
			log.errorStream() << "LDAP initialized, but handle is NULL.";
			return;
		}

		// Here and following, if (true) is just a marker for a block scope
		if (true)
		{
			_APIInfo info(ldaphandle);
			if (!info.is_valid())
			{
				log.errorStream() << "Could not get LDAP API info."
					<< " Expected API version " << LDAP_API_INFO_VERSION
					<< " but got " << info.get_version();
				return;
			}
			info.log(log, Steamworks::Logging::DEBUG);
		}

		if (true)
		{
			int protocol_version = 3;
			r = ldap_set_option(ldaphandle, LDAP_OPT_PROTOCOL_VERSION, &protocol_version);
			if (r)
			{
				log.errorStream() << "Could not set LDAP protocol version 3. Error" << r;
				// TODO: close handle
				return;
			}
		}

		if (true)
		{
			int tls_require_cert = LDAP_OPT_X_TLS_NEVER;
			r = ldap_set_option(ldaphandle, LDAP_OPT_X_TLS_REQUIRE_CERT, &tls_require_cert);
			if (r)
			{
				log.warnStream() << "Could not ignore TLS certificate. Error" << r;
			}
			// But carry on ..
		}

		// TODO: checking that this works is tricky
		r = ldap_start_tls_s(ldaphandle, &serverctl, &clientctl);
		if (r)
		{
			log.errorStream() << "Could not start TLS. Error" << r << ", " << ldap_err2string(r);
		}

		valid = true;
	}

	~Private()
	{
		if (valid)
		{
			// TODO: disconnect gracefully
			valid = false;
		}
		ldap_memfree(ldaphandle);
		ldaphandle = nullptr;
	}

	bool is_valid() const { return valid && ldaphandle; }

	void execute(const Steamworks::LDAP::Search& s, picojson::value::object* results)
	{
		Steamworks::Logging::Logger& log = Steamworks::Logging::getLogger("steamworks.ldap");

		// TODO: settings for timeouts?
		struct timeval tv;
		tv.tv_sec = 2;
		tv.tv_usec = 0;

		LDAPMessage *res;
		int r = ldap_search_ext_s(ldaphandle,
			s.d->base().c_str(),
			LDAP_SCOPE_SUBTREE,
			s.d->filter().c_str(),
			nullptr,  // attrs
			0,
			&serverctl,
			&clientctl,
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

					copy_entry(entry, placed_map);
				}
				entry = ldap_next_entry(ldaphandle, entry);
			}
		}
		log.infoStream() << " .. search OK.";

		ldap_msgfree(res);
	}

	void copy_entry(LDAPMessage* entry, picojson::value::object& map)
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
} ;



Steamworks::LDAP::Connection::Connection(const std::string& uri) :
	d(new Private(uri)),
	valid(false)
{
	valid = d->is_valid();
}

Steamworks::LDAP::Connection::~Connection()
{
}

void Steamworks::LDAP::Connection::execute(const Steamworks::LDAP::Search& search, picojson::value::object* results)
{
	return d->execute(search, results);
}

Steamworks::LDAP::Search::Search(const std::string& base, const std::string& filter) :
	d(new Private(base, filter)),
	valid(true)
{
}

Steamworks::LDAP::Search::~Search()
{
}
