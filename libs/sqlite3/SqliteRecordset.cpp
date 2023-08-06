#include <cassert>
#include <algorithm>
#include "amalgamation/sqlite3.h"
#include "SqliteRecordset.h"
#include "SqliteExceptions.h"

SqliteRecordset::SqliteRecordset(sqlite3* db, sqlite3_stmt* preparedStmt, bool valid)
	: m_db(db),
	  m_preparedStmt(preparedStmt),
	  m_valid(valid)
{
}

SqliteRecordset::~SqliteRecordset()
{
	if (m_preparedStmt)
	{
		sqlite3_finalize(m_preparedStmt);
		m_preparedStmt = nullptr;
	}
}

bool
SqliteRecordset::operator()() const
{
	return m_valid;
}

SqliteRecordset&
SqliteRecordset::operator++()
{
	auto res = sqlite3_step(m_preparedStmt);
	assert(SQLITE_BUSY != res && SQLITE_LOCKED != res && !"SQLITE_BUSY, SQLITE_LOCKED are not handled since concurent access is not supported");
	assert(SQLITE_ROW == res || SQLITE_DONE == res);

	m_valid = SQLITE_ROW == res;

	return *this;
}

bool
SqliteRecordset::isNull(int index) const
{
	const auto type = sqlite3_column_type(m_preparedStmt, index);
	bool res = SQLITE_NULL == type;
	return res;
}

std::optional<int>
SqliteRecordset::getInt(int index) const
{
	const auto type = sqlite3_column_type(m_preparedStmt, index);

	if (SQLITE_NULL == type)
		return std::optional<int>();
	else if (SQLITE_INTEGER != type)
		throw SqliteInvalidTypeError();

	auto value = sqlite3_column_int(m_preparedStmt, index);
	return value;
}

std::optional<long long>
SqliteRecordset::getInt64(int index) const
{
	const auto type = sqlite3_column_type(m_preparedStmt, index);

	if (SQLITE_NULL == type)
		return std::optional<long long>();
	else if (SQLITE_INTEGER != type)
		throw SqliteInvalidTypeError();

	auto value = sqlite3_column_int64(m_preparedStmt, index);
	return value;
}

std::optional<std::wstring>
SqliteRecordset::getWString(int index) const
{
	const auto type = sqlite3_column_type(m_preparedStmt, index);

	if (SQLITE_NULL == type)
		return std::optional<std::wstring>();
	else if (SQLITE_TEXT != type)
		throw SqliteInvalidTypeError();

	auto szValue = sqlite3_column_text16(m_preparedStmt, index);
	std::wstring value(reinterpret_cast<const wchar_t*>(szValue));

	return value;
}

std::optional<double>
SqliteRecordset::getDouble(int index) const
{
	const auto type = sqlite3_column_type(m_preparedStmt, index);

	if (SQLITE_NULL == type)
		return std::optional<double>();
	else if (SQLITE_FLOAT != type)
		throw SqliteInvalidTypeError();

	auto value = sqlite3_column_double(m_preparedStmt, index);
	return value;
}

std::optional<SqliteRecordset::TDateTime>
SqliteRecordset::getDateTime(int index) const
{
	const auto type = sqlite3_column_type(m_preparedStmt, index);

	if (SQLITE_NULL == type)
		return std::optional<TDateTime>();
	else if (SQLITE_TEXT != type)
		throw SqliteInvalidTypeError();

	// Get text value
	auto szValue = sqlite3_column_text(m_preparedStmt, index);
	std::string sValue(reinterpret_cast<const char*>(szValue));
	std::stringstream st(sValue);

	// Parse text value into date/time
	TDateTime value;
	if (!std::chrono::from_stream(st, DATE_TIME_FORMAT, value))
		throw SqliteInvalidDateFormatError();

	return value;
}

std::optional<std::vector<unsigned char>>
SqliteRecordset::getBlob(int index) const
{
	const auto type = sqlite3_column_type(m_preparedStmt, index);

	if (SQLITE_NULL == type)
		return std::optional<std::vector<unsigned char>>();
	else if (SQLITE_TEXT != type)
		throw SqliteInvalidTypeError();

	auto bufSize = sqlite3_column_bytes(m_preparedStmt, index);
	auto buf = reinterpret_cast<const unsigned char*>(sqlite3_column_blob(m_preparedStmt, index));

	// Copy buffer
	std::vector<unsigned char> value(bufSize);
	memcpy(value.data(), buf, bufSize);

	return value;
}
