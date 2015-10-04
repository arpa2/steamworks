/*
Copyright (c) 2014, 2015 InternetWide.org and the ARPA2.net project
All rights reserved. See file LICENSE for exact terms (2-clause BSD license).

Adriaan de Groot <groot@kde.org>
*/

#include "crank.h"
#include "ldapcpp.h"

CrankDispatcher::CrankDispatcher() :
	state(disconnected)
{
}

int CrankDispatcher::exec(const std::string& verb, const Values values)
{
	if (verb == "connect") return do_connect(values);
	else if (verb == "stop") return do_stop(values);
	return -1;
}

int CrankDispatcher::do_connect(const Values values)
{
	std::string name = values.get("uri").to_str();
	Steamworks::LDAPConnection ldap(name);
	return 0;
}

int CrankDispatcher::do_stop(const Values values)
{
	state = stopped;
	return -1;
}
