/*
Copyright (c) 2014, 2015 InternetWide.org and the ARPA2.net project
All rights reserved. See file LICENSE for exact terms (2-clause BSD license).

Adriaan de Groot <groot@kde.org>
*/

#ifndef STEAMWORKS_CRANK_H
#define STEAMWORKS_CRANK_H

#include <memory>

#include "verb.h"

class CrankDispatcher : public VerbDispatcher
{
private:
	class Private;
	std::unique_ptr<Private> d;

	typedef enum { disconnected=0, connected, stopped } State;
	State m_state;

public:
	CrankDispatcher();

	virtual int exec(const std::string& verb, const Values& values, Object& response) override;

	State state() const { return m_state; }

protected:
	// Generic commands
	int do_connect(const Values& values, Object& response);
	int do_stop(const Values& values);
	int do_serverinfo(const Values& values, Object& response);
	int do_serverstatus(const Values& values, Object& response);

	// LDAP search / update etc.
	int do_search(const Values& values, Object& response);
	int do_update(const Values& values, Object& response);
	int do_delete(const Values& values, Object& response);
	int do_add(const Values& values, Object& response);

	// Meta-information about the server
	int do_typeinfo(const Values& values, Object& response);
} ;


#endif
