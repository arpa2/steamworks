/*
Copyright (c) 2014-2016 InternetWide.org and the ARPA2.net project
All rights reserved. See file LICENSE for exact terms (2-clause BSD license).

Adriaan de Groot <groot@kde.org>
*/

#ifndef STEAMWORKS_PULLEY_H
#define STEAMWORKS_PULLEY_H

#include <memory>

#include "verb.h"

class PulleyDispatcher : public VerbDispatcher
{
private:
	class Private;
	std::unique_ptr<Private> d;

	typedef enum { disconnected=0, connected, stopped } State;
	State m_state;

public:
	PulleyDispatcher();

	int exec(const std::string& verb, const Values values, Object response) override;
	void poll() override;

	State state() const { return m_state; }

	/** Load a PulleyScript script. This overload with a const char *
	 *  parameter loads a file from the local filesystem, for use
	 *  before the Pulley connects to the LDAP server.
	 */
	int do_script(const char* filename);

protected:
	/** Connect to the upstream (e.g. source) LDAP server.
	 *  This is where the pulley gets its information. */
	int do_connect(const Values values, Object response);
	int do_stop(const Values values);
	int do_serverinfo(const Values values, Object response);

	/** Start following a (subtree-) DIT. This starts up SyncRepl
	 *  for that DIT. */
	int do_follow(const Values values, Object response);
	/** Stop following a previously followed DIT. This terminates
	 *  SyncRepl for that DIT. */
	int do_unfollow(const Values values, Object response);

	/** Debugging method, dump the tree of stored DIT entries. */
	int do_dump_dit(const VerbDispatcher::Values values, VerbDispatcher::Object response);

	/** Drop all stored state, and restart LDAP SyncRepl. */
	int do_resync(const Values values, Object response);

	/** Load a PulleyScript script. */
	int do_script(const Values values, Object response);
} ;


#endif
