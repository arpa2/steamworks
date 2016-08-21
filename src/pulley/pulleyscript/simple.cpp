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


static inline std::string _get_parameter(const picojson::value& values, const char* key)
{
	auto v = values.get(key);
	if (v.is<picojson::null>())
	{
		return std::string();
	}
	return v.to_str();
}


void process_values(SteamWorks::PulleyScript::Parser& prs, const picojson::object& v)
{
	auto& log = SteamWorks::Logging::getLogger("steamworks.pulleysimple");

	if (v.count("values") == 0)
	{
		log.errorStream() << "No 'values' element in JSON.";
		return;
	}

	if (!v.at("values").is<picojson::array>())
	{
		log.errorStream() << "Element 'values' is not an array.";
		return;
	}

	bool add_not_delete;
	{
		std::string verb;
		if (v.count("verb") > 0)
		{
			const auto& verb_val = v.at("verb");
			if (verb_val.is<std::string>())
			{
				verb = verb_val.to_str();
			}
		}
		if (verb == "add")
		{
			add_not_delete = true;
		}
		else if (verb == "del")
		{
			add_not_delete = false;
		}
		else
		{
			log.errorStream() << "Verb '" << verb << "' is not recognized (use add or del).";
			return;
		}
	}

	prs.begin();

	const auto& values = v.at("values").get<picojson::array>();
	unsigned int index = 0;
	unsigned int count = 0;
	for (const auto& v : values)
	{
		index++;
		if (!v.is<picojson::object>())
		{
			log.warnStream() << "values[" << index-1 << "] is not an object.";
			continue;
		}

		std::string uuid = _get_parameter(v, "uuid");
		if (uuid.empty())
		{
			log.warnStream() << "values[" << index-1 << "] has no UUID.";
			continue;
		}

		if (add_not_delete)
		{
			prs.add_entry(uuid, v.get<picojson::object>());
		}
		else
		{
			prs.remove_entry(uuid);
		}
		count++;
	}
	log.debugStream() << "Processed " << count << " values.";

	prs.commit();
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
	prs.find_backends();

	picojson::value v;
	const char *json_filename = argv[optind++];
	std::ifstream input(json_filename);
	picojson::parse(v, input);

	if (v.is<picojson::object>())
	{
		log.debugStream() << "JSON object read from " << json_filename;
		process_values(prs, v.get<picojson::object>());
	}
	else
	{
		log.errorStream() << "Could not read JSON from " << json_filename;
		return 1;
	}

	return 0;
}

