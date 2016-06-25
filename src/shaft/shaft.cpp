/*
Copyright (c) 2014, 2015 InternetWide.org and the ARPA2.net project
All rights reserved. See file LICENSE for exact terms (2-clause BSD license).

Adriaan de Groot <groot@kde.org>
*/

#include "shaft.h"

#include "swldap/connection.h"
#include "swldap/private.h"
#include "swldap/serverinfo.h"
#include "swldap/sync.h"

#include "jsonresponse.h"
#include "logger.h"

/**
 * Private data for the shaft.
 *
 * A shaft has one downstream -- where it writes data -- which is the usual
 * connection for a SteamWorks component. There may be zero or more upstreams,
 * which the shaft monitors via SyncRepl for changes. These changes are written
 * (possibly after a renaming) to the downstream.
 */
class ShaftDispatcher::Private
{
friend class ShaftDispatcher;
private:
	using ConnectionUPtr = std::unique_ptr<SteamWorks::LDAP::Connection>;
	ConnectionUPtr connection;
	std::vector<ConnectionUPtr> upstream;

public:
	Private() :
		connection(nullptr)
	{
	}

	~Private()
	{
	}
} ;

ShaftDispatcher::ShaftDispatcher() :
	m_state(disconnected),
	d(new ShaftDispatcher::Private())
{
}

int ShaftDispatcher::exec(const std::string& verb, const Values values, Object response)
{
	if (verb == "connect") return do_connect(values, response);
	else if (verb == "stop") return do_stop(values);
	else if (verb == "serverinfo") return do_serverinfo(values, response);
	else if (verb == "upstream") return do_upstream(values, response);
	return -1;
}

int ShaftDispatcher::do_connect(const Values values, Object response)
{
	SteamWorks::Logging::Logger& log = SteamWorks::Logging::getLogger("steamworks.shaft");
	std::string name = values.get("uri").to_str();

	if (SteamWorks::LDAP::do_connect(d->connection, name, response, log) &&
	    SteamWorks::LDAP::require_syncrepl(d->connection, response, log)
	)
	{
		m_state = connected;
	}
	// Always return 0 because we don't want the FCGI to stop.
	return 0;
}

int ShaftDispatcher::do_stop(const Values values)
{
	m_state = stopped;
	d->connection.reset(nullptr);
	return -1;
}

int ShaftDispatcher::do_serverinfo(const Values values, Object response)
{
	SteamWorks::Logging::Logger& log = SteamWorks::Logging::getLogger("steamworks.shaft");

	if (m_state != connected)
	{
		log.debugStream() << "ServerInfo on disconnected server.";
		return 0;
	}

	SteamWorks::LDAP::ServerControlInfo info;
	info.execute(*d->connection, &response);

	return 0;
}

int ShaftDispatcher::do_upstream(const VerbDispatcher::Values values, VerbDispatcher::Object response)
{
	SteamWorks::Logging::Logger& log = SteamWorks::Logging::getLogger("steamworks.shaft");

	if (m_state != connected)
	{
		log.debugStream() << "ServerInfo on disconnected server.";
		return 0;
	}

	std::string name(values.get("uri").to_str());
	if (name.empty())
	{
		log.warnStream() << "No upstream Server URI given to connect.";
		SteamWorks::JSON::simple_output(response, 400, "No upstream server given", LDAP_OPERATIONS_ERROR);
		return 0;
	}
	log.debugStream() << "Connecting to upstream " << name;

	std::string dn(values.get("base").to_str());
	if (dn.empty())
	{
		log.warnStream() << "No upstream DN to follow.";
		SteamWorks::JSON::simple_output(response, 400, "No upstream DN given", LDAP_OPERATIONS_ERROR);
		return 0;
	}

	d->upstream.emplace_back(new SteamWorks::LDAP::Connection(name));
	if (!d->upstream.back()->is_valid())
	{
		log.warnStream() << "Could not connect to upstream " << name;
		d->upstream.pop_back();
		return 0;
	}

	std::string filter(values.get("filter").to_str());

	// TODO: actually do something with the upstream
	SteamWorks::LDAP::SyncRepl r(dn, filter);
	r.execute(*d->connection, &response);
	return 0;
}
