#include "fcgi.h"
#include "logger.h"

#include "crank.h"

int main(int argc, char** argv)
{
	Steamworks::LoggerManager logManager("crank.properties");
	
	CrankDispatcher dispatcher;
	fcgi_init_logging("crank.fcgi");
	fcgi_mainloop(&dispatcher);
	return 0;
}
