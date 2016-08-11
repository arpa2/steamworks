/*
Copyright (c) 2016 InternetWide.org and the ARPA2.net project
All rights reserved. See file LICENSE for exact terms (2-clause BSD license).

Adriaan de Groot <groot@kde.org>
*/

/**
 * This file is for use with C programs that call into write_logger()
 * but don't actually link with swcommon (which is the bridge from
 * write_logger() to log4cpp). So instead of logging, this just goes
 * back to printf().
 */

#include <stdio.h>

void write_logger(const char* logname, const char* message)
{
	printf("%s: %s\n", logname, message);
}

