#include <stdio.h>
#include <string>

#include "parserpp.h"

#include <logger.h>

static const char* copyright = "Copyright (C) 2014-2016 InternetWide.org and the ARPA2.net project";

int main(int argc, char **argv)
{
	if (argc < 2)
	{
		fprintf(stderr,"Usage: simple <file>\n");
		return 1;
	}

	SteamWorks::Logging::Manager logManager("pulleyscript.properties", SteamWorks::Logging::DEBUG);
	SteamWorks::Logging::getRoot().debugStream() << "Steamworks PulleyScript " << copyright;

	SteamWorks::PulleyScript::Parser prs;
	int prsret = prs.read_file(argv[1]);
	printf("Parser status %d\n", prsret);
	printf("Parser analysis %d\n", prs.structural_analysis());
	printf("  .. %s\n", prs.state_string().c_str());
	printf("Parser SQL setup %d\n", prs.setup_sql());
	printf("  .. %s\n", prs.state_string().c_str());
	return 0;
}

