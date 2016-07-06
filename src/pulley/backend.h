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

#include <memory>

namespace SteamWorks
{

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
	std::shared_ptr<Private> d;

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
} ;


class Instance
{
friend class Loader;
private:
	std::shared_ptr<Loader::Private> d;
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
} ;

}  // namespace PulleyBack
}  // namespace

#endif
