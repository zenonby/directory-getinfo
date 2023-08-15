#ifndef IDIRECTORYSCANNEREVENTSINK_H
#define IDIRECTORYSCANNEREVENTSINK_H

#include "KDirectoryInfo.h"
#include "KMimeSizesInfo.h"

// Data scanning event subscribers/sinks
struct IDirectoryScannerEventSink
{
	virtual ~IDirectoryScannerEventSink() = default;

	virtual void onUpdateDirectoryInfo(KDirectoryInfoPtr pInfo) = 0;

	virtual void onUpdateMimeSizes(KMimeSizesInfoPtr pInfo) = 0;

	virtual void onWorkerException(std::exception_ptr&& pEx) = 0;
};

#endif // IDIRECTORYSCANNEREVENTSINK_H
