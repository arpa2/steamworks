/*
Copyright (c) 2014-2016 InternetWide.org and the ARPA2.net project
All rights reserved. See file LICENSE for exact terms (2-clause BSD license).

Adriaan de Groot <groot@kde.org>
*/

#include <ldap.h>

#include "picojson.h"

#include "../logger.h"

#include "connection.h"
#include "serverinfo.h"

/**
 * Actual implementation of the connection parts.
 * Holds on to the pointers that the OpenLDAP
 * C-API uses for a connection.
 */
class Steamworks::LDAP::Connection::Private
{
friend class Steamworks::LDAP::Connection;

private:
	std::string uri;
	::LDAP* ldaphandle;
	::LDAPControl* serverctl;
	::LDAPControl* clientctl;
	bool valid;

	// Internal class that frees the LDAP handle unless the member variable
	// keep is set to true before destruction.
	class _handle_free
	{
	public:
		::LDAP*& handle;
		bool keep;
	public:
		_handle_free(::LDAP*& h) :
			handle(h),
			keep(false)
		{
		}

		~_handle_free()
		{
			if (handle && !keep)
			{
				ldap_memfree(handle);
				handle = nullptr;
			}
		}
	} ;

protected:
	::LDAP* handle() const { return ldaphandle; }

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

		_handle_free handle_disconnector(ldaphandle);

		int r = 0;

		r = ldap_initialize(&ldaphandle, uri.c_str());
		if (r)
		{
			log.errorStream() << "Could not initialize LDAP. Error " << r;
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
			APIInfo info;
			info.execute(ldaphandle);

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
				return;
			}
		}

		// From here on, we'll keep the LDAP connection.
		handle_disconnector.keep = true;

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
			// But carry on ..
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
		if (ldaphandle)
		{
			ldap_memfree(ldaphandle);
		}
		ldaphandle = nullptr;
	}

	bool is_valid() const { return valid && ldaphandle; }

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

LDAP* Steamworks::LDAP::Connection::handle() const
{
	return d->handle();
}
