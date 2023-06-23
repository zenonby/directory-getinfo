#ifndef DIRECTORYDETAILS_H
#define DIRECTORYDETAILS_H

#include <optional>

#include "DirectoryProcessingStatus.h"
#include "MimeDetails.h"

struct DirectoryDetails
{
	DirectoryProcessingStatus status;
	bool scan = false;
	std::optional<unsigned long> subdirectoryCount = {};
	std::optional<TMimeDetailsList> mimeDetailsList = {};

	DirectoryDetails clone(bool cloneMimeDetails) const
	{
		DirectoryDetails retVal{
			status, scan, subdirectoryCount,
			cloneMimeDetails ? mimeDetailsList : std::optional<TMimeDetailsList>{}
		};
		return retVal;
	}
};

#endif // DIRECTORYDETAILS_H
