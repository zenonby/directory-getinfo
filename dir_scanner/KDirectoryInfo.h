#ifndef KDIRECTORYINFO_H
#define KDIRECTORYINFO_H

#include <memory>
#include <optional>
#include <QString>
#include "model/DirectoryProcessingStatus.h"

// Directory data without a directory path
struct KDirectoryData
{
	// Directory scanning status
	DirectoryProcessingStatus status = DirectoryProcessingStatus::Pending;

	// Immediate subdirectory count
	std::optional<unsigned long> subDirCount = 0;
};

// Directory tree item
struct KDirectoryInfo : KDirectoryData
{
	// Unified directory path
	QString fullPath;
};

typedef std::shared_ptr<KDirectoryInfo> KDirectoryInfoPtr;

Q_DECLARE_METATYPE(KDirectoryInfoPtr)

#endif // KDIRECTORYINFO_H
