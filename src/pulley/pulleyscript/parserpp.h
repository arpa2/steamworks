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

#include <picojson.h>

namespace SteamWorks
{

namespace PulleyScript
{

/**
 * Pulley(script) controller object.
 *
 * This is called a parser because it starts off with parsing one
 * or more PulleyScript files; but once that is done it is
 * in charge of the SQLite database storing the data generated
 * by the Pulley middle-end (which consists of the generators
 * defined by the PulleyScript; they are fed data from the LDAP
 * front-end, filling the middle-end database which is then
 * handed off to Pulley back-ends to be consumed locally).
 *
 * A Parser instance should have methods called in the right
 * sequence, first one or more read_file(), then structural_analysis()
 * and setup_sql(). After that setup-phase, things are more free.
 */
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
	 * Return a list of LDAP filter expressions that correspond
	 * to the generators in this script that pull from world;
	 * these generators (and their expressions) correspond to
	 * the SyncRepl subscriptions that are necessary to run
	 * the script.
	 */
	std::forward_list<std::string> find_subscriptions();


	/**
	 * Remove a UUID from the middle-end.
	 */
	void remove_entry(const std::string& uuid);
	void add_entry(const std::string& uuid, const picojson::object& data);
} ;

}  // namespace PulleyScript
}  // namespace SteamWorks

#endif
