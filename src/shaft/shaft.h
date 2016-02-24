/*
Copyright (c) 2014-2016 InternetWide.org and the ARPA2.net project
All rights reserved. See file LICENSE for exact terms (2-clause BSD license).

Adriaan de Groot <groot@kde.org>
*/

#ifndef STEAMWORKS_SHAFT_H
#define STEAMWORKS_SHAFT_H

#include <memory>

#include "verb.h"

class ShaftDispatcher : public VerbDispatcher
{
private:
	class Private;
	std::unique_ptr<Private> d;

	typedef enum { disconnected=0, connected, stopped } State;
	State m_state;

public:
	ShaftDispatcher();

	virtual int exec(const std::string& verb, const Values values, Object response);

	State state() const { return m_state; }

protected:
	/** Connect to the downstream (e.g. destination) LDAP server.
	 *  This is where the shaft is going to write to. */
	int do_connect(const Values values, Object response);
	int do_stop(const Values values);
	int do_serverinfo(const Values values, Object response);
} ;


#endif
