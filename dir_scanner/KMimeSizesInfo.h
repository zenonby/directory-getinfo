#ifndef KMIMESIZE_H
#define KMIMESIZE_H

#include <memory>
#include <QString>
#include <QList>

// Information (total, average file size, etc) related to a particular MIME type
// A record in KMimeSizesModel.
struct KMimeSize
{
    QString mimeType;
    unsigned long fileCount = 0;
    unsigned long long totalSize = 0;
    float avgSize = 0;
};

struct KMimeSizesInfo
{
    typedef QList<KMimeSize> KMimeSizesList;

    // Directory full (unified) path
    QString fullPath;

    KMimeSizesList mimeSizes;
};

typedef std::shared_ptr<KMimeSizesInfo> KMimeSizesInfoPtr;

Q_DECLARE_METATYPE(KMimeSizesInfoPtr)

#endif // KMIMESIZE_H
