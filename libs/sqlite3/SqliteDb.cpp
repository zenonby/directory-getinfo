#include <filesystem>
#include <cassert>
#include "amalgamation/sqlite3.h"
#include "SqliteDb.h"
#include "SqliteExceptions.h"

SqliteDb::SqliteDb(const std::wstring& dbFileName)
	: m_db(nullptr),
    m_dbFileName(dbFileName)
{
    assert(!m_dbFileName.empty());

    checkCreateDatabaseDirectory();
    open();
}

SqliteDb::~SqliteDb()
{
    close();
}

std::wstring
SqliteDb::getDirectoryFromFilePath(const std::wstring& filePath)
{
    auto slashPos = filePath.find_last_of(L"/\\");
    if (std::string::npos == slashPos)
        return L"";

    std::wstring dir = filePath.substr(0, slashPos);
    return dir;
}

void
SqliteDb::checkCreateDatabaseDirectory()
{
    const auto& dbDir = getDirectoryFromFilePath(m_dbFileName);

    // Create directory for DB file if the dir does not exist
    std::error_code err;
    if (!std::filesystem::exists(dbDir) &&
        !std::filesystem::create_directories(dbDir, err))
    {
        throw SqliteError(err.message());
    }
}

void
SqliteDb::open()
{
    int res = sqlite3_open16(m_dbFileName.c_str(), &m_db);
    if (SQLITE_OK != res)
    {
        std::string errorMsg{"Failed to open database"};
        if (nullptr != m_db)
        {
            errorMsg += ", error: ";
            errorMsg += sqlite3_errmsg(m_db);

            sqlite3_close(m_db);
            m_db = nullptr;
        }

        throw SqliteError(errorMsg);
    }

    char* szErrMsg = nullptr;
    res = sqlite3_exec(m_db, "PRAGMA temp_store=MEMORY", NULL, NULL, &szErrMsg);
    if (SQLITE_OK != res)
    {
        assert(0);
        sqlite3_free(szErrMsg);
    }

    szErrMsg = nullptr;
    res = sqlite3_exec(m_db, "PRAGMA journal_mode=MEMORY", NULL, NULL, &szErrMsg);
    if (SQLITE_OK != res)
    {
        assert(0);
        sqlite3_free(szErrMsg);
    }
}

void
SqliteDb::close()
{
    if (nullptr != m_db)
    {
        sqlite3_close(m_db);
        m_db = nullptr;
    }
}

void
SqliteDb::execute(const std::wstring& sql)
{
    prepare(sql)
        .execute();
}

SqliteRecordset
SqliteDb::select(const std::wstring& sql)
{
    return prepare(sql)
        .select();
}

SqliteCommand
SqliteDb::prepare(const std::wstring& sql)
{
    return SqliteCommand(m_db, sql);
}

SqliteTransaction
SqliteDb::beginTransaction()
{
    return SqliteTransaction(this);
}
