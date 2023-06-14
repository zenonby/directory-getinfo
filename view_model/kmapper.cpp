#include <algorithm>
#include "kmapper.h"

void
KMapper::mapTMimeDetailsListToKMimeSizesList(
	const TMimeDetailsList& mimeDetailsList,
	KMimeSizesInfo::KMimeSizesList& mimeSizes)
{
    mimeSizes.reserve(static_cast<int>(mimeDetailsList.size()));

    // Отобразить mimeDetailsList в элементы mimeSizes
    std::transform(mimeDetailsList.cbegin(), mimeDetailsList.cend(),
        std::back_inserter(mimeSizes), [](auto iter) {
            KMimeSize rv;
            auto& mimeDetails_ = iter.second;

            rv.mimeType = iter.first;
            rv.fileCount = mimeDetails_.fileCount;
            rv.totalSize = mimeDetails_.totalSize;
            rv.avgSize = mimeDetails_.fileCount ?
                static_cast<float>(mimeDetails_.totalSize) / mimeDetails_.fileCount : 0;

            return rv;
        });

    // Перенести TMimeDetailsList::ALL_MIMETYPE в начало
    for (auto iter = mimeSizes.begin(); iter != mimeSizes.end(); ++iter)
    {
        const auto mimeSize = *iter;
        if (mimeSize.mimeType == TMimeDetailsList::ALL_MIMETYPE)
        {
            mimeSizes.erase(iter);
            mimeSizes.push_front(mimeSize);
            break;
        }
    }
}
