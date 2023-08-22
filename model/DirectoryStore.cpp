#include <yasw/SqliteDb.h>
#include "DirectoryStore.h"
#include "utils.h"
#include "settings.h"

#define SQL_TABLE_SNAPSHOTS L"snapshots"
#define SQL_TABLE_DIRECTORIES L"directories"

DirectoryStore::DirectoryStore()
{
}

DirectoryStore::~DirectoryStore()
{
}

DirectoryStore*
DirectoryStore::instance()
{
	static DirectoryStore s_instance;
	return &s_instance;
}

void
DirectoryStore::upsertDirectory(
	const QString& unifiedPath,
	const DirectoryDetails& dirDetails,
	bool updateDirectoryStats)
{
	assert(isUnifiedPath(unifiedPath));

	std::scoped_lock lock_(m_sync);

	auto iter = m_directories.find(unifiedPath);
	if (iter == m_directories.end())
	{
		auto tup = m_directories.emplace(std::make_pair(unifiedPath, DirectoryDetails{
			{ .status = DirectoryProcessingStatus::Pending } }));
		assert(tup.second);
		iter = tup.first;
	}

	DirectoryDetails &existingDirDetails = iter->second;
	existingDirDetails.status = dirDetails.status;

	if (updateDirectoryStats)
		existingDirDetails.DirectoryStats::assignStats(dirDetails);

	if (dirDetails.mimeDetailsList.has_value())
		existingDirDetails.mimeDetailsList = dirDetails.mimeDetailsList;
}

bool
DirectoryStore::tryGetDirectory(
	const QString& unifiedPath,
	bool fillinMimeSizesOnlyIfReady,
	DirectoryDetails& directoryDetails)
{
	assert(isUnifiedPath(unifiedPath));

	std::scoped_lock lock_(m_sync);

	const auto iter = m_directories.find(unifiedPath);
	if (iter == m_directories.end())
		return false;

	const auto& dirDetails = iter->second;
	bool ready = dirDetails.status == DirectoryProcessingStatus::Ready;

	directoryDetails = dirDetails.clone(ready || !fillinMimeSizesOnlyIfReady);
	return true;
}

std::wstring
DirectoryStore::getDbFileName() const
{
	const auto& dbFileName = Settings::instance()->dbFileName();
	return dbFileName;
}

void
DirectoryStore::checkCreateDbSchema()
{
	const auto& dbFileName = getDbFileName();
	SqliteDb db(dbFileName);

	const auto sqlCheckTable = L"SELECT COUNT(*) FROM sqlite_master WHERE type='table' AND name=?";

	// snapshots table
	auto rs1 = db
		.prepare(sqlCheckTable)
		.addParameter(SQL_TABLE_SNAPSHOTS)
		.select();

	int rowCount = rs1.getInt(0).value();
	if (0 == rowCount)
	{
		db.execute(L"CREATE TABLE " SQL_TABLE_SNAPSHOTS L" (\n"
			L"id INTEGER PRIMARY KEY AUTOINCREMENT,\n"
			L"date_time TEXT NOT NULL\n"
			L")");
	}

	// directories table
	auto rs2 = db
		.prepare(sqlCheckTable)
		.addParameter(SQL_TABLE_DIRECTORIES)
		.select();

	rowCount = rs2.getInt(0).value();
	if (0 == rowCount)
	{
		db.execute(L"CREATE TABLE " SQL_TABLE_DIRECTORIES L" (\n"
			L"id INTEGER PRIMARY KEY,\n"
			L"snapshot_id INTEGER NOT NULL,\n"
			L"path TEXT NOT NULL,\n"
			L"total_file_count INTEGER NOT NULL,\n"
			L"total_size INTEGER NOT NULL,\n"
			L"FOREIGN KEY (snapshot_id) REFERENCES " SQL_TABLE_SNAPSHOTS L"(id)\n"
			L")");
	}
}

bool
DirectoryStore::hasData() const
{
	std::scoped_lock lock_(m_sync);
	bool res = !m_directories.empty();
	return res;
}

void
DirectoryStore::saveCurrentData()
{
	std::scoped_lock lock_(m_sync);

	const auto& dbFileName = getDbFileName();
	SqliteDb db(dbFileName);
	auto transaction = db.beginTransaction();

	try
	{
		// Create new snapshot
		auto rs = db.prepare(
			L"INSERT INTO " SQL_TABLE_SNAPSHOTS L" (date_time) VALUES (?); "
			L"SELECT MAX(last_insert_rowid()) FROM " SQL_TABLE_SNAPSHOTS)
			.addParameter(std::chrono::utc_clock::now())
			.select();

		// Get snapshot id
		const int snapshotId = rs.getInt(0).value();

		const auto sqlInsertDir =
			L"INSERT INTO " SQL_TABLE_DIRECTORIES L" (snapshot_id, path, total_file_count, total_size) "
			L"VALUES (?, ?, ?, ?)";

		// Add directory data
		for (auto iter = m_directories.cbegin(); iter != m_directories.cend(); ++iter)
		{
			const auto& unifiedPath = iter->first;
			const auto& dirDetails = iter->second;

			if (!dirDetails.mimeDetailsList.has_value())
				continue;

			// Get details
			auto mimeDetailsList = dirDetails.mimeDetailsList.value();
			auto mimeDetails = mimeDetailsList[TMimeDetailsList::ALL_MIMETYPE];

			auto cmd = std::move(db.prepare(sqlInsertDir)
				.addParameter(snapshotId)
				.addParameter(unifiedPath.toStdWString()));

			if (dirDetails.totalFileCount.has_value())
				cmd.addParameter(static_cast<long long>(dirDetails.totalFileCount.value()));
			else
				cmd.addParameterNull();

			if (dirDetails.totalSize.has_value())
				cmd.addParameter(static_cast<long long>(dirDetails.totalSize.value()));
			else
				cmd.addParameterNull();

			cmd.execute();
		}

		transaction.commit();
	}
	catch (...)
	{
		transaction.rollback();
		throw;
	}
}
