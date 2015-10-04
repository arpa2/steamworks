/*
Copyright (c) 2014, 2015 InternetWide.org and the ARPA2.net project
All rights reserved. See file LICENSE for exact terms (2-clause BSD license).

Adriaan de Groot <groot@kde.org>
*/

#include "verb.h"

int fcgi_init_logging(const std::string& logname); 
int fcgi_mainloop(VerbDispatcher* dispatcher=0);
