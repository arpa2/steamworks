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

static char hex[] = "0123456789ABCDEF";

void SteamWorks::Logging::log_hex(SteamWorks::Logging::LoggerStream& log, const uint8_t* p, uint32_t len)
{
	for (uint32_t i=0; i<len; i++, p++)
	{
		char c0 = hex[(*p & 0xf0) >> 4];
		char c1 = hex[*p & 0x0f];
		log << c0 << c1 << ' ';
	}
}

void SteamWorks::Logging::log_hex(SteamWorks::Logging::LoggerStream& log, const char* p, uint32_t len)
{
	log_hex(log, (const unsigned char *)p, len);
}

void SteamWorks::Logging::log_indent(SteamWorks::Logging::LoggerStream& log, unsigned int indent)
{
	for (unsigned int i=0; i<indent; i++)
	{
		log << ' ' << ' ';
	}
}

extern "C" void write_logger(const char* logname, const char* message)
{
	SteamWorks::Logging::getLogger(std::string(logname)).debugStream() << message;
}


