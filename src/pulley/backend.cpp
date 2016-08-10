/*
Copyright (c) 2016 InternetWide.org and the ARPA2.net project
All rights reserved. See file LICENSE for exact terms (2-clause BSD license).

Adriaan de Groot <groot@kde.org>
*/

#include "backend.h"
#include "pulleyback.h"

#include <assert.h>
#include <dlfcn.h>
#include <stdio.h>

#include <string>

#include "logger.h"

#ifndef PULLEY_BACKEND_DIR
#ifndef PREFIX
#define PREFIX "/usr/local"
#endif
#define PULLEY_BACKEND_DIR PREFIX "/share/steamworks/pulleyback/"
#endif

static const char plugindir[] = PULLEY_BACKEND_DIR;

static_assert(sizeof(plugindir) > 1, "Prefix / plugin directory path is weird.");

// static_assert(plugindir[0] == '/', "Backend must be absolute dir.");
// static_assert(plugindir[sizeof(plugindir)-1] == '/', "Backend must end in /.");


SteamWorks::PulleyBack::Parameters::Parameters(const std::vector<std::string>& expressions) :
	varc(0),
	argc(expressions.size()),
	argv(nullptr)
{
	if (argc > 0)
	{
		argv = (char **)calloc(argc, sizeof(char *));
	}
	if (argv)
	{
		unsigned int i = 0;
		for (const auto& s : expressions)
		{
			argv[i++] = strdup(s.c_str());
		}
	}
}

SteamWorks::PulleyBack::Parameters::~Parameters()
{
	if (argv)
	{
		for (unsigned int i=0; i < argc; i++)
		{
			free(argv[i]);
		}
		free(argv);
		argv = nullptr;
	}
}


/**
 * Helper class when loading the API from a DLL. Uses dlfunc()
 * to resolve functions and stores them in referenced function
 * pointers. If any of the name-resolvings fails, the referenced
 * boolean valid is set to false. Use multiple FuncKeepers
 * all referencing the same valid bool to load an entire
 * API and check if it's valid.
 */
template<typename funcptr> class FuncKeeper {
private:
	bool& m_valid;
	funcptr*& m_func;
public:
	FuncKeeper(void *handle, const char *name, bool& valid, funcptr*& func) :
		m_valid(valid),
		m_func(func)
	{
		dlfunc_t f = dlfunc(handle, name);
		if (f)
		{
			func = reinterpret_cast<funcptr*>(f);
		}
		else
		{
			func = nullptr;
			auto& log = SteamWorks::Logging::getLogger("steamworks.pulleyback");
			log.warnStream() << "Function " << name << " not found.";
			valid = false;
		}
	}

	~FuncKeeper()
	{
		if (!m_valid)
		{
			m_func = nullptr;
		}
	}
} ;


