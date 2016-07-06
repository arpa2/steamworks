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

/**
 * Loader for named Pulley backends.
 */
class BackEnd
{
private:
	class Private;
	std::unique_ptr<Private> d;

public:
	BackEnd(const std::string& name);
	~BackEnd();

	/**
	 * The following functions correspond with the pulleyback_*()
	 * functions in pulleyback.h. Calling one of these functions
	 * will call the corresponding function in the plugin which
	 * is loaded by this backend-object.
	 *
	 * If the backend has failed to load, calling any of these
	 * functions will return NULL or a suitable error value.
	 */
	void *open(int argc, char **argv, int varc);
	void close(void *);
} ;

#endif
