#ifndef KMIMESIZESMODEL_H
#define KMIMESIZESMODEL_H

#include <QAbstractListModel>

#include "dir_scanner/KMimeSizesInfo.h"

// Модель для таблицы с размерами по типам файлов
class KMimeSizesModel : public QAbstractListModel
{
public:
    enum { NumColumns = 4 };

    int rowCount(const QModelIndex&) const override;
    int columnCount(const QModelIndex& parent) const override
    {
        return NumColumns;
    }

    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    QVariant data(const QModelIndex& index, int role) const override;

    // Устанавливает новые значения, перенося посредством swap
    void setMimeSizes(KMimeSizesInfo::KMimeSizesList&& values);

private:
    KMimeSizesInfo::KMimeSizesList m_values;
};

#endif // !KMIMESIZESMODEL_H
