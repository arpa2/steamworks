/*
Copyright (c) 2014-2016 InternetWide.org and the ARPA2.net project
All rights reserved. See file LICENSE for exact terms (2-clause BSD license).

Adriaan de Groot <groot@kde.org>
*/
#define LDAP_DEPRECATED 1
// TODO: use ldap_bind_sasl_s instead of ldap_simple_bind_s.
#include <ldap.h>
#undef LDAP_DEPRECATED

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
	std::string m_uri;
	std::string m_user, m_pass;

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
				ldap_unbind_ext(handle, nullptr, nullptr);
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
	Private(const std::string& uri, const std::string& user=std::string(), const std::string& pass=std::string()) :
		m_uri(uri),
		m_user(user),
		m_pass(pass),
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
				log.errorStream() << "Could not set LDAP protocol version 3. Error " << r;
				return;
			}
		}

		if (true)
		{
			int tls_require_cert = LDAP_OPT_X_TLS_NEVER;
			r = ldap_set_option(ldaphandle, LDAP_OPT_X_TLS_REQUIRE_CERT, &tls_require_cert);
			if (r)
			{
				log.warnStream() << "Could not ignore TLS certificate. Error " << r << ", " << ldap_err2string(r);
			}
			// But carry on ..
		}

		// TODO: checking that this works is tricky
		r = ldap_start_tls_s(ldaphandle, &serverctl, &clientctl);
		if (r)
		{
			log.errorStream() << "Could not start TLS. Error " << r << ", " << ldap_err2string(r);
		}
		if (r<0)
		{
			return;
		}

		if (!m_user.empty())
		{
			log.debugStream() << "Binding to LDAP server as user " << m_user;
			r = ldap_simple_bind_s(ldaphandle, m_user.c_str(), m_pass.c_str());
			if (r)
			{
				log.errorStream() << "Could not bind to server with username " << (m_user.empty() ? "<empty>" : m_user);
				log.errorStream() << "Error " << r << ", " << ldap_err2string(r);
				return;
			}
		}

		// From here on, we'll keep the LDAP connection.
		handle_disconnector.keep = true;
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

	std::string get_uri() const { return m_uri; }
} ;

Steamworks::LDAP::Connection::Connection(const std::string& uri) :
	d(new Private(uri))
{
}

Steamworks::LDAP::Connection::Connection(const std::string& uri, const std::string& user, const std::string& password) :
	d(new Private(uri, user, password))
{
}

Steamworks::LDAP::Connection::~Connection()
{
}

::LDAP* Steamworks::LDAP::Connection::handle() const { return d->handle(); }
::LDAPControl** Steamworks::LDAP::Connection::client_controls() const { return d->client_controls(); }
::LDAPControl** Steamworks::LDAP::Connection::server_controls() const { return d->server_controls(); }
bool Steamworks::LDAP::Connection::is_valid() const { return d->is_valid(); }
std::string Steamworks::LDAP::Connection::get_uri() const { return d->get_uri(); }

static bool _do_connect(Steamworks::LDAP::ConnectionUPtr& connection, const std::string& uri, const std::string& user, const std::string& password, Steamworks::JSON::Object response, Steamworks::Logging::Logger& log)
{
	if (uri.empty())
	{
		log.warnStream() << "No Server URI given to connect.";
		Steamworks::JSON::simple_output(response, 400, "No server given", LDAP_OPERATIONS_ERROR);
		return false;
	}
	log.debugStream() << "Connecting to " << uri;

	if (!(user.empty() || password.empty()))
	{
		log.debugStream() << "Authenticating to " << uri << " as " << user;
		connection.reset(new Steamworks::LDAP::Connection(uri, user, password));
	}
	else
	{
		log.debugStream() << "Using no authentication.";
		connection.reset(new Steamworks::LDAP::Connection(uri));
	}
	if (!connection->is_valid())
	{
		log.warnStream() << "Could not connect to " << uri;
		// Still return 0 because we don't want the FCGI to stop.
		Steamworks::JSON::simple_output(response, 404, "Could not connect to server", LDAP_OPERATIONS_ERROR);
		return false;
	}

	return true;
}

static inline std::string _get_parameter(Steamworks::JSON::Values values, const char* key)
{
	auto v = values.get(key);
	if (v.is<picojson::null>())
	{
		return std::string();
	}
	return v.to_str();
}

bool Steamworks::LDAP::do_connect(Steamworks::LDAP::ConnectionUPtr& connection, Steamworks::JSON::Values values, Steamworks::JSON::Object response, Steamworks::Logging::Logger& log)
{
	std::string uri = _get_parameter(values, "uri");
	std::string user = _get_parameter(values, "user");
	std::string password = _get_parameter(values, "password");

	return _do_connect(connection, uri, user, password, response, log);
}

bool Steamworks::LDAP::do_connect(Steamworks::LDAP::ConnectionUPtr& connection, const std::string& uri, Steamworks::JSON::Object response, Steamworks::Logging::Logger& log)
{
	return _do_connect(connection, uri, std::string(), std::string(), response, log);
}
