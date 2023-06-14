#ifndef KFILESYSTEMMODEL_H
#define KFILESYSTEMMODEL_H

#include <QFileSystemModel>

#include "dir_scanner/KDirectoryInfo.h"

// Модель для дерева директорий
class KFileSystemModel : public QFileSystemModel
{
public:
	KFileSystemModel();

	enum { NumColumns = 3 };

	int columnCount(const QModelIndex& parent = QModelIndex()) const override
	{
		return NumColumns;
	}

	QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
	QVariant data(const QModelIndex& index, int role) const override;

	void SetDirectoryInfo(const KDirectoryInfo& dirInfo);

private:
	std::map<
		QString, // Перед вставкой привести к общему виду посредством getUnifiedPathName
		KDirectoryData
	> m_dirData;

	static QString translateDirectoryProcessingStatus(DirectoryProcessingStatus status);
};

#endif // KFILESYSTEMMODEL_H
