#ifndef SQLITERECORDSET_H
#define SQLITERECORDSET_H

#include <string>
#include <optional>
#include <chrono>
#include <sstream>

struct sqlite3;
struct sqlite3_stmt;

class SqliteCommand;

/**
 * Sample usage (assume rs is of SqliteRecordset type):
 *		while(rs)
 *		{
 *			// get column data
 *
 *			++rs;
 *		}
 */
class SqliteRecordset
{
	friend class SqliteCommand;

public:
	~SqliteRecordset();

	typedef std::chrono::time_point<std::chrono::utc_clock> TDateTime;

	// Checks if more records are available
	bool operator()() const;

	// Moves to the next record;
	SqliteRecordset& operator++();

	// Checks if specified column value in the current row IS NULL
	bool isNull(int index) const;

	// Returns int value in the specified column in the current row
	std::optional<int> getInt(int index) const;

	// Returns int64 value in the specified column in the current row
	std::optional<long long> getInt64(int index) const;

	// Returns std::wstring value in the specified column in the current row
	std::optional<std::wstring> getWString(int index) const;

	// Returns double value in the specified column in the current row
	std::optional<double> getDouble(int index) const;

	// Returns date/time value in the specified column in the current row
	std::optional<TDateTime> getDateTime(int index) const;

	// Retrieves blob value from the specified column in the current row
	std::optional<std::vector<unsigned char>> getBlob(int index) const;

private:
	SqliteRecordset(sqlite3* db, sqlite3_stmt* preparedStmt, bool valid);

	SqliteRecordset(const SqliteRecordset&) = delete;
	SqliteRecordset(SqliteRecordset&&) = delete;
	SqliteRecordset& operator=(const SqliteRecordset&) = delete;
	SqliteRecordset& operator=(SqliteRecordset&&) = delete;

	// Internal format of datetime value reprsenation as string in database
	inline static const char* const DATE_TIME_FORMAT{ "%Y-%m-%dT%H:%M:%S" };

	sqlite3* m_db;
	sqlite3_stmt* m_preparedStmt;

	// true if more records are available
	bool m_valid;
};

#endif // SQLITERECORDSET_H
