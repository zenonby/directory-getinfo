#include "DirectoryStore.h"
#include "utils.h"
#include "settings.h"

DirectoryStore::DirectoryStore()
	: m_db(nullptr)
{
	const auto& dbFileName = Settings::instance()->dbFileName();

	auto rc = sqlite3_open(dbFileName.c_str(), &m_db);
	if (rc)
		throw std::logic_error(sqlite3_errmsg(m_db));
}

DirectoryStore::~DirectoryStore()
{
	sqlite3_close(m_db);
	m_db = nullptr;
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
	const DirectoryDetails& dirDetails)
{
	assert(isUnifiedPath(unifiedPath));

	std::scoped_lock lock_(m_sync);

	auto iter = m_directories.find(unifiedPath);
	if (iter == m_directories.end())
	{
		auto tup = m_directories.emplace(std::make_pair(unifiedPath, DirectoryDetails{
			DirectoryProcessingStatus::Pending, {}, {} }));
		assert(tup.second);
		iter = tup.first;
	}

	DirectoryDetails &existingDirDetails = iter->second;
	existingDirDetails.status = dirDetails.status;

	if (dirDetails.subdirectoryCount.has_value())
		existingDirDetails.subdirectoryCount = dirDetails.subdirectoryCount;

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