class SteamWorks::PulleyBack::Loader::Private
{
private:
	std::string m_name;
	bool m_valid;
	void *m_handle;

public:
	decltype(pulleyback_open)* m_pulleyback_open;
	decltype(pulleyback_close)* m_pulleyback_close;
	decltype(pulleyback_add)* m_pulleyback_add;
	decltype(pulleyback_del)* m_pulleyback_del;
	decltype(pulleyback_reset)* m_pulleyback_reset;
	decltype(pulleyback_prepare)* m_pulleyback_prepare;  // OPTIONAL
	decltype(pulleyback_commit)* m_pulleyback_commit;
	decltype(pulleyback_rollback)* m_pulleyback_rollback;
	decltype(pulleyback_collaborate) *m_pulleyback_collaborate;

public:
	Private(const std::string& name) :
		m_name(name),
		m_valid(false),
		m_handle(nullptr)
	{
		auto& log = SteamWorks::Logging::getLogger("steamworks.pulleyback");
		log.debugStream() << "Trying to load backend '" << name << '\'';

#ifdef NO_SECURITY
		char soname[128];
		snprintf(soname, sizeof(soname), "%s", name.c_str());
#else
		assert(plugindir[0] == '/');
		assert(plugindir[sizeof(plugindir)-2] == '/');  // -1 is the NUL, -2 is trailing /

		if (name.find('/') != std::string::npos)
		{
			log.errorStream() << "  .. illegal / in backend-name.";
			return;
		}

		char soname[sizeof(plugindir) + 128];
		snprintf(soname, sizeof(soname), "%spulleyback_%s.so", plugindir, name.c_str());
#endif

		soname[sizeof(soname)-1] = 0;
		log.debugStream() << "Trying to load '" << soname << '\'';

		m_handle = dlopen(soname, RTLD_NOW | RTLD_LOCAL);
		if (m_handle == nullptr)
		{
			log.errorStream() << "  .. could not load backend shared-object. " << dlerror();
			return;
		}

		m_valid = true;
		// As far as we know .. the FuncKeepers can modify it. Put them in a block
		// so that their destructors have run before we (possibly) dlclose the
		// shared library their function-pointers point into.
		{
			bool optionalvalid = true;

			FuncKeeper<decltype(pulleyback_open)> fk0(m_handle, "pulleyback_open", m_valid, m_pulleyback_open);
			FuncKeeper<decltype(pulleyback_close)> fk1(m_handle, "pulleyback_close", m_valid, m_pulleyback_close);
			FuncKeeper<decltype(pulleyback_add)> fk2(m_handle, "pulleyback_add", m_valid, m_pulleyback_add);
			FuncKeeper<decltype(pulleyback_del)> fk3(m_handle, "pulleyback_del", m_valid, m_pulleyback_del);
			FuncKeeper<decltype(pulleyback_reset)> fk4(m_handle, "pulleyback_reset", m_valid, m_pulleyback_reset);
			FuncKeeper<decltype(pulleyback_prepare)> fk5(m_handle, "pulleyback_prepare", optionalvalid, m_pulleyback_prepare);
			FuncKeeper<decltype(pulleyback_commit)> fk6(m_handle, "pulleyback_commit", m_valid, m_pulleyback_commit);
			FuncKeeper<decltype(pulleyback_rollback)> fk7(m_handle, "pulleyback_rollback", m_valid, m_pulleyback_rollback);
			FuncKeeper<decltype(pulleyback_collaborate)> fk8(m_handle, "pulleyback_collaborate", m_valid, m_pulleyback_collaborate);

			optionalvalid &= m_valid;  // If any required func missing, the optionals are invalid too
		}

		// If any function has not been resolved, the FuncKeeper will have set m_valid to false
		if (!m_valid)
		{
			dlclose(m_handle);
			m_handle = nullptr;
		}
	}

	~Private()
	{
		auto& log = SteamWorks::Logging::getLogger("steamworks.pulleyback");
		log.debugStream() << "Unloaded priv " << name();

		if (m_handle != nullptr)
		{
			dlclose(m_handle);
			m_handle = nullptr;
		}
		m_valid = false;
	}

	bool is_valid() const { return m_valid; }
	std::string name() const { return m_name; }
} ;

SteamWorks::PulleyBack::Loader::Loader(const std::string& name) :
	d(new Private(name))
{
	auto& log = SteamWorks::Logging::getLogger("steamworks.pulleyback");
	log.debugStream() << "Loaded " << name << " valid? " << (d->is_valid() ? "yes" : "no");
	log.debugStream() << "  .. open @" << (void *)d->m_pulleyback_open << " close @" << (void *)d->m_pulleyback_close;
}

SteamWorks::PulleyBack::Loader::~Loader()
{
	auto& log = SteamWorks::Logging::getLogger("steamworks.pulleyback");
	log.debugStream() << "Unloaded " << d->name();
}

SteamWorks::PulleyBack::Instance SteamWorks::PulleyBack::Loader::get_instance(int argc, char** argv, int varc)
{
	return Instance(d, argc, argv, varc);
}

SteamWorks::PulleyBack::Instance SteamWorks::PulleyBack::Loader::get_instance(SteamWorks::PulleyBack::Parameters& parameters)
{
	return Instance(d, parameters.argc, parameters.argv, parameters.varc);
}



SteamWorks::PulleyBack::Instance::Instance(std::shared_ptr<Loader::Private>& parent_d, int argc, char** argv, int varc) :
	d(parent_d),
	m_handle(nullptr)
{
	if (d->is_valid())
	{
		m_handle = d->m_pulleyback_open(argc, argv, varc);
	}
}

SteamWorks::PulleyBack::Instance::~Instance()
{
	if (m_handle and d->is_valid())
	{
		d->m_pulleyback_close(m_handle);
	}
}
