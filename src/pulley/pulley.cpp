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

	using SyncReplUPtr = std::unique_ptr<Steamworks::LDAP::SyncRepl>;
	std::vector<SyncReplUPtr> following;

public:
	Private() :
		connection(nullptr)
	{
	}

	~Private()
	{
	}

	int add_follower(const std::string& base, const std::string& filter, Object response)
	{
		following.emplace_back(new Steamworks::LDAP::SyncRepl(base, filter));
		following.back()->execute(*connection, &response);
		return 0;
	}

	void dump_followers(Object response)
	{
		for(auto& f : following)
		{
			f->dump_dit(&response);
		}
	}

	void resync()
	{
		for(auto& f : following)
		{
			f->resync();
		}
	}
} ;

PulleyDispatcher::PulleyDispatcher() :
	m_state(disconnected),
	d(new PulleyDispatcher::Private())
{
}

void PulleyDispatcher::poll()
{
	VerbDispatcher::poll();

	for (auto i=d->following.cbegin(); i!=d->following.cend(); ++i)
	{
		(*i)->poll(*d->connection);
	}
}


int PulleyDispatcher::exec(const std::string& verb, const Values values, Object response)
{
	if (verb == "connect") return do_connect(values, response);
	else if (verb == "stop") return do_stop(values);
	else if (verb == "serverinfo") return do_serverinfo(values, response);
	else if (verb == "follow") return do_follow(values, response);
	else if (verb == "unfollow") return do_unfollow(values, response);
	else if (verb == "dump_dit") return do_dump_dit(values, response);
	else if (verb == "resync") return do_resync(values, response);
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

static inline std::string _get_parameter(Steamworks::JSON::Values values, const char* key)
{
	auto v = values.get(key);
	if (v.is<picojson::null>())
	{
		return std::string();
	}
	return v.to_str();
}

int PulleyDispatcher::do_follow(const VerbDispatcher::Values values, VerbDispatcher::Object response)
{
	Steamworks::Logging::Logger& log = Steamworks::Logging::getLogger("steamworks.pulley");

	if (m_state != connected)
	{
		log.debugStream() << "Follow on disconnected server.";
		return 0;
	}

	std::string base = _get_parameter(values, "base");
	std::string filter = _get_parameter(values, "filter");

	if (base.empty())
	{
		log.warnStream() << "No base given for follow.";
		return 0;
	}

	return d->add_follower(base, filter, response);
}

int PulleyDispatcher::do_unfollow(const VerbDispatcher::Values values, VerbDispatcher::Object response)
{
	Steamworks::Logging::Logger& log = Steamworks::Logging::getLogger("steamworks.pulley");

	if (m_state != connected)
	{
		log.debugStream() << "UnFollow on disconnected server.";
		return 0;
	}

	return 0;
}

int PulleyDispatcher::do_dump_dit(const VerbDispatcher::Values values, VerbDispatcher::Object response)
{
	d->dump_followers(response);
	return 0;
}

int PulleyDispatcher::do_resync(const VerbDispatcher::Values values, VerbDispatcher::Object response)
{
	d->resync();
	return 0;
}
