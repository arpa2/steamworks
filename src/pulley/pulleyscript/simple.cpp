#include <stdio.h>
#include <string>

#include "parserpp.h"

int main(int argc, char **argv)
{
	if (argc < 2)
	{
		fprintf(stderr,"Usage: simple <file>\n");
		return 1;
	}

	SteamWorks::PulleyScript::Parser prs;
	int prsret = prs.read_file(argv[1]);
	printf("Parser status %d\n", prsret);
	printf("Parser analysis %d\n", prs.structural_analysis());
	printf("  .. %s\n", prs.state_string().c_str());
	return 0;
}

