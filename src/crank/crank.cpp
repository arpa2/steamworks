/*
Copyright (c) 2014, 2015 InternetWide.org and the ARPA2.net project
All rights reserved. See file LICENSE for exact terms (2-clause BSD license).

Adriaan de Groot <groot@kde.org>
*/

#include "crank.h"

#include "swldap/search.h"
#include "logger.h"

class CrankDispatcher::Private
{
friend class CrankDispatcher;
private:
	std::unique_ptr<Steamworks::LDAP::Connection> connection;

public:
	Private() :
		connection(nullptr)
	{
	}

	~Private()
	{
	}
} ;

CrankDispatcher::CrankDispatcher() :
	m_state(disconnected),
	d(new CrankDispatcher::Private())
{
}

int CrankDispatcher::exec(const std::string& verb, const Values values, Object response)
{
	if (verb == "connect") return do_connect(values);
	else if (verb == "stop") return do_stop(values);
	else if (verb == "search") return do_search(values, response);
	else if (verb == "update") return do_update(values, response);
	return -1;
}

int CrankDispatcher::do_connect(const Values values)
{
	std::string name = values.get("uri").to_str();

	Steamworks::Logging::Logger& log = Steamworks::Logging::getLogger("steamworks.crank");
	log.debugStream() << "Connecting to " << name;

	d->connection.reset(new Steamworks::LDAP::Connection(name));
	if (d->connection->is_valid())
	{
		m_state = connected;
	}
	return 0;
}

int CrankDispatcher::do_stop(const Values values)
{
	m_state = stopped;
	d->connection.reset(nullptr);
	return -1;
}

int CrankDispatcher::do_search(const Values values, Object response)
{
	Steamworks::Logging::Logger& log = Steamworks::Logging::getLogger("steamworks.crank");

	if (m_state != connected)
	{
		log.debugStream() << "Search on disconnected server.";
		return 0;
	}

	std::string base = values.get("base").to_str();
	std::string filter = values.get("filter").to_str();

	log.debugStream() << "Search parameter base=" << base;
	log.debugStream() << "Search         filter=" << filter;

	// TODO: check authorization for this query
	Steamworks::LDAP::Search search(base, filter);
	search.execute(*d->connection, &response);
	return 0;
}


int CrankDispatcher::do_update(const Values values, Object response)
{
#if 0
	Steamworks::Logging::Logger& log = Steamworks::Logging::getLogger("steamworks.crank");

	if (m_state != connected)
	{
		log.debugStream() << "Update on disconnected server.";
		return 0;
	}

	picojson::value v = values.get("values");
	if (!v.is<picojson::array>())
	{
		log.debugStream() << "Update json is not an array of updates.";
		return 0;
	}

	for (unsigned int count = 0; ; count++)
	{
		log.debugStream() << "Updating #" << count;

		Steamworks::LDAP::Update u(v.get(count));
		if (u.is_valid())
		{
			d->connection->execute(u, &response);
		}
		else
		{
			break;
		}
	}
#endif
	return 0;
}
