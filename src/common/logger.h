/*
Copyright (c) 2014, 2015 InternetWide.org and the ARPA2.net project
All rights reserved. See file LICENSE for exact terms (2-clause BSD license).

Adriaan de Groot <groot@kde.org>
*/

#include <log4cpp/Category.hh>
#include <log4cpp/PropertyConfigurator.hh>

#include <iostream>

namespace Steamworks
{

/// Alias for the actual logging implementation, since I'm nog totally convinced of log4cpp
typedef log4cpp::Category Logger;
typedef log4cpp::CategoryStream LoggerStream;

typedef enum { DEBUG=log4cpp::Priority::DEBUG, INFO=log4cpp::Priority::INFO } LogLevel;

class LoggerManager
{
private:
	bool valid;
	static int instances;
	
public:
	LoggerManager(const std::string& propertyfile) :
		valid(false)
	{
		try
		{
			log4cpp::PropertyConfigurator::configure("crank.properties");
			valid = true;
		}
		catch (log4cpp::ConfigureFailure& e)
		{
			// Logging disabled.
			valid = false;
			std::cerr << "No file " << propertyfile << ". Logging disabled.";
		}
		instances++;
		getLogger().debugStream() << "LoggerManager created, #" << instances;
	}
	
	static
	Logger& getLogger(const std::string& categoryname)
	{
		if (!categoryname.empty())
		{
			return log4cpp::Category::getInstance(categoryname);
		}
		else
		{
			return log4cpp::Category::getRoot();
		}
	}
	
	static
	Logger& getLogger()
	{
		return log4cpp::Category::getRoot();
	}
	
	~LoggerManager()
	{
		if (!(--instances))
		{
			getLogger().debugStream() << "LoggerManager shutdown.";
			log4cpp::Category::shutdown();
		}
	}
};

} // namespace
