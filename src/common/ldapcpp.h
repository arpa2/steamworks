/*
Copyright (c) 2014, 2015 InternetWide.org and the ARPA2.net project
All rights reserved. See file LICENSE for exact terms (2-clause BSD license).

Adriaan de Groot <groot@kde.org>
*/

#include <iostream>  // wrong header for std::string

namespace Steamworks
{

namespace LDAP
{
	
class Connection
{
private:
	class Private;
	Private *d;
	bool valid;
	
public:
	Connection(const std::string& uri);
	~Connection();
	bool is_valid() const { return valid; }
} ;
	
}  // namespace LDAP
}  // namespace
