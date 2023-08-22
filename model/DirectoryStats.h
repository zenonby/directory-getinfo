#ifndef DIRECTORYSTATS_H
#define DIRECTORYSTATS_H

#include "DirectoryProcessingStatus.h"

template <typename T>
std::optional<T>& operator+=(std::optional<T>& lhs, const std::optional<T>& rhs)
{
	if (rhs.has_value())
	{
		if (!lhs.has_value())
			lhs = rhs;
		else
			lhs = lhs.value() + rhs.value();
	}

	return lhs;
}

struct DirectoryStats
{
	// Immediate subdirectory count
	std::optional<unsigned long> subdirectoryCount = {};

	// Total file count (recursively with subdirectories)
	std::optional<unsigned long> totalFileCount = {};

	// Total file size (recursively with subdirectories)
	std::optional<unsigned long long> totalSize = {};

	void assignStats(const DirectoryStats& rhs)
	{
		subdirectoryCount = rhs.subdirectoryCount;
		totalFileCount = rhs.totalFileCount;
		totalSize = rhs.totalSize;
	}

	void addStats(const DirectoryStats& rhs)
	{
		subdirectoryCount += rhs.subdirectoryCount;
		totalFileCount += rhs.totalFileCount;
		totalSize += rhs.totalSize;
	}
};

struct DirectoryStatsWithStatus : DirectoryStats
{
	// Directory scanning status
	DirectoryProcessingStatus status = DirectoryProcessingStatus::Pending;

	void assignStatsWithStatus(const DirectoryStatsWithStatus& rhs)
	{
		DirectoryStats::assignStats(rhs);
		status = rhs.status;
	}
};

#endif // DIRECTORYSTATS_H
