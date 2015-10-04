/*
Copyright (c) 2014, 2015 InternetWide.org and the ARPA2.net project
All rights reserved. See file LICENSE for exact terms (2-clause BSD license).

Adriaan de Groot <groot@kde.org>
*/

#include <iostream>  // wrong header for std::string

namespace Steamworks
{
	
class LDAPConnection
{
private:
	class Private;
	Private *d;
	bool valid;
	
public:
	LDAPConnection(const std::string& uri);
	~LDAPConnection();
	bool is_valid() const { return valid; }
} ;
	
}  // namespace
