#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>

#include <fstream>
#include <string>

#include "parserpp.h"

#include <logger.h>

static const char progname[] = "PulleySimple";
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
    simple [-L libdir] scriptfile datafile
\n\n)");
}

static void version_help()
{
	version_info();
	printf(R"(
Parse a PulleyScript file and feed the resulting pulley one data file,
which is a JSON representation of an LDAP object. Calls the backend
plugins specified by the script to process the change represented by
the JSON file.

Backend plug-ins will be loaded from <libdir>.

)");
	version_usage();
}


int main(int argc, char **argv)
{
	SteamWorks::Logging::Manager logManager("pulleyscript.properties", SteamWorks::Logging::DEBUG);
	SteamWorks::Logging::getRoot().debugStream() << "SteamWorks " << progname << ' ' << copyright;

	auto& log = SteamWorks::Logging::getLogger("steamworks.pulleysimple");

	const struct option longopts[] =
	{
		{"version",   no_argument,        0, 'v'},
		{"help",      no_argument,        0, 'h'},
		{"libdir",    required_argument,  0, 'L'},
		{0,0,0,0},
	};


	bool carry_on = true;
	int index;
	int iarg = 0;

	while (iarg != -1)
	{
		iarg = getopt_long(argc, argv, "L:vh", longopts, &index);

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
		case 'L':
			log.debugStream() << "-L" << optarg;
			break;
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

	SteamWorks::PulleyScript::Parser prs;
	int prsret = -1;
	if (!(optind < argc))
	{
		version_usage();
		return 1;
	}
	if (optind != argc - 2)
	{
		version_usage();
		return 1;
	}

	prsret = prs.read_file(argv[optind++]);
	if (prsret)
	{
		// Couldn't parse file, or no files given.
		return 1;
	}

	log.debugStream() << "Parser status " << prsret;
	prsret = prs.structural_analysis();
	log.debugStream() << "Parser analysis " << prsret;
	log.debugStream() << "  .. " << prs.state_string().c_str();
	prsret = prs.setup_sql();
	log.debugStream() << "Parser SQL setup " << prsret;
	log.debugStream() << "  .. " << prs.state_string().c_str();
	if (prs.state() == SteamWorks::PulleyScript::Parser::State::Broken)
	{
		return 1;
	}
	prs.find_subscriptions();
	auto l = prs.find_backends();
	log.debugStream() << "Parser found backends:";
	for (const auto& be : l)
	{
		log.debugStream() << "  .. " << be;
	}

	picojson::value v;
	const char *json_filename = argv[optind++];
	std::ifstream input(json_filename);
	picojson::parse(v, input);

	if (v.is<picojson::object>())
	{
		log.debugStream() << "JSON object read from " << json_filename;

		const picojson::value::object& obj = v.get<picojson::object>();
		prs.add_entry("1234", obj);
	}
	else
	{
		log.errorStream() << "Could not read JSON from " << json_filename;
		return 1;
	}

	return 0;
}

