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
	::LDAPControl** client_controls() { return &clientctl; }
	::LDAPControl** server_controls() { return &serverctl; }

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
		}
		if (r<0)
		{
			return;
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

::LDAP* Steamworks::LDAP::Connection::handle() const { return d->handle(); }
::LDAPControl** Steamworks::LDAP::Connection::client_controls() const { return d->client_controls(); }
::LDAPControl** Steamworks::LDAP::Connection::server_controls() const { return d->server_controls(); }
