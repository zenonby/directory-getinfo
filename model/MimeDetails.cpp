#include "MimeDetails.h"

TMimeDetailsList::TMimeDetailsList()
{
    addMimeDetails(ALL_MIMETYPE, 0, 0);
}

void
TMimeDetailsList::addMimeDetails(
    QString mimeType,
    unsigned long long totalSize,
    unsigned long fileCount)
{
    auto iter = find(mimeType);
    if (iter == end())
    {
        MimeDetails md;
        md.totalSize = totalSize;
        md.fileCount = fileCount;
        emplace(std::make_pair(mimeType, md));
    }
    else
    {
        iter->second.totalSize += totalSize;
        iter->second.fileCount += fileCount;
    }
}

void
TMimeDetailsList::addMimeDetails(const TMimeDetailsList& mimeDetailsList)
{
    for (const auto& mt : mimeDetailsList)
    {
        addMimeDetails(mt.first, mt.second.totalSize, mt.second.fileCount);
    }
}
