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
	enum class State { Initial, Parsing, Analyzed };

	Parser();
	~Parser();

	State state() const;
	std::string state_string() const;

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

	/**
	 * Perform structural analysis on the parsed structures
	 * (you must have parsed at least one thing already)
	 * and prepare for using the script.
	 * Returns 0 on success.
	 */
	int structural_analysis();

	int setup_sql();
} ;

}  // namespace PulleyScript
}  // namespace SteamWorks

#endif
