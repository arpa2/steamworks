#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>

#include "parserpp.h"

#include <logger.h>

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


int main(int argc, char **argv)
{
	SteamWorks::Logging::Manager logManager("pulleyscript.properties", SteamWorks::Logging::DEBUG);
	SteamWorks::Logging::getRoot().debugStream() << "SteamWorks " << progname << ' ' << copyright;

	auto& log = SteamWorks::Logging::getLogger("steamworks.pulleyscript");

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
	while (optind < argc)
	{
		prsret = prs.read_file(argv[optind++]);
		if (prsret)
		{
			//Error in parsing file
			break;
		}
	}
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
	picojson::parse(v, R"({
	"TlsPoolTrustAnchor": "o=Verishot primary root,ou=Trust Contestors,dc=test,dc=example,dc=com",
	"TlsPoolValidationExpression": "1",
	"TlsPoolSupportedRole": "server"
})"
	);
	log.debugStream() << "JSON object is object? " << (v.is<picojson::object>() ? "yes" : "no");

	const picojson::value::object& obj = v.get<picojson::object>();
	prs.add_entry("1234", obj);
	return 0;
}

