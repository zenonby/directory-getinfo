#ifndef KDIRECTORYINFO_H
#define KDIRECTORYINFO_H

#include <memory>
#include <optional>
#include <QString>
#include "model/DirectoryStats.h"

// Directory data without a directory path
struct KDirectoryData : DirectoryStatsWithStatus
{
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
