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

#include "driver.h"

#include "backend.h"

#include <forward_list>
#include <memory>

#include <picojson.h>

namespace SteamWorks
{

namespace PulleyScript
{
class BackendTransaction;

/**
 * Parameters to pass to a backend instance. This is basically a
 * C-array version of vector<std::string>, clumsily constructed.
 * Memory is owned by the Parameters object.
 */
struct BackendParameters
{
	std::string name; // Only used for logging
	drvnum_t driver;  // Parameters for which driver
	std::unique_ptr<PulleyBack::Instance> instance;
	int varc;
	int argc;
	char** argv;

	/**
	 * Create a Parameters object from a vector of C++
	 * strings. Creates copies of the c_str() data of each
	 * string in the vector.
	 *
	 * The name of the backend is passed in @p n ; this is only
	 * used for logging.
	 */
	BackendParameters(std::string n, const std::vector<std::string>& expressions);
	~BackendParameters();
} ;

/**
 * Logging support for BackendParameters.
 */
std::ostringstream& operator <<(std::ostringstream&, const BackendParameters&);

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
friend class BackendTransaction;
private:
	class Private;
	std::unique_ptr<Private> d;

public:
	enum class State { Broken, Initial, Parsing, Analyzed, Ready };

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
	 * Find BackendParameters objects, one for each output line.
	 * This is stored internally, and should be done before
	 * adding or removing entries.
	 */
	void find_backends();

	/**
	 * Remove a UUID from the middle-end.
	 */
	void remove_entry(const std::string& uuid);
	void add_entry(const std::string& uuid, const picojson::object& data);

	/**
	 * Transaction support. This is not mandatory -- if you do
	 * not call these functions, remove_entry() and add_entry()
	 * will perform implicit transactions around each addition or
	 * removal. If you call begin() then you start a transaction
	 * that lasts as long as the returned BackendTransaction is
	 * alive; there is no explicit commit. Keeping the transaction
	 * longer than the underlying Parser is a Bad Idea (tm).
	 *
	 * There is no support for nested transactions: everyone shares
	 * one BackendTransaction until everyone is done with it.
	 *
	 * Sample use:
	 *   {
	 *     auto transaction = parser.begin();
	 *     parser.add_entry(uuid, data);
	 *     parser.remove_entry(other_uuid);
	 *   }
	 * The block scope destroys the transaction at the end of the
	 * block and the changes are committed (or rolled back) together.
	 *
	 * You can't tell if the transaction completed without error.
	 * (TODO: return error state somehow -- e.g. exception).
	 */
	std::shared_ptr<BackendTransaction> begin();
} ;

class BackendTransaction
{
friend class Parser;
private:
	static unsigned int instance_count;

	unsigned int m_instance_number;
	Parser::Private *m_parent;

public:
	BackendTransaction(SteamWorks::PulleyScript::Parser::Private* parent);
	~BackendTransaction();
} ;

}  // namespace PulleyScript
}  // namespace SteamWorks

#endif
