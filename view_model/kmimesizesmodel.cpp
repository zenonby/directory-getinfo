#include <QFont>
#include "kmimesizesmodel.h"

void
KMimeSizesModel::setFileSizeDivisor(FileSizeDivisor divisor)
{
    bool changed = m_divisor != divisor;
    m_divisor = divisor;

    if (changed)
    {
        emit dataChanged(index(0, 0), index(m_values.count() - 1, NumColumns - 1));
        emit headerDataChanged(Qt::Horizontal, 0, NumColumns - 1);
    }
}

QString
KMimeSizesModel::getDivisorSuffix() const noexcept
{
    switch (m_divisor)
    {
    case FileSizeDivisor::Bytes:
        return tr("B");
    case FileSizeDivisor::KBytes:
        return tr("KB");
    case FileSizeDivisor::MBytes:
        return tr("MB");
    default:
        assert(!"Unexpected FileSizeDivisor value");
    }

    return "";
}

unsigned int
KMimeSizesModel::getDivisorValue() const noexcept
{
    switch (m_divisor)
    {
    case FileSizeDivisor::Bytes:
        return 1;
    case FileSizeDivisor::KBytes:
        return 1024;
    case FileSizeDivisor::MBytes:
        return 1024 * 1024;
    default:
        assert(!"Unexpected FileSizeDivisor value");
    }

    return 1;
}

int
KMimeSizesModel::rowCount(const QModelIndex&) const
{
	return m_values.count();
}

QVariant
KMimeSizesModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    switch (role) {
    case Qt::DecorationRole:
        return QVariant();
    case Qt::TextAlignmentRole:
        return Qt::AlignHCenter;
    }

    if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
        return QAbstractItemModel::headerData(section, orientation, role);

    QString returnValue;

    switch (section) {
    case 0:
        returnValue = tr("Mime type");
        break;
    case 1:
        returnValue = tr("File count");
        break;
    case 2:
        returnValue = tr("Total size, ") + getDivisorSuffix();
        break;
    case 3: returnValue =
        returnValue = tr("Avg size, ") + getDivisorSuffix();
        break;
    default:
        assert(!"Unexpected");
        return QVariant();
    }

    return returnValue;
}

QVariant
KMimeSizesModel::data(const QModelIndex& index, int role) const
{
    const int colIndex = index.column();

    if (Qt::TextAlignmentRole == role)
        return 0 == colIndex ? Qt::AlignLeft : Qt::AlignRight;

    if (Qt::FontRole == role &&
        0 == index.row())
    {
        QFont font;
        font.setBold(true);
        return font;
    }

    unsigned int divisorValue = getDivisorValue();

    if (Qt::DisplayRole == role &&
        index.isValid())
    {
        const int rowIndex = index.row();

        assert(rowIndex < m_values.count());
        const KMimeSize& row = m_values[rowIndex];

        switch (colIndex)
        {
        case 0:
            return row.mimeType;
        case 1:
            return QString("%L1").arg(static_cast<unsigned long long>(row.fileCount));
        case 2:
            return QString("%L1").arg(row.totalSize / divisorValue);
        case 3:
            return QString("%L1").arg(round(row.avgSize / static_cast<float>(divisorValue) * 1000) / 1000.f);
        default:
            assert(!"Unexpected");
            return QVariant();
        }
    }

    return QVariant();
}

void
KMimeSizesModel::setMimeSizes(KMimeSizesInfo::KMimeSizesList&& values)
{
    const int oldRowCount = m_values.count();
    const int newRowCount = values.count();

    const bool rowsAdded = newRowCount > oldRowCount;
    const bool rowsRemoved = newRowCount < oldRowCount;

    if (rowsAdded)
        beginInsertRows(index(oldRowCount), oldRowCount, newRowCount - 1);
    else if (rowsRemoved)
        beginRemoveRows(QModelIndex(), 0, oldRowCount - newRowCount - 1);

    m_values.swap(values);

    if (0 < newRowCount)
        emit dataChanged(index(0, 0), index(newRowCount - 1, NumColumns - 1));

    if (rowsAdded)
        endInsertRows();
    else if (rowsRemoved)
        endRemoveRows();
}
