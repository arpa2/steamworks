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

	virtual int exec(const std::string& verb, const Values values, Object response);

	State state() const { return m_state; }

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
} ;


#endif
