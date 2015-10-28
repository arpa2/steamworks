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

	virtual int exec(const std::string& verb, const Values values, Object response);

	State state() const { return m_state; }

protected:
	int do_connect(const Values values);
	int do_stop(const Values values);
	int do_search(const Values values, Object response);
} ;


#endif
