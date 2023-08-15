#ifndef KMAPPER_H
#define KMAPPER_H

#include "model/MimeDetails.h"
#include "dir_scanner/KMimeSizesInfo.h"

// Maps model entities to DTO view-model
struct KMapper
{
	static void mapTMimeDetailsListToKMimeSizesList(
		const TMimeDetailsList& mimeDetailsList,
		KMimeSizesInfo::KMimeSizesList& mimeSizes);
};

#endif // KMAPPER_H
