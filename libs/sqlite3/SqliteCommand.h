#ifndef SQLITECOMMAND_H
#define SQLITECOMMAND_H

#include <string>
#include "SqliteRecordset.h"

class SqliteDb;

/**
 * Use SqliteDb::prepare() function to create instance of SqliteCommand.
 * Usage:
 * db.prepare(sql)
 *   .addParameter(...)
 *   .addParameter(...)
 *   ...
 *   .execute();
 * or:
 * auto rs = db.prepare(sql)
 *   .addParameter(...)
 *   .addParameter(...)
 *   ...
 *   .select();
 */
class SqliteCommand
{
	friend class SqliteDb;

public:
	~SqliteCommand();

	SqliteCommand& addParameter(int value);
	SqliteCommand& addParameter(long long value);
	SqliteCommand& addParameter(double value);
	SqliteCommand& addParameter(const std::wstring& value);
	SqliteCommand& addParameter(const SqliteRecordset::TDateTime& value);
	SqliteCommand& addParameterBlob(const unsigned char* buf, int bufSize);
	SqliteCommand& addParameterNull();

	void execute();
	SqliteRecordset select();

private:
	SqliteCommand(sqlite3* db, const std::wstring& sql);

	SqliteCommand(const SqliteCommand&) = delete;
	SqliteCommand(SqliteCommand&&) = delete;
	SqliteCommand& operator=(const SqliteCommand&) = delete;
	SqliteCommand& operator=(SqliteCommand&&) = delete;

	sqlite3* m_db;
	sqlite3_stmt* m_preparedStmt;

	int m_parameterCount;

	void checkStatement();
};

#endif // SQLITECOMMAND_H
