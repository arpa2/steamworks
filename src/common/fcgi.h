/*
Copyright (c) 2014, 2015 InternetWide.org and the ARPA2.net project
All rights reserved. See file LICENSE for exact terms (2-clause BSD license).

Adriaan de Groot <groot@kde.org>
*/

/**
 * FCGI support code. An FCGI program waits for incoming connections
 * (corresponding to a new FCGI request) and processes them. The API
 * is just one function, mainloop(), which uses a passed-in dispatcher
 * object to respond to the requests.
 *
 * Use init_logging() to cause FCGI processing to log to a given
 * category; this is optional.
 */
#ifndef STEAMWORKS_COMMON_FCGI_H
#define STEAMWORKS_COMMON_FCGI_H

#include "verb.h"

namespace SteamWorks
{
namespace FCGI
{

/// Set up logging, use the given @p categoryname for logging FCGI actions.
int init_logging(const std::string& categoryname);
/// Run the main FCGI processing loop, using @p dispatcher for actual processing.
/// If @p dispatcher is null, then only dummy processing occurs.
int mainloop(VerbDispatcher* dispatcher=0);

}  // namespace FCGI
}  // namespace Steamworks

#endif
