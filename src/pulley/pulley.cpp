/*
Copyright (c) 2014, 2015 InternetWide.org and the ARPA2.net project
All rights reserved. See file LICENSE for exact terms (2-clause BSD license).

Adriaan de Groot <groot@kde.org>
*/

#include <forward_list>

#include "pulley.h"
#include "pulleyscript/parserpp.h"

#include "swldap/connection.h"
#include "swldap/serverinfo.h"
#include "swldap/sync.h"

#include "jsonresponse.h"
#include "logger.h"

class PulleySyncRepl : public SteamWorks::LDAP::SyncRepl
{
protected:
	std::shared_ptr<SteamWorks::PulleyScript::Parser> m_prs;

public:
	PulleySyncRepl(const std::string& base, const std::string& filter, std::shared_ptr<SteamWorks::PulleyScript::Parser> parser) :
		SyncRepl(base, filter),
		m_prs(parser)
	{
	}

protected:
	virtual void after_modification(const std::string& removed) override;
	virtual void after_modification(const std::string& modified, const picojson::object& values) override;
} ;

void PulleySyncRepl::after_modification(const std::string& removed)
{
	// SteamWorks::LDAP::SyncRepl::after_modification(removed);
	m_prs->remove_entry(removed);
}

void PulleySyncRepl::after_modification(const std::string& modified, const picojson::object& values)
{
	// SteamWorks::LDAP::SyncRepl::after_modification(modified, values);
	m_prs->remove_entry(modified);
	m_prs->add_entry(modified, values);
}


class PulleyDispatcher::Private
{
friend class PulleyDispatcher;
private:
	using ConnectionUPtr = std::unique_ptr<SteamWorks::LDAP::Connection>;
	ConnectionUPtr m_connection;

	using SyncReplUPtr = std::unique_ptr<PulleySyncRepl>;
	std::forward_list<SyncReplUPtr> m_following;

	using ParserSPtr = std::shared_ptr<SteamWorks::PulleyScript::Parser>;
	ParserSPtr m_parser;

public:
	Private() :
		m_connection(nullptr),
		m_parser(nullptr)
	{
	}

	~Private()
	{
	}

	int add_follower(const std::string& base, const std::string& filter, Object response)
	{
		m_following.emplace_front(new PulleySyncRepl(base, filter, m_parser));
		m_following.front()->execute(*m_connection, &response);
		return 0;
	}

	int remove_follower(const std::string& base, const std::string& filter)
	{
		SteamWorks::Logging::Logger& log = SteamWorks::Logging::getLogger("steamworks.pulley");
		log.debugStream() << "Looking for SyncRepl base=" << base << " filter=" << filter;

#ifndef NDEBUG
		for(auto it = m_following.cbegin(); it != m_following.cend(); it++)
		{
			auto& f = *it;
			log.debugStream() << "  .. SyncRepl @" << (void *)f.get() << " base=" << f->base() << " filter=" << f->filter();
		}
#endif

		m_following.remove_if([&](const SyncReplUPtr& f) { return (f->base() == base) && (f->filter() == filter); });
		return 0;
	}

	void dump_followers(Object response)
	{
		for(auto& f : m_following)
		{
			f->dump_dit(&response);
		}
	}

	void resync_followers()
	{
		for(auto& f : m_following)
		{
			f->resync();
		}
	}

