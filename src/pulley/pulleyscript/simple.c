#include <stdio.h>

#include "parser.h"

int main(int argc, char **argv)
{
	struct parser prs;
	int prsret = 0;

	pulley_parser_init(&prs);
	prsret = pulley_parser_buffer (&prs, PARSER_LINE, "# hi\n", 5);

	printf("Parser status %d", prsret);
	return 0;
}

