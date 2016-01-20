/*
Copyright (c) 2014-2016 InternetWide.org and the ARPA2.net project
All rights reserved. See file LICENSE for exact terms (2-clause BSD license).

Adriaan de Groot <groot@kde.org>
*/

#include "logger.h"

static const char* copyright = "Copyright (C) 2014, 2015 InternetWide.org and the ARPA2.net project";

int main(int argc, char** argv)
{
	Steamworks::Logging::Manager logManager("shaft.properties");
	Steamworks::Logging::getRoot().debugStream() << "Steamworks Shaft " << copyright;

	return 0;
}
