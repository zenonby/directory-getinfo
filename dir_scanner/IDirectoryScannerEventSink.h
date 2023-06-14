#ifndef IDIRECTORYSCANNEREVENTSINK_H
#define IDIRECTORYSCANNEREVENTSINK_H

#include "KDirectoryInfo.h"
#include "KMimeSizesInfo.h"

// Потребитель сообщений о сканировании директорий
struct IDirectoryScannerEventSink
{
	virtual ~IDirectoryScannerEventSink() = default;

	virtual void onUpdateDirectoryInfo(KDirectoryInfoPtr pInfo) = 0;

	virtual void onUpdateMimeSizes(KMimeSizesInfoPtr pInfo) = 0;
};

#endif // IDIRECTORYSCANNEREVENTSINK_H
