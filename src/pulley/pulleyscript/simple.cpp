#include <stdio.h>

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
	return 0;
}

