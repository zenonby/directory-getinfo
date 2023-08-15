#ifndef KFILESYSTEMMODEL_H
#define KFILESYSTEMMODEL_H

#include <QFileSystemModel>

#include "dir_scanner/KDirectoryInfo.h"

// Directory tree model
class KFileSystemModel : public QFileSystemModel
{
	Q_OBJECT

public:
	KFileSystemModel();

	enum { NumColumns = 3 };

	int columnCount(const QModelIndex& parent = QModelIndex()) const override
	{
		return NumColumns;
	}

	Qt::ItemFlags flags(const QModelIndex& index) const override;
	QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
	QVariant data(const QModelIndex& index, int role) const override;
	bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;

	void SetDirectoryInfo(const KDirectoryInfo& dirInfo);

private:
	std::map<
		QString, // N.B. __Unified__ path. Translate to unified path before insertion !
		KDirectoryData
	> m_dirData;

	// Returns pointer to directory data if the data were addressed and stored previously
	//	or nullptr if data not found
	const KDirectoryData* lookupDirectoryData(const QString& path) const;
	KDirectoryData* lookupDirectoryData(const QString& path);

	// Updates or inserts directory data
	void updateDirectoryData(const QString& unifiedPath, KDirectoryData&& dirData);

	// Emits dataChanged event for the specified path
	void emitDataChanged(const QString& unifiedPath);

	void setDirectoryChecked(const QModelIndex& index_, bool checked);

	static QString translateDirectoryProcessingStatus(DirectoryProcessingStatus status);

private slots:
	void onDirectoryLoaded(QString path);
	void emitDataChangedForChildren(const QModelIndex& parentIndex);
};

#endif // KFILESYSTEMMODEL_H
