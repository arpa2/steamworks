#ifndef STEAMWORKS_VERB_H
#define STEAMWORKS_VERB_H

#include "picojson.h"

class VerbDispatcher
{
public:
	typedef picojson::value& Values;

	virtual int exec(const std::string& verb, const Values values) = 0;
} ;

#endif