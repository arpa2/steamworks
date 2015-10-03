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