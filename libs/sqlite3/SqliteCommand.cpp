#include <cassert>
#include <format>
#include "amalgamation/sqlite3.h"
#include "SqliteCommand.h"
#include "SqliteExceptions.h"

SqliteCommand::SqliteCommand(sqlite3* db, const std::wstring& sql)
	: m_db(db),
	  m_preparedStmt(nullptr),
	  m_parameterCount(0)
{
	assert(m_db);

	const void* wszTail = nullptr;
	auto res = sqlite3_prepare16_v2(m_db, sql.c_str(), -1, &m_preparedStmt, &wszTail);
	if (SQLITE_OK != res)
		throw SqliteError(sqlite3_errmsg(m_db));
}

SqliteCommand::~SqliteCommand()
{
	if (m_preparedStmt)
	{
		sqlite3_finalize(m_preparedStmt);
		m_preparedStmt = nullptr;
	}
}

void
SqliteCommand::checkStatement()
{
	if (nullptr == m_preparedStmt)
	{
		assert(0);
		throw StatementAlreadyFinishedError();
	}
}

void
SqliteCommand::execute()
{
	checkStatement();

	auto res = sqlite3_step(m_preparedStmt);
	if (SQLITE_DONE != res)
	{
		std::string errMsg = sqlite3_errmsg(m_db);
		sqlite3_finalize(m_preparedStmt);

		throw SqliteError(errMsg);
	}

	// Finalize statement in order to prevent further attempts to execute
	sqlite3_finalize(m_preparedStmt);
	m_preparedStmt = nullptr;
}

SqliteRecordset
SqliteCommand::select()
{
	auto res = sqlite3_step(m_preparedStmt);
	if (SQLITE_DONE != res &&
		SQLITE_ROW != res)
	{
		std::string errMsg = sqlite3_errmsg(m_db);
		sqlite3_finalize(m_preparedStmt);

		throw SqliteError(errMsg);
	}

	// Ownership of m_preparedStmt is being transferred to SqliteRecordset
	auto preparedStmt = m_preparedStmt;
	m_preparedStmt = nullptr;

	return SqliteRecordset(m_db, preparedStmt, SQLITE_ROW == res);
}

SqliteCommand&
SqliteCommand::addParameter(int value)
{
	checkStatement();

	auto res = sqlite3_bind_int(m_preparedStmt, ++m_parameterCount, value);
	if (SQLITE_OK != res)
		throw SqliteError(sqlite3_errmsg(m_db));

	return *this;
}

SqliteCommand&
SqliteCommand::addParameter(long long value)
{
	checkStatement();

	auto res = sqlite3_bind_int64(m_preparedStmt, ++m_parameterCount, value);
	if (SQLITE_OK != res)
		throw SqliteError(sqlite3_errmsg(m_db));

	return *this;
}

SqliteCommand&
SqliteCommand::addParameter(double value)
{
	checkStatement();

	auto res = sqlite3_bind_double(m_preparedStmt, ++m_parameterCount, value);
	if (SQLITE_OK != res)
		throw SqliteError(sqlite3_errmsg(m_db));

	return *this;
}

SqliteCommand&
SqliteCommand::addParameter(const std::wstring& value)
{
	checkStatement();

	auto res = sqlite3_bind_text16(
		m_preparedStmt, ++m_parameterCount, value.c_str(), -1, SQLITE_TRANSIENT);

	if (SQLITE_OK != res)
		throw SqliteError(sqlite3_errmsg(m_db));

	return *this;
}

SqliteCommand&
SqliteCommand::addParameter(const SqliteRecordset::TDateTime& value)
{
	checkStatement();

	// Store as string
	std::string sValue = std::format(SqliteRecordset::DATE_TIME_FORMAT, value);

	auto res = sqlite3_bind_text(
		m_preparedStmt, ++m_parameterCount, sValue.c_str(), -1, SQLITE_TRANSIENT);

	if (SQLITE_OK != res)
		throw SqliteError(sqlite3_errmsg(m_db));

	return *this;
}

SqliteCommand&
SqliteCommand::addParameterBlob(const unsigned char* buf, int bufSize)
{
	checkStatement();

	auto res = sqlite3_bind_blob(
		m_preparedStmt, ++m_parameterCount, buf, bufSize, SQLITE_TRANSIENT);

	if (SQLITE_OK != res)
		throw SqliteError(sqlite3_errmsg(m_db));

	return *this;
}

SqliteCommand&
SqliteCommand::addParameterNull()
{
	checkStatement();

	auto res = sqlite3_bind_null(m_preparedStmt, ++m_parameterCount);
	if (SQLITE_OK != res)
		throw SqliteError(sqlite3_errmsg(m_db));

	return *this;
}
