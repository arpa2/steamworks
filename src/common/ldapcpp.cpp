#include <ldap.h>

#include "ldapcpp.h"
#include "logger.h"

template <typename T>
void zerofill(T* p) { memset(p, 0, sizeof(T)); }


class _APIInfo
{
private:
	LDAPAPIInfo info;
	bool valid;
	
public:
	_APIInfo(LDAP* ldaphandle) :
		valid(false)
	{
		zerofill(&info);
		info.ldapai_info_version = LDAP_API_INFO_VERSION;
		int r = ldap_get_option(ldaphandle, LDAP_OPT_API_INFO, &info);
		if (r) return;
		valid = true;
	}

	~_APIInfo()
	{
		if (!valid) return;
		
		ldap_memfree(info.ldapai_vendor_name);
		if (info.ldapai_extensions)
		{
			for (char **s = info.ldapai_extensions; *s; s++)
			{
				ldap_memfree(*s);
			}
			ldap_memfree(info.ldapai_extensions);
		}
	}

	void log(Steamworks::Logger& log, Steamworks::LogLevel level)
	{
		log.getStream(level) << "LDAP API version " << info.ldapai_info_version;
		log.getStream(level) << "LDAP API vendor  " << (info.ldapai_vendor_name ? info.ldapai_vendor_name : "<unknown>");
		if (info.ldapai_extensions)
		{
			for (char **s = info.ldapai_extensions; *s; s++)
			{
				log.getStream(level) << "LDAP extension   " << *s;
			}
		}
	}
	
	bool is_valid() const { return valid; }
	int get_version() const { return info.ldapai_info_version; }
} ;

namespace Steamworks
{
	
	
class LDAPConnection::Private
{
friend class LDAPConnection;
private:
	std::string uri;
	LDAP* ldaphandle;
	LDAPControl* serverctl;
	LDAPControl* clientctl;
	bool valid;
	
	Private(const std::string& uri) :
		uri(uri),
		ldaphandle(nullptr),
		serverctl(nullptr),
		clientctl(nullptr),
		valid(false)
	{
		Steamworks::Logger& log = log4cpp::Category::getInstance("steamworks.ldap");
		log.debugStream() << "LDAP connect to '" << uri << "'";
		
		int r = 0;
		
		r = ldap_initialize(&ldaphandle, uri.c_str());
		if (r) 
		{
			log.errorStream() << "Could not initialize LDAP. Error " << r;
			if (ldaphandle)
			{
				ldap_memfree(ldaphandle);
			}
			return;
		}
		if (!ldaphandle)
		{
			log.errorStream() << "LDAP initialized, but handle is NULL.";
			return;
		}

		if (true)
		{
			_APIInfo info(ldaphandle);
			if (!info.is_valid())
			{
				log.errorStream() << "Could not get LDAP API info."
					<< " Expected API version " << LDAP_API_INFO_VERSION 
					<< " but got " << info.get_version();
				return;
			}
			info.log(log, Steamworks::DEBUG);
		}
		
		if (true)
		{
			int protocol_version = 3;
			r = ldap_set_option(ldaphandle, LDAP_OPT_PROTOCOL_VERSION, &protocol_version);
			if (r)
			{
				log.errorStream() << "Could not set LDAP protocol version 3. Error" << r;
				// TODO: close handle
				return;
			}
		}
		
		if (true)
		{
			int tls_require_cert = LDAP_OPT_X_TLS_NEVER;
			r = ldap_set_option(ldaphandle, LDAP_OPT_X_TLS_REQUIRE_CERT, &tls_require_cert);
			if (r)
			{
				log.warnStream() << "Could not ignore TLS certificate. Error" << r;
			}
			// But carry on ..
		}
		
		// TODO: checking that this works is tricky
		r = ldap_start_tls_s(ldaphandle, &serverctl, &clientctl);
		if (r)
		{
			log.errorStream() << "Could not start TLS. Error" << r << ", " << ldap_err2string(r);
		}
		
		valid = true;
	}

	~Private()
	{
		if (valid)
		{
			// TODO: disconnect gracefully
			valid = false;
		}
		ldap_memfree(ldaphandle);
		ldaphandle = nullptr;
	}
	
	bool is_valid() const { return valid && ldaphandle; }
} ;

LDAPConnection::LDAPConnection(const std::string& uri) :
	d(new Private(uri)),
	valid(false)
{
}

LDAPConnection::~LDAPConnection()
{
	delete d;
	d = nullptr;
}

}  // namespace