#ifndef SqliteErrorS_H
#define SqliteErrorS_H

#include <string>
#include <stdexcept>

/**
 * Error returned by sqlite3_errmsg
 */
class SqliteError : std::logic_error
{
public:
	explicit SqliteError(const std::string& errorMessage)
		: std::logic_error(errorMessage) { }

	explicit SqliteError(const char* errorMessage)
		: std::logic_error(errorMessage) { }
};

/**
 * Invalid value type requested
 */
class SqliteInvalidTypeError : std::runtime_error
{
public:
	explicit SqliteInvalidTypeError()
		: std::runtime_error("Invalid type") { }
};

/**
 * Invalid value type requested
 */
class SqliteInvalidDateFormatError : std::runtime_error
{
public:
	explicit SqliteInvalidDateFormatError()
		: std::runtime_error("Invalid date/time format representation") { }
};

/**
 * Invalid attempt to execute/query already finished statement
 */
class StatementAlreadyFinishedError : std::logic_error
{
public:
	explicit StatementAlreadyFinishedError()
		: std::logic_error("Statement already finished") { }
};

#endif // SqliteErrorS_H
