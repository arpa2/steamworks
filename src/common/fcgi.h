#include "verb.h"

int fcgi_init_logging(const std::string& logname); 
int fcgi_mainloop(VerbDispatcher* dispatcher=0);
