#ifndef DIRECTORYSTORE_H
#define DIRECTORYSTORE_H

#include <mutex>
#include <map>
#include <QString>

#include "DirectoryDetails.h"

class DirectoryStore
{
public:
	~DirectoryStore();

	static DirectoryStore* instance();

	// Неуказанные в dirDetails значения (optional) не будут обновлены или удалены
	void upsertDirectory(
		const QString& unifiedPath,
		const DirectoryDetails& dirDetails);

	// Если fillinMimeSizesOnlyIfReady == true,
	//	DirectoryDetails::mimeDetailsList будет заполнена
	//	только если сканирование указанной директории завершено.
	bool tryGetDirectory(
		const QString& unifiedPath,
		bool fillinMimeSizesOnlyIfReady,
		DirectoryDetails& directoryDetails);

private:
	DirectoryStore();
	DirectoryStore(const DirectoryStore&) = delete;
	DirectoryStore& operator=(const DirectoryStore&) = delete;

	mutable std::mutex m_sync;

	std::map<
		QString,	// Унифицированный путь
		DirectoryDetails
	> m_directories;
};

#endif // DIRECTORYSTORE_H
