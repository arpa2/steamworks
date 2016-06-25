/*
Copyright (c) 2014-2016 InternetWide.org and the ARPA2.net project
All rights reserved. See file LICENSE for exact terms (2-clause BSD license).

Adriaan de Groot <groot@kde.org>
*/

#include "jsonresponse.h"

#include <string>

void SteamWorks::JSON::simple_output(SteamWorks::JSON::Object response, int status, const char* message, const int err)
{
	if (status)
	{
		picojson::value status_v(static_cast<double>(status));
		response.emplace(std::string("status"), status_v);
	}
	if ((message != nullptr) && (strlen(message) > 0))
	{
		picojson::value msg_v{std::string(message)};
		response.emplace(std::string("message"), msg_v);
	}
	if (errno)
	{
		picojson::value errno_v{static_cast<double>(err)};
		response.emplace(std::string("errno"), errno_v);
	}
}
