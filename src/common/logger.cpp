/*
Copyright (c) 2014, 2015 InternetWide.org and the ARPA2.net project
All rights reserved. See file LICENSE for exact terms (2-clause BSD license).

Adriaan de Groot <groot@kde.org>
*/

#include "logger.h"

#ifdef NDEBUG
SteamWorks::Logging::LoggerStream SteamWorks::Logging::Logger::stream;
SteamWorks::Logging::Logger SteamWorks::Logging::Manager::logger;
#else
int SteamWorks::Logging::Manager::instances = 0;
#endif

