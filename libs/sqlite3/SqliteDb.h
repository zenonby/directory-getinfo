#ifndef SQLITEDB_H
#define SQLITEDB_H

#include <string>
#include "SqliteRecordset.h"
#include "SqliteCommand.h"
#include "SqliteTransaction.h"
#include "SqliteExceptions.h"

struct sqlite3;

/// <summary>
/// Wrapper for sqlite3 library.
/// Not thread safe.
/// </summary>
class SqliteDb
{
public:
	SqliteDb(const std::wstring& dbFileName);
	~SqliteDb();

	// Executes SQL query without adding parameters
	SqliteRecordset select(const std::wstring& sql);

	// Executes non-query SQL without adding parameters
	void execute(const std::wstring& sql);

	/**
	 * Use SqlCommand to add parameters before
	 *	calling SqlCommand::select() or SqlCommand::execute().
	 * Check SqlCommand documentation for more info.
	 */
	SqliteCommand prepare(const std::wstring& sql);

	// Lifetime of a returned instance cannot exceed lifetime of this instance
	SqliteTransaction beginTransaction();

private:
	std::wstring m_dbFileName;
	sqlite3* m_db;

	void checkCreateDatabaseDirectory();
	void open();
	void close();

	// Strips filename from full file path and returns just directory
	static std::wstring getDirectoryFromFilePath(const std::wstring& filePath);
};

#endif // SQLITEDB_H
