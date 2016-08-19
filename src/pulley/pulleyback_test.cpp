#include "pulleyscript/backend.h"

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

	std::vector<std::shared_ptr<SteamWorks::PulleyBack::Loader> > backends;

	for (int c=1; c < argc; c++)
	{
		backends.emplace_back(new SteamWorks::PulleyBack::Loader(argv[c]));
		auto& plugin = backends.back();
		{
			auto b(plugin->get_instance(1, argv, 0));
			if (!b.is_valid())
			{
				log.errorStream() << "Could not load plugin " << argv[c];
				return 1;
			}
		}
	}

	return 0;
}
