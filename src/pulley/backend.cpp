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
#define PULLEY_BACKEND_DIR "/usr/share/steamworks/pulleyback/"
#endif

static const char plugindir[] = PULLEY_BACKEND_DIR;

// static_assert(plugindir[0] == '/', "Backend must be absolute dir.");
// static_assert(plugindir[sizeof(plugindir)-1] == '/', "Backend must end in /.");

class BackEnd::Private
{
private:
	std::string m_name;
	bool m_valid;
	void *m_handle;

public:
	decltype(pulleyback_open)* m_pulleyback_open;
	decltype(pulleyback_close)* m_pulleyback_close;

public:
	Private(const std::string& name) :
		m_name(name),
		m_valid(false),
		m_handle(nullptr)
	{
		auto& log = SteamWorks::Logging::getLogger("steamworks.pulleyback");
		log.debugStream() << "Trying to load backend " << name;

#ifdef NO_SECURITY
		char soname[128];
		snprintf(soname, sizeof(soname), "%s", name.c_str());
#else
		assert(plugindir[0] == '/');
		assert(plugindir[sizeof(plugindir)-1] == '/');

		if (name.find('/'))
		{
			log.errorStream() << "  .. illegal / in backend-name.";
			return;
		}

		char soname[sizeof(plugindir) + 128];
		snprintf(soname, sizeof(soname), "%s%s.so", plugindir, name.c_str());
#endif

		m_handle = dlopen(soname, RTLD_NOW | RTLD_LOCAL);
		if (m_handle == nullptr)
		{
			log.errorStream() << "  .. could not load backend shared-object. " << dlerror();
			return;
		}

		m_pulleyback_open = reinterpret_cast<decltype(pulleyback_open)*>(dlfunc(m_handle, "pulleyback_open"));
		m_pulleyback_close= reinterpret_cast<decltype(pulleyback_close)*>(dlfunc(m_handle, "pulleyback_close"));

		if (!(m_pulleyback_close && m_pulleyback_open))
		{
			m_pulleyback_close = nullptr;
			m_pulleyback_open = nullptr;
			m_valid = false;
		}
		else
		{
			m_valid = true;
		}

		if (!m_valid)
		{
			dlclose(m_handle);
			m_handle = nullptr;
		}
	}

	~Private()
	{
		if (m_handle != nullptr)
		{
			dlclose(m_handle);
			m_handle = nullptr;
		}
		m_valid = false;
	}

	bool is_valid() const { return m_valid; }
} ;

BackEnd::BackEnd(const std::string& name) :
	d(new Private(name))
{
	auto& log = SteamWorks::Logging::getLogger("steamworks.pulleyback");
	log.debugStream() << "Loaded " << name << " valid?" << d->is_valid();
	log.debugStream() << "  .. open @" << (void *)d->m_pulleyback_open << " close @" << (void *)d->m_pulleyback_close;
}

BackEnd::~BackEnd()
{

}

void* BackEnd::open(int argc, char** argv, int varc)
{
	if (!d->is_valid())
	{
		return nullptr;
	}
	else
	{
		return d->m_pulleyback_open(argc, argv, varc);
	}
}

void BackEnd::close(void* handle)
{
	if (d->is_valid())
	{
		d->m_pulleyback_close(handle);
	}
}
