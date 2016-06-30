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

#include <forward_list>
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
	enum class State { Broken, Initial, Parsing, Analyzed };

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

	/**
	 * Prepare the SQL database belonging to this script;
	 * ensure that it has the right tables, etc.
	 */
	int setup_sql();

	/**
	 * Testing-function for whatever is next in the C++ parser wrapper.
	 */
	void explain();

	/**
	 * Return a list of LDAP filter expressions that correspond
	 * to the generators in this script that pull from world;
	 * these generators (and their expressions) correspond to
	 * the SyncRepl subscriptions that are necessary to run
	 * the script.
	 */
	std::forward_list<std::string> find_subscriptions();
} ;

}  // namespace PulleyScript
}  // namespace SteamWorks

#endif
