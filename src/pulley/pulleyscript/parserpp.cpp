/*
Copyright (c) 2016 InternetWide.org and the ARPA2.net project
All rights reserved. See file LICENSE for exact terms (2-clause BSD license).

Adriaan de Groot <groot@kde.org>
*/

#include "parserpp.h"

#include "parser.h"

#include <logger.h>

class SteamWorks::PulleyScript::Parser::Private
{
private:
	struct parser m_prs;
	bool m_valid;

public:
	Private() : m_valid(false)
	{
		if (pulley_parser_init(&m_prs))
		{
			m_valid = true;
		}
	}
	~Private()
	{
		if (is_valid())
		{
			pulley_parser_cleanup_syntax(&m_prs);
			pulley_parser_cleanup_semantics(&m_prs);
		}
		m_valid = false;
	}

	bool is_valid() const { return m_valid; }

	int read_file(FILE* input)
	{
		int prsret = pulley_parser_file (&m_prs, input);
		if (prsret)
		{
			auto& log = SteamWorks::Logging::getLogger("steamworks.pulleyscript");
			log.errorStream() << "Could not read file input (" << prsret << ')';
			return prsret;
		}

		return 0;
	}
} ;

SteamWorks::PulleyScript::Parser::Parser() :
	d(new Private)
{
}

SteamWorks::PulleyScript::Parser::~Parser()
{
}


int SteamWorks::PulleyScript::Parser::read_file(const char* filename)
{
	auto& log = SteamWorks::Logging::getLogger("steamworks.pulleyscript");

	FILE *fh = fopen(filename, "r");
	if (!fh) {
		log.errorStream() << "Failed to open " << filename;
		return 1;
	}
	log.debugStream() << "Loading " << filename;

	int r = read_file(fh);
	fclose(fh);
	return r;
}

int SteamWorks::PulleyScript::Parser::read_file(FILE* input)
{
	return d->read_file(input);
}
