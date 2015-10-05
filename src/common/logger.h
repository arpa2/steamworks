/*
Copyright (c) 2014, 2015 InternetWide.org and the ARPA2.net project
All rights reserved. See file LICENSE for exact terms (2-clause BSD license).

Adriaan de Groot <groot@kde.org>
*/
#ifndef NDEBUG
#include <log4cpp/Category.hh>
#include <log4cpp/PropertyConfigurator.hh>
#endif

#include <iostream>

namespace Steamworks
{

namespace Logging
{
#ifdef NDEBUG
class LoggerStream
{
public:
	LoggerStream() {};

	template<typename T> LoggerStream& operator<<(const T& rhs)
	{
		return *this;
	}
} ;

class Logger
{
private:
    static LoggerStream stream;
public:
    Logger() {};

    LoggerStream& debugStream() { return stream; }
    LoggerStream& infoStream() { return stream; }
} ;

typedef enum { DEBUG=10, INFO=20 } LogLevel;

class Manager
{
private:
	static Logger logger;

public:
	Manager(const std::string& propertyfile)
	{
	}

	static
	Logger& getLogger(const std::string& categoryname)
	{
		return logger;
	}

	static
	Logger& getRoot()
	{
		return logger;
	}
} ;
#else
/// Alias for the actual logging implementation, since I'm nog totally convinced of log4cpp
typedef log4cpp::Category Logger;
typedef log4cpp::CategoryStream LoggerStream;

typedef enum { DEBUG=log4cpp::Priority::DEBUG, INFO=log4cpp::Priority::INFO } LogLevel;

class Manager
{
private:
	bool valid;
	static int instances;
	
public:
	Manager(const std::string& propertyfile) :
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
		getRoot().debugStream() << "LoggerManager created, #" << instances;
	}
	

	~Manager()
	{
		if (!(--instances))
		{
			getRoot().debugStream() << "LoggerManager shutdown.";
			log4cpp::Category::shutdown();
		}
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
	Logger& getRoot()
	{
		return log4cpp::Category::getRoot();
	}
};
#endif

// Convenience function, use the Manager-implementation (which depends on NDEBUG).
// This is a template because it is otherwise multiply-defined.
template<typename T>
Logger& getLogger(const T& categoryname) { return Manager::getLogger(categoryname); }

} // namespace Logging
} // namespace