	unsigned int count_followers()
	{
		return std::distance(m_following.cbegin(), m_following.cend());
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

	for (auto i=d->m_following.cbegin(); i!=d->m_following.cend(); ++i)
	{
		(*i)->poll(*d->m_connection);
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
	else if (verb == "script") return do_script(values, response);
	return -1;
}

int PulleyDispatcher::do_connect(const Values values, Object response)
{
	SteamWorks::Logging::Logger& log = SteamWorks::Logging::getLogger("steamworks.pulley");
	std::string name = values.get("uri").to_str();

	if (SteamWorks::LDAP::do_connect(d->m_connection, name, response, log) &&
	    SteamWorks::LDAP::require_syncrepl(d->m_connection, response, log)
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
	d->m_connection.reset(nullptr);
	return -1;
}

int PulleyDispatcher::do_serverinfo(const Values values, Object response)
{
	SteamWorks::Logging::Logger& log = SteamWorks::Logging::getLogger("steamworks.pulley");

	if (m_state != connected)
	{
		log.debugStream() << "ServerInfo on disconnected server.";
		return 0;
	}

	SteamWorks::LDAP::ServerControlInfo info;
	info.execute(*d->m_connection, &response);

	return 0;
}

static inline std::string _get_parameter(SteamWorks::JSON::Values values, const char* key)
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
	SteamWorks::Logging::Logger& log = SteamWorks::Logging::getLogger("steamworks.pulley");

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
	if (filter.empty())
	{
		log.warnStream() << "No filter given for follow.";
		return 0;
	}

	return d->add_follower(base, filter, response);
}

int PulleyDispatcher::do_unfollow(const VerbDispatcher::Values values, VerbDispatcher::Object response)
{
	SteamWorks::Logging::Logger& log = SteamWorks::Logging::getLogger("steamworks.pulley");

	if (m_state != connected)
	{
		log.debugStream() << "UnFollow on disconnected server.";
		return 0;
	}

	std::string base = _get_parameter(values, "base");
	std::string filter = _get_parameter(values, "filter");

	if (base.empty())
	{
		log.warnStream() << "No base given for unfollow.";
		return 0;
	}
	if (filter.empty())
	{
		log.warnStream() << "No filter given for unfollow.";
		return 0;
	}

	log.debugStream() << "Unfollowing base=" << base << " filter=" << filter;
	return d->remove_follower(base, filter);
}

int PulleyDispatcher::do_dump_dit(const VerbDispatcher::Values values, VerbDispatcher::Object response)
{
	d->dump_followers(response);
	return 0;
}

int PulleyDispatcher::do_resync(const VerbDispatcher::Values values, VerbDispatcher::Object response)
{
	d->resync_followers();
	return 0;
}

int PulleyDispatcher::do_script(const char* filename)
{
	auto& log = SteamWorks::Logging::getLogger("steamworks.pulley");

	// TODO: cleanups when changing scripts?
	if (d->count_followers())
	{
		log.errorStream() << "Cannot load a PulleyScript when connected to LDAP server.";
		return 1;
	}

	d->m_parser.reset(new SteamWorks::PulleyScript::Parser());
	if (d->m_parser->read_file(filename))
	{
		log.warnStream() << "Parser error on reading " << filename;
		return 1;
	}

	d->m_parser->structural_analysis();
	d->m_parser->setup_sql();

	return 0;
}

int PulleyDispatcher::do_script(const VerbDispatcher::Values values, VerbDispatcher::Object response)
{
	auto& log = SteamWorks::Logging::getLogger("steamworks.pulley");

	std::string filename = _get_parameter(values, "filename");
	if (filename.empty())
	{
		log.warnStream() << "No filename given.";
		return 0;
	}
	if (d->count_followers())
	{
		log.warnStream() << "Can't read script file after connecting to server.";
		return 0;
	}

	log.debugStream() << "Reading script from '" << filename << '\'';
	auto r = do_script(filename.c_str());

	if (r)
	{
		// Still return 0, because we want the pulley to continue.
		// TODO: error reporting through @p response
		return 0;
	}

	std::string base = _get_parameter(values, "base");
	bool autofollow = false;
	auto a = values.get("autofollow");
	if (!a.is<picojson::null>())
	{
		autofollow = a.is<bool>() && a.get<bool>();
	}

	if (autofollow && base.empty())
	{
		log.warnStream() << "Pulleyscript autoload is on, but no base is set.";
	}
	else if (autofollow)
	{
		auto synclist = d->m_parser->find_subscriptions();
		log.debugStream() << "Pulleyscript adding " << std::distance(synclist.cbegin(), synclist.cend()) << " subscriptions.";
		for (auto filter : synclist)
		{
			d->add_follower(base, filter, response);
		}
	}
	else
	{
		log.debugStream() << "Pulleyscript loaded, no autofollow.";
	}

	return 0;
}
