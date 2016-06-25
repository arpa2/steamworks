/*
Copyright (c) 2014, 2015 InternetWide.org and the ARPA2.net project
All rights reserved. See file LICENSE for exact terms (2-clause BSD license).

Adriaan de Groot <groot@kde.org>
*/

#include "crank.h"

#include "swldap/search.h"
#include "swldap/serverinfo.h"

#include "jsonresponse.h"
#include "logger.h"

class CrankDispatcher::Private
{
friend class CrankDispatcher;
private:
	std::unique_ptr<SteamWorks::LDAP::Connection> connection;

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
	if (verb == "connect") return do_connect(values, response);
	else if (verb == "stop") return do_stop(values);
	else if (verb == "search") return do_search(values, response);
	else if (verb == "typeinfo") return do_typeinfo(values, response);
	else if (verb == "update") return do_update(values, response);
	else if (verb == "delete") return do_delete(values, response);
	else if (verb == "add") return do_add(values, response);
	else if (verb == "serverinfo") return do_serverinfo(values, response);
	else if (verb == "serverstatus") return do_serverstatus(values, response);
	else {
		std::string s("Unknown verb '");
		s.append(verb);
		s.append("', ignored.");
		SteamWorks::JSON::simple_output(response, 500, s.c_str());
		return 0;
	}
}

int CrankDispatcher::do_connect(const Values values, Object response)
{
	SteamWorks::Logging::Logger& log = SteamWorks::Logging::getLogger("steamworks.crank");
	if (SteamWorks::LDAP::do_connect(d->connection, values, response, log))
	{
		m_state = connected;
	}
	// Always return 0 because we don't want the FCGI to stop.
	return 0;
}

int CrankDispatcher::do_stop(const Values values)
{
	m_state = stopped;
	d->connection.reset(nullptr);
	return -1;
}

void _serverstatus(VerbDispatcher::Object response, int status, const char* statusname, const char* message)
{
	picojson::value v(statusname);
	response.emplace("status", v);

	if (!message)
	{
		char buffer[64];
		snprintf(buffer, sizeof(buffer), "Unknown status %d", status);
		v = picojson::value(buffer);
		response.emplace("message", v);
	}
	else
	{
		v = picojson::value(message);
		response.emplace("message", v);
	}

	v = picojson::value((double)status);
	response.emplace("_status", v);
}

int CrankDispatcher::do_serverstatus(const VerbDispatcher::Values values, VerbDispatcher::Object response)
{
	switch(m_state)
	{
		case stopped:
			_serverstatus(response, m_state, "stopped", "The Crank has stopped and is disconnected from the server.");
			break;
		case connected:
			_serverstatus(response, m_state, "connected", "The Crank is connected to the server.");
			break;
		default:
			_serverstatus(response, m_state, "unknown", nullptr);
	}
	return 0;
}

int CrankDispatcher::do_search(const Values values, Object response)
{
	SteamWorks::Logging::Logger& log = SteamWorks::Logging::getLogger("steamworks.crank");

	if (m_state != connected)
	{
		log.debugStream() << "Search on disconnected server.";
		return 0;
	}

	std::string base = values.get("base").to_string();
	std::string filter = values.get("filter").to_string();

	log.debugStream() << "Search parameter base=" << base;
	log.debugStream() << "Search         filter=" << filter;

	// TODO: check authorization for this query
	SteamWorks::LDAP::Search search(base, filter);
	search.execute(*d->connection, &response);
	return 0;
}

int CrankDispatcher::do_typeinfo(const Values values, Object response)
{
	SteamWorks::Logging::Logger& log = SteamWorks::Logging::getLogger("steamworks.crank");

	if (m_state != connected)
	{
		log.debugStream() << "Typeinfo on disconnected server.";
		return 0;
	}

	SteamWorks::LDAP::TypeInfo search;
	search.execute(*d->connection, &response);
	return 0;
}



int CrankDispatcher::do_update(const Values values, Object response)
{
	SteamWorks::Logging::Logger& log = SteamWorks::Logging::getLogger("steamworks.crank");

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

		SteamWorks::LDAP::Update u(v.get(count));
		if (u.is_valid())
		{
			u.execute(*d->connection, &response);
			log.debugStream() << "Completed #" << count;
		}
		else
		{
			break;
		}
	}
	return 0;
}

int CrankDispatcher::do_delete(const Values values, Object response)
{
	SteamWorks::Logging::Logger& log = SteamWorks::Logging::getLogger("steamworks.crank");

	if (m_state != connected)
	{
		log.debugStream() << "Update on disconnected server.";
		return 0;
	}

	std::string dn = values.get("dn").to_string();
	if (dn.empty())
	{
		log.debugStream() << "Can't delete with an empty DN.";
		// TODO: error in the response object?
		return 0;
	}

	SteamWorks::LDAP::Remove r(dn);
	if (r.is_valid())
	{
		r.execute(*d->connection, &response);
	}
	return 0;
}

int CrankDispatcher::do_add(const VerbDispatcher::Values values, VerbDispatcher::Object response)
{
	SteamWorks::Logging::Logger& log = SteamWorks::Logging::getLogger("steamworks.crank");

	if (m_state != connected)
	{
		log.debugStream() << "Add on disconnected server.";
		return 0;
	}

	picojson::value v = values.get("values");
	if (!v.is<picojson::array>())
	{
		log.debugStream() << "Add json is not an array of additions.";
		return 0;
	}

	for (unsigned int count = 0; ; count++)
	{
		log.debugStream() << "Add #" << count;

		SteamWorks::LDAP::Addition u(v.get(count));
		if (u.is_valid())
		{
			u.execute(*d->connection, &response);
			log.debugStream() << "Completed #" << count;
		}
		else
		{
			break;
		}
	}
	return 0;


}



int CrankDispatcher::do_serverinfo(const Values values, Object response)
{
	SteamWorks::Logging::Logger& log = SteamWorks::Logging::getLogger("steamworks.crank");

	if (m_state != connected)
	{
		log.debugStream() << "ServerInfo on disconnected server.";
		return 0;
	}

	SteamWorks::LDAP::ServerControlInfo info;
	info.execute(*d->connection, &response);

	return 0;
}
