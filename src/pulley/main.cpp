/*
Copyright (c) 2014-2016 InternetWide.org and the ARPA2.net project
All rights reserved. See file LICENSE for exact terms (2-clause BSD license).

Adriaan de Groot <groot@kde.org>
*/

#include <getopt.h>

#include "fcgi.h"
#include "logger.h"

#include "pulley.h"

#include "pulleyscript/parserpp.h"

static const char progname[] = "PulleyScript";
static const char version[] = "v0.1";
static const char copyright[] = "Copyright (C) 2014-2016 InternetWide.org and the ARPA2.net project";

static void version_info()
{
	printf("SteamWorks %s %s\n", progname, version);
	printf("%s\n", copyright);
}

static void version_usage()
{
	printf(R"(
Usage:
    pulley [-L libdir] scriptfile [...]
\n\n)");
}

static void version_help()
{
	version_info();
	printf(R"(
Use the Pulley to output configuration received from other
SteamWorks components to a local configuration though the
backends available on the system.

Backend plug-ins will be loaded from <libdir>. The PulleyScript
scripts are read once, at start-up.

)");
	version_usage();
}


int main(int argc, char** argv)
{
	SteamWorks::Logging::Manager logManager("pulley.properties");
	SteamWorks::Logging::getRoot().debugStream() << "Steamworks Pulley" << copyright;

	const struct option longopts[] =
	{
		{"version",   no_argument,        0, 'v'},
		{"help",      no_argument,        0, 'h'},
		/* {"libdir",    required_argument,  0, 'L'}, */
		{0,0,0,0},
	};


	bool carry_on = true;
	int index;
	int iarg = 0;

	while (iarg != -1)
	{
		iarg = getopt_long(argc, argv, "vh", longopts, &index);

		switch (iarg)
		{
		case 'v':
			version_info();
			carry_on = false;
			break;
		case 'h':
			version_help();
			carry_on = false;
			break;
/*
		case 'L':
			log.debugStream() << "-L" << optarg;
			break;
*/
		case '?':
			carry_on = false;
			break;
		case -1:
			// End of options, detected next time around
			break;
		default:
			abort();
		}
	}
	if (!carry_on)
	{
		// Either --help or --version or an option error
		return 1;
	}



	PulleyDispatcher* dispatcher = new PulleyDispatcher();

	while (optind < argc)
	{
		int prsret = dispatcher->do_script(argv[optind++]);
		if (prsret)
		{
			//Error in parsing file
			return 1;
		}
	}

	SteamWorks::FCGI::init_logging("pulley.fcgi");
	SteamWorks::FCGI::mainloop(dispatcher);

	return 0;
}
