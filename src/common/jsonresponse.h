/*
Copyright (c) 2014-2016 InternetWide.org and the ARPA2.net project
All rights reserved. See file LICENSE for exact terms (2-clause BSD license).

Adriaan de Groot <groot@kde.org>
*/

#ifndef STEAMWORKS_COMMON_JSONRESPONSE_H
#define STEAMWORKS_COMMON_JSONRESPONSE_H

#include "picojson.h"

namespace SteamWorks
{
namespace JSON
{
using Values = picojson::value;
using Object = picojson::value::object;

/**
	* Produce a simple JSON output with the status code and message.
	* The response is given attributes "status", "message" and "errno".
	*
	* This does not change the HTTP status of the FCGI response.
	*
	* @see simple_output in fcgi.cpp
	*/
void simple_output(Object response, int status, const char* message=nullptr, const int err=0);

}  // namespace JSON
}  // namespace Steamworks

#endif
