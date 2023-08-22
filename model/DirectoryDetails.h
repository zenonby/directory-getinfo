#ifndef DIRECTORYDETAILS_H
#define DIRECTORYDETAILS_H

#include <optional>

#include "DirectoryProcessingStatus.h"
#include "MimeDetails.h"
#include "DirectoryStats.h"

// Information, collected about a particular directory
struct DirectoryDetails : DirectoryStatsWithStatus
{
	bool scan = false;
	std::optional<TMimeDetailsList> mimeDetailsList = {};

	DirectoryDetails clone(bool cloneMimeDetails) const
	{
		DirectoryDetails retVal{
			{ { .subdirectoryCount = subdirectoryCount,
			    .totalFileCount = totalFileCount,
			    .totalSize = totalSize }, status },
			scan,
			cloneMimeDetails ? mimeDetailsList : std::optional<TMimeDetailsList>{}
		};

		return retVal;
	}
};

#endif // DIRECTORYDETAILS_H
