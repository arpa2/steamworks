/*
Copyright (c) 2014, 2015 InternetWide.org and the ARPA2.net project
All rights reserved. See file LICENSE for exact terms (2-clause BSD license).

Adriaan de Groot <groot@kde.org>
*/

#ifndef STEAMWORKS_VERB_H
#define STEAMWORKS_VERB_H

#include "picojson.h"

class VerbDispatcher
{
public:
	typedef picojson::value& Values;

	virtual int exec(const std::string& verb, const Values values) = 0;
} ;

#endif
