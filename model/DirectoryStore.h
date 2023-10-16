#ifndef DIRECTORYSTORE_H
#define DIRECTORYSTORE_H

#include <mutex>
#include <map>
#include <chrono>
#include <QString>

#include "DirectoryDetails.h"

class DirectoryStore
{
public:
	~DirectoryStore();

	static DirectoryStore* instance();

	// if updateDirectoryStats is false, directory stats are not updated
	void upsertDirectory(
		const QString& unifiedPath,
		const DirectoryDetails& dirDetails,
		bool updateDirectoryStats);

	// If fillinMimeSizesOnlyIfReady == true,
	//	DirectoryDetails::mimeDetailsList is filled in
	//	only if scanning of particular directory is complete
	bool tryGetDirectory(
		const QString& unifiedPath,
		bool fillinMimeSizesOnlyIfReady,
		DirectoryDetails& directoryDetails);

	// Returns true if any data (at least for 1 dir) are present
	bool hasData() const;

	/// <summary>
	/// Saves current data (m_directories) to database
	/// </summary>
	void saveCurrentData();

	typedef std::map<
		std::chrono::utc_clock::time_point,
		DirectoryStats
	> TDirectoryStatsHistory;

	/// Retrieves all previously stored directory stats from the store
	TDirectoryStatsHistory getDirectoryStatsHistory(const QString& unifiedPath) const;

private:
	DirectoryStore();
	DirectoryStore(const DirectoryStore&) = delete;
	DirectoryStore& operator=(const DirectoryStore&) = delete;

	mutable std::mutex m_sync;

	std::map<
		QString,	// Unified path
		DirectoryDetails
	> m_directories;

	std::wstring getDbFileName() const;

	void checkCreateDbSchema();
};

#endif // DIRECTORYSTORE_H
