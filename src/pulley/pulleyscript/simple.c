#include <stdio.h>

#include "parser.h"

int main(int argc, char **argv)
{
	struct parser prs;
	int prsret = 0;
	FILE *fh;

	if (argc < 2)
	{
		fprintf(stderr,"Usage: simple <file>\n");
		return 1;
	}

	pulley_parser_init(&prs);

	fh = fopen (argv [1], "r");
	if (!fh) {
		fprintf (stderr, "Failed to open %s\n", argv [1]);
		return 1;
	}
	printf ("Loading %s\n", argv [1]);
	prsret = pulley_parser_file (&prs, fh);
	fclose (fh);

	printf("Parser status %d", prsret);
	return 0;
}

