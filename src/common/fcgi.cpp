/*
Copyright (c) 2014, 2015 InternetWide.org and the ARPA2.net project
All rights reserved. See file LICENSE for exact terms (2-clause BSD license).

Adriaan de Groot <groot@kde.org>
*/

#include <stdlib.h>
#include <string.h>

#include "fcgiapp.h"
#include "picojson.h"

#include "fcgi.h"
#include "logger.h"
#include "verb.h"


static const int MAX_STDIN = 65535;
static int request_count = 0;

namespace fcgi
{
static Steamworks::Logging::Logger *logger = 0;

/**
 * Reads the remainer of an input stream (this seems to be needed if wrapping
 * FCGI streams into C++ streams on some versions of libg++). Currently no-op.
 */
void drain_input(FCGX_Stream* in)
{
}

class FCGInputStream
{
private:
	FCGX_Stream* input;
	bool eof, valid;
	char current;

public:
	FCGInputStream(FCGX_Stream* s) :
		input(s),
		eof(false),
		valid(false)
	{
	}

	FCGInputStream() :
		input(0),
		eof(true),
		valid(false)
	{
	}

	char operator *()
	{
		if (valid)
		{
			return current;
		}
		else
		{
			current = FCGX_GetChar(input);
			if (current < 0)
			{
				eof = true;
			}
			valid = true;
		}
		return current;
	}

	bool operator==(const FCGInputStream& rhs) const
	{
		if (eof && rhs.eof)
		{
			return true;
		}
		else
		{
			return (eof == rhs.eof) && (input == rhs.input);
		}
	}

	// Dummy prefix-++
	FCGInputStream& operator++()
	{
		valid = false;
		return *this;
	}
} ;

const char _empty_response[] = "Content-type: text/json\r\nContent-length: 2\r\n\r\n{}";

void simple_output(FCGX_Stream* out, int status, const picojson::value& map)
{
	std::string text = map.serialize(true);
	if (text.empty())
	{
		FCGX_PutS(_empty_response, out);
		return;
	}
	const char* const textp = text.c_str();
	if (!textp)
	{
		FCGX_PutS(_empty_response, out);
		return;
	}
	size_t len = strlen(textp);
	if (len < 1)
	{
		FCGX_PutS(_empty_response, out);
		return;
	}

	FCGX_SetExitStatus(status, out);
	FCGX_FPrintF(out, "Content-type: text/json\r\nContent-length: %u\r\n\r\n", len);
	FCGX_PutS(textp, out);
}

void simple_output(FCGX_Stream* out, int status, const char* message=nullptr, const int err=0)
{
	if (logger && (status != 200))
	{
		logger->debugStream() << "HTTP status " << status << ": " << message;
	}

	picojson::value::object map;
	if (status)
	{
		picojson::value status_v(static_cast<double>(status));
		map.emplace(std::string("status"), status_v);
	}
	if ((message != nullptr) && (strlen(message) > 0))
	{
		picojson::value msg_v{std::string(message)};
		map.emplace(std::string("message"), msg_v);
	}
	if (errno)
	{
		picojson::value errno_v{static_cast<double>(err)};
		map.emplace(std::string("errno"), errno_v);
	}
	picojson::value v(map);
	simple_output(out, status, v);
}

static int find_verb(const picojson::value& v, std::string &out)
{
	if (!v.is<picojson::object>())
	{
		return -1;
	}
	const picojson::value::object& obj = v.get<picojson::object>();
	for (picojson::value::object::const_iterator i = obj.begin();i != obj.end(); ++i)
	{
		if (i->first == "verb")
		{
			out = i->second.to_str();
			return 0;
		}
	}
	return -2;
}

int handle_request(FCGX_Stream* in, FCGX_Stream* out, FCGX_Stream* err, FCGX_ParamArray env, VerbDispatcher* dispatcher)
{
	const char* s_content_length = FCGX_GetParam("CONTENT_LENGTH", env);
	if (!s_content_length)
	{
		simple_output(out, 500, "No request data.");
		drain_input(in);
		return 0;  // Handled correctly, even though we sent an error to the client
	}

	int content_length = atoi(s_content_length);
	if (content_length < 1)
	{
		simple_output(out, 500, "Request data too short.");
		drain_input(in);
		return 0;
	}
	if (content_length > MAX_STDIN)
	{
		content_length = MAX_STDIN;
	}

	if (content_length > 0)
	{
		picojson::value v;
		std::string error_string;
		std::string verb;
		int r;
		picojson::parse(v, FCGInputStream(in), FCGInputStream(), &error_string);
		if (!error_string.empty())
		{
			simple_output(out, 500, error_string.c_str());
		}
		else
		{
			if ((r = find_verb(v, verb)) < 0)
			{
				simple_output(out, 500, "No verb.");
			}
			else
			{
				if (logger)
				{
					logger->debug("Got verb '%s'.", verb.c_str());
				}
				if (dispatcher)
				{
					r = dispatcher->exec(verb, v);
				}
				if (r < 0)
				{
					simple_output(out, 500, "Bad request", -r);
					drain_input(in);
					return r;
				}
			}
		}
	}
	drain_input(in);
	return 0;
}

}  // namespace

int Steamworks::FCGI::init_logging(const std::string& logname)
{
	Steamworks::Logging::Logger& log = Steamworks::Logging::getLogger(logname);
	fcgi::logger = &log;
	return 0;
}

// TODO: integrate with other main loops
int Steamworks::FCGI::mainloop(VerbDispatcher *dispatcher)
{
	FCGX_Stream *in, *out, *err;
	FCGX_ParamArray envp;

	while(FCGX_Accept(&in, &out, &err, &envp) >= 0)
	{
		int r = fcgi::handle_request(in, out, err, envp, dispatcher);
		if (r) return r;

		request_count++;
	}
	return 0;
}
