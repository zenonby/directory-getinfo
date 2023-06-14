#ifndef KMIMESIZE_H
#define KMIMESIZE_H

#include <memory>
#include <QString>
#include <QList>

// Суммарный и средний размер файлов определенного типа.
// Запись в KMimeSizesModel.
struct KMimeSize
{
    QString mimeType;
    unsigned long long totalSize = 0;
    float avgSize = 0;
};

struct KMimeSizesInfo
{
    typedef QList<KMimeSize> KMimeSizesList;

    // Полный путь директории
    QString fullPath;

    KMimeSizesList mimeSizes;
};

typedef std::shared_ptr<KMimeSizesInfo> KMimeSizesInfoPtr;

Q_DECLARE_METATYPE(KMimeSizesInfoPtr)

#endif // KMIMESIZE_H
