/*
Copyright (c) 2014, 2015 InternetWide.org and the ARPA2.net project
All rights reserved. See file LICENSE for exact terms (2-clause BSD license).

Adriaan de Groot <groot@kde.org>
*/

#include "fcgi.h"
#include "logger.h"

#include "crank.h"

int main(int argc, char** argv)
{
	Steamworks::Logging::Manager logManager("crank.properties");
	
	CrankDispatcher dispatcher;
	fcgi_init_logging("crank.fcgi");
	fcgi_mainloop(&dispatcher);
	return 0;
}
