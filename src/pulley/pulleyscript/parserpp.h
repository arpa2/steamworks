/*
Copyright (c) 2016 InternetWide.org and the ARPA2.net project
All rights reserved. See file LICENSE for exact terms (2-clause BSD license).

Adriaan de Groot <groot@kde.org>
*/

/**
 * This is a C++ wrapper around the PulleyScript parser / generator / etc.
 * complex, handling some memory and file management and providing
 * convenience C++-style interfaces to some of the iterators.
 */
#ifndef STEAMWORKS_PULLEYSCRIPT_PARSERPP_H
#define STEAMWORKS_PULLEYSCRIPT_PARSERPP_H

#include <memory>

namespace SteamWorks
{

namespace PulleyScript
{

class Parser
{
private:
	class Private;
	std::unique_ptr<Private> d;

public:
	Parser();
	~Parser();

	/**
	 * Reads (parses & analyzes) a given @p filename.
	 * Adds the result to the parser state. Returns
	 * 0 on success, non-zero on error.
	 */
	int read_file(const char *filename);
	/**
	 * As above, from an opened file.
	 */
	int read_file(FILE *input);
} ;

}  // namespace PulleyScript
}  // namespace SteamWorks

#endif
