/*
Copyright (c) 2014, 2015 InternetWide.org and the ARPA2.net project
All rights reserved. See file LICENSE for exact terms (2-clause BSD license).

Adriaan de Groot <groot@kde.org>
*/

/**
 * Logging support for Steamworks. Logging is based on log4cpp, which is
 * in turn inspired by log4j, and which resembles Python's logging module
 * as well.
 *
 * The Logging::Manager class can be used to read configuration files and
 * set up logging; without a manager, whatever the default configuration
 * is, gets used. Logging itself goes through the Logging::getLogger()
 * function, which returns a logger instance.
 *
 * Logging typically goes like this:
 * 	Logger& log = getLogger("category.subcategory.specifics");
 * 	log.debugStream() << "A debug message";
 *
 * If NDEBUG is defined, logging is a no-op.
 *
 * Right now, logging is based on log4cpp. Using only the interfaces in
 * this file makes it possible to swap log4cpp out for something else:
 * there are several logging frameworks, and it's not entirely clear
 * if log4cpp is still maintained.
 */

#ifndef STEAMWORKS_COMMON_LOGGER_H
#define STEAMWORKS_COMMON_LOGGER_H

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
/*
 * In the NDEBUG case, we have dummy classes LoggerStream,
 * Logger, and Manager that do nothing. This API needs to
 * be kept in-sync with the API in the other branch -- in
 * particular, the names of LogLevel enums.
 */
class LoggerStream
{
public:
	LoggerStream() {};

	// Eat anything when logged; this probably consumes more
	// than the real implementation does.
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

	// TODO: other levels of streams.
	LoggerStream& debugStream() { return stream; }
	LoggerStream& infoStream() { return stream; }
	LoggerStream& warnStream() { return stream; }
	LoggerStream& errorStream() { return stream; }
	LoggerStream& getStream(int level) { return stream; }

	void debug(...) {}
} ;

// Bogus level numbers; I don't know if these are compatible with
// log4cpp values in particular.
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
/// Provide an alias for the implementation of Logger.
typedef log4cpp::Category Logger;
/// Provide an alias for the implementation of LoggerStream.
typedef log4cpp::CategoryStream LoggerStream;

/// Provide aliases for logging levels (priorities).
typedef enum { DEBUG=log4cpp::Priority::DEBUG, INFO=log4cpp::Priority::INFO } LogLevel;

/**
 * Manages configurations for logging by reading in a configuration file:
 * with log4cpp, this is a Java-style properties file that sets up logging.
 * Multiple managers may be created; the configuration is additive.
 * If any managers are created, then when the last one is destroyed logging
 * is also stopped.
 */
class Manager
{
private:
	static int instances;

public:
	/**
	 * Create a logging manager and read the @p propertyfile
	 * for configuration information. If the file cannot
	 * be read, a message is printed to stderr and logging
	 * is left unchanged (for the first manager, in whatever
	 * default state the underlying implementation uses).
	 */
	Manager(const std::string& propertyfile)
	{
		try
		{
			log4cpp::PropertyConfigurator::configure(propertyfile);
		}
		catch (log4cpp::ConfigureFailure& e)
		{
			// Logging disabled.
			std::cerr << "No file " << propertyfile << ". Logging disabled.\n";
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

	/**
	 * Gets a named logger (e.g. "app.database.sql"). Use an
	 * empty name to get the root logger (see also getRoot()).
	 */
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

	/**
	 * Gets the root logger (the one with no category). This is
	 * the eventual destination of all logging messages.
	 */
	static
	Logger& getRoot()
	{
		return log4cpp::Category::getRoot();
	}
};
#endif

// Convenience function, use the Manager-implementation (which depends on NDEBUG).
// This is static inline because it's just forwarding.
static inline
Logger& getLogger(const std::string& categoryname) { return Manager::getLogger(categoryname); }

static inline
Logger& getRoot() { return Manager::getRoot(); }

} // namespace Logging
} // namespace

#endif
