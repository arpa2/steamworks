/*
Copyright (c) 2014, 2015 InternetWide.org and the ARPA2.net project
All rights reserved. See file LICENSE for exact terms (2-clause BSD license).

Adriaan de Groot <groot@kde.org>
*/

#include "pulley.h"

#include "swldap/connection.h"
#include "swldap/serverinfo.h"
#include "swldap/sync.h"

#include "jsonresponse.h"
#include "logger.h"

class PulleyDispatcher::Private
{
friend class PulleyDispatcher;
private:
	using ConnectionUPtr = std::unique_ptr<Steamworks::LDAP::Connection>;
	ConnectionUPtr connection;
	std::vector<ConnectionUPtr> downstream;

public:
	Private() :
		connection(nullptr)
	{
	}

	~Private()
	{
	}
} ;

PulleyDispatcher::PulleyDispatcher() :
	m_state(disconnected),
	d(new PulleyDispatcher::Private())
{
}

int PulleyDispatcher::exec(const std::string& verb, const Values values, Object response)
{
	if (verb == "connect") return do_connect(values, response);
	else if (verb == "stop") return do_stop(values);
	else if (verb == "serverinfo") return do_serverinfo(values, response);
	return -1;
}

int PulleyDispatcher::do_connect(const Values values, Object response)
{
	std::string name = values.get("uri").to_str();

	Steamworks::Logging::Logger& log = Steamworks::Logging::getLogger("steamworks.pulley");
	if (name.empty())
	{
		log.warnStream() << "No Server URI given to connect.";
		Steamworks::JSON::simple_output(response, 400, "No server given", LDAP_OPERATIONS_ERROR);
		return 0;
	}
	log.debugStream() << "Connecting to " << name;

	d->connection.reset(new Steamworks::LDAP::Connection(name));
	if (d->connection->is_valid())
	{
		m_state = connected;
		Steamworks::LDAP::ServerControlInfo info;
		info.execute(*d->connection);

		if (!info.is_available(info.SC_SYNC))
		{
			m_state = disconnected;
			d->connection.reset(nullptr);
			log.warnStream() << "Server at " << name << " does not support SyncRepl.";
			Steamworks::JSON::simple_output(response, 501, "Server does not support SyncRepl", LDAP_UNAVAILABLE_CRITICAL_EXTENSION);
			return 0;
		}
	}
	else
	{
		log.warnStream() << "Could not connect to " << name;
		// Still return 0 because we don't want the FCGI to stop.
		Steamworks::JSON::simple_output(response, 404, "Could not connect to server", LDAP_OPERATIONS_ERROR);
		return 0;
	}
	return 0;
}

int PulleyDispatcher::do_stop(const Values values)
{
	m_state = stopped;
	d->connection.reset(nullptr);
	return -1;
}

int PulleyDispatcher::do_serverinfo(const Values values, Object response)
{
	Steamworks::Logging::Logger& log = Steamworks::Logging::getLogger("steamworks.pulley");

	if (m_state != connected)
	{
		log.debugStream() << "ServerInfo on disconnected server.";
		return 0;
	}

	Steamworks::LDAP::ServerControlInfo info;
	info.execute(*d->connection, &response);

	return 0;
}
