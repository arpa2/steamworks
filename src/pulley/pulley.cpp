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
	Steamworks::Logging::Logger& log = Steamworks::Logging::getLogger("steamworks.pulley");
	std::string name = values.get("uri").to_str();

	if (Steamworks::LDAP::do_connect(d->connection, name, response, log) &&
	    Steamworks::LDAP::require_syncrepl(d->connection, response, log)
	)
	{
		m_state = connected;
	}
	// Always return 0 because we don't want the FCGI to stop.
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
