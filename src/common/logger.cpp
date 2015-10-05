/*
Copyright (c) 2014, 2015 InternetWide.org and the ARPA2.net project
All rights reserved. See file LICENSE for exact terms (2-clause BSD license).

Adriaan de Groot <groot@kde.org>
*/

#include "logger.h"

#ifdef NDEBUG
Steamworks::Logging::LoggerStream Steamworks::Logging::Logger::stream;
Steamworks::Logging::Logger Steamworks::Logging::Manager::logger;
#else
int Steamworks::Logging::Manager::instances = 0;
#endif

