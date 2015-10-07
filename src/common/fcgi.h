/*
Copyright (c) 2014, 2015 InternetWide.org and the ARPA2.net project
All rights reserved. See file LICENSE for exact terms (2-clause BSD license).

Adriaan de Groot <groot@kde.org>
*/

#include "verb.h"

namespace Steamworks
{
namespace FCGI
{

int init_logging(const std::string& logname); 
int mainloop(VerbDispatcher* dispatcher=0);

}  // namespace FCGI
}  // namespace Steamworks
