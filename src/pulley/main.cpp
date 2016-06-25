/*
Copyright (c) 2014-2016 InternetWide.org and the ARPA2.net project
All rights reserved. See file LICENSE for exact terms (2-clause BSD license).

Adriaan de Groot <groot@kde.org>
*/

#include "fcgi.h"
#include "logger.h"

#include "pulley.h"

static const char* copyright = "Copyright (C) 2014-2016 InternetWide.org and the ARPA2.net project";

int main(int argc, char** argv)
{
	SteamWorks::Logging::Manager logManager("pulley.properties");
	SteamWorks::Logging::getRoot().debugStream() << "Steamworks Pulley" << copyright;

	PulleyDispatcher* dispatcher = new PulleyDispatcher();
	SteamWorks::FCGI::init_logging("pulley.fcgi");
	SteamWorks::FCGI::mainloop(dispatcher);

	return 0;
}
