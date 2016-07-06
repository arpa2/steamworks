#include "backend.h"

#include <logger.h>

#include <stdio.h>

static const char progname[] = "PulleyScript Test";
static const char version[] = "v0.1";
static const char copyright[] = "Copyright (C) 2014-2016 InternetWide.org and the ARPA2.net project";

static void version_info()
{
	printf("SteamWorks %s %s\n", progname, version);
	printf("%s\n", copyright);
}



int main(int argc, char **argv)
{
	SteamWorks::Logging::Manager logManager("pulleyscript.properties", SteamWorks::Logging::DEBUG);
	SteamWorks::Logging::getRoot().debugStream() << "SteamWorks " << progname << ' ' << copyright;

	auto& log = SteamWorks::Logging::getLogger("steamworks.pulleyscript");

	BackEnd* b = new BackEnd(argv[1]);

	void *p = b->open(1, argv, 0);
	b->close(p);
	delete b;
}
