/*
Copyright (c) 2014, 2015 InternetWide.org and the ARPA2.net project
All rights reserved. See file LICENSE for exact terms (2-clause BSD license).

Adriaan de Groot <groot@kde.org>
*/

#ifndef STEAMWORKS_COMMON_VERB_H
#define STEAMWORKS_COMMON_VERB_H

#include "picojson.h"

struct fd_set;

class VerbDispatcher
{
public:
	typedef picojson::value& Values;
	typedef picojson::value::object& Object;


	virtual int exec(const std::string& verb, const Values values, Object response) = 0;

	/**
	 * If this dispatcher has anything to poll (regardless of
	 * select() on the file-descriptors it might be watching)
	 * then poll that.
	 *
	 * The default implementation does nothing.
	 */
	virtual void poll();
} ;

#endif
