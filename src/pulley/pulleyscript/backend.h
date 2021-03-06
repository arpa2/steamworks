/*
Copyright (c) 2016 InternetWide.org and the ARPA2.net project
All rights reserved. See file LICENSE for exact terms (2-clause BSD license).

Adriaan de Groot <groot@kde.org>
*/

/**
 * Pulley backends are dynamic libraries, loaded through instantiation
 * of the BackEnd class. The library must implement the PulleyBack
 * API (documented elsewhere). The backends are loaded in response
 * to PulleyScripts which output to those backends. A Pulley PulleyScript
 * that outputs like this:
 *
 *     var -> backendname(...)
 *
 * will cause the backend named "backendname" to be loaded (from
 * libbackendname.so) and its output-API used to write the variable.
 */

#ifndef STEAMWORKS_PULLEY_BACKEND_H
#define STEAMWORKS_PULLEY_BACKEND_H

#include <vector>
#include <memory>

#include "../pulleyback.h"

namespace SteamWorks
{

// Forward declare; see pulleyscript/parserpp.h
namespace PulleyScript { class BackendParameters; }

namespace PulleyBack
{

class Instance;

/**
 * Loader for named Pulley backends.
 */
class Loader
{
friend class Instance;
private:
	class Private;
	std::shared_ptr<Private> d;  // Shared with instances

public:
	Loader(const std::string& name);
	~Loader();

	/**
	 * Gets an (open) instance of a plugin. This calls
	 * pulleyback_open() on the plugin and returns an
	 * Instance which can be used to call the other
	 * API functions of the plugin.
	 */
	Instance get_instance(int argc, char** argv, int varc);
	/**
	 * See above, using a Parameters instance instead.
	 */
	Instance get_instance(PulleyScript::BackendParameters& parameters);

	bool is_valid() const;
} ;


class Instance
{
friend class Loader;
private:
	std::shared_ptr<Loader::Private> d;  // Shared with loaders
	void* m_handle;

	Instance(std::shared_ptr<Loader::Private>& loader, int argc, char** argv, int varc);

public:
	/**
	 * Close this instance of a Pulley backend-plugin api.
	 */
	~Instance();

	/**
	 * The following functions correspond with the pulleyback_*()
	 * functions in pulleyback.h. Calling one of these functions
	 * will call the corresponding function in the plugin which
	 * is loaded by this backend-object.
	 *
	 * If the backend has failed to load, calling any of these
	 * functions will return NULL or a suitable error value.
	 */

	bool is_valid() const;
	std::string name() const;

	int add(der_t* forkdata);
	int del(der_t* forkdata);
	int prepare();
	int commit();
	void rollback();
} ;

}  // namespace PulleyBack
}  // namespace

#endif
