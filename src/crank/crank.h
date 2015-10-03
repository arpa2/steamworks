#ifndef STEAMWORKS_CRANK_H
#define STEAMWORKS_CRANK_H

#include "verb.h"

class CrankDispatcher : public VerbDispatcher
{
public:
	CrankDispatcher();
	
	virtual int exec(const std::string& verb, const Values values);
	
protected:
	int do_connect(const Values values);
	int do_stop(const Values values);
	
private:
	typedef enum { disconnected=0, connected, stopped } State;
	State state;
} ;


#endif