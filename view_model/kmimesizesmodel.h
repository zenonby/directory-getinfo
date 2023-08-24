#ifndef KMIMESIZESMODEL_H
#define KMIMESIZESMODEL_H

#include <QAbstractListModel>
#include "dir_scanner/KMimeSizesInfo.h"
#include "FileSizeDivisor.h"

// Model for a table of MIME type total sizes
class KMimeSizesModel : public QAbstractListModel
{
public:
    enum { NumColumns = 4 };

    void setFileSizeDivisor(FileSizeDivisor divisor);

    int rowCount(const QModelIndex&) const override;
    int columnCount(const QModelIndex& parent) const override
    {
        return NumColumns;
    }

    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    QVariant data(const QModelIndex& index, int role) const override;

    // Sets new values by transferring via swap
    void setMimeSizes(KMimeSizesInfo::KMimeSizesList&& values);

private:
    FileSizeDivisor m_divisor = FileSizeDivisor::Bytes;

    KMimeSizesInfo::KMimeSizesList m_values;
};

#endif // !KMIMESIZESMODEL_H
