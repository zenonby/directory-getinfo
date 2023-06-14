#include <QDebug>
#include "kfilesystemmodel.h"
#include "utils.h"

KFileSystemModel::KFileSystemModel()
{
	QString rootPath;
#ifndef Q_OS_WIN
	rootPath = QDir::rootPath();
#endif // !Q_OS_WIN

#if _DEBUG
//	rootPath = "";
#endif

	setRootPath(rootPath);

	setFilter(
		QDir::Dirs |
		QDir::Drives |
		QDir::NoSymLinks |
		QDir::NoDotAndDotDot);
}

QVariant
KFileSystemModel::data(const QModelIndex& index, int role) const
{
	const int columnIndex = index.column();

	if (Qt::DisplayRole == role &&
		0 < columnIndex &&
		index.isValid())
	{
		QVariant val;

		// Привести путь к общему виду
		const QString& fullPath = getUnifiedPathName(filePath(index));

		// Найти данные о директории
		auto iter = m_dirData.find(fullPath);

		// Найти данные о директории, если были обновлены ранее
		const KDirectoryData* dirData = nullptr;
		if (iter != m_dirData.end())
			dirData = &iter->second;

		switch (columnIndex)
		{
		case 1:
			return nullptr != dirData && dirData->subDirCount.has_value() ?
				QVariant(static_cast<unsigned long long>(dirData->subDirCount.value())) :
				QVariant();
			break;
		case 2:
			return translateDirectoryProcessingStatus(
				nullptr != dirData ? dirData->status : DirectoryProcessingStatus::Pending);
			break;
		default:
			assert(!"Unexpected");
		}

		return val;
	}
	else
	{
		// Базовая реализация
		return QFileSystemModel::data(index, role);
	}
}

QVariant
KFileSystemModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (Qt::Horizontal == orientation &&
		Qt::DisplayRole == role)
	{
		QString returnValue;

		switch (section) {
		case 0:
			returnValue = tr("Name");
			break;
		case 1:
			returnValue = tr("Subdirs");
			break;
		case 2: returnValue =
			returnValue = tr("Status");
			break;
		default:
			assert(!"Unexpected");
			return QVariant();
		}

		return returnValue;
	}
	else
	{
		// Базовая реализация
		return QFileSystemModel::headerData(section, orientation, role);
	}
}

QString
KFileSystemModel::translateDirectoryProcessingStatus(DirectoryProcessingStatus status)
{
	QString sStatus;

	switch (status)
	{
	case DirectoryProcessingStatus::Pending:
		sStatus = tr("");
		break;
	case DirectoryProcessingStatus::Scanning:
		sStatus = tr("Scanning...");
		break;
	case DirectoryProcessingStatus::Ready:
		sStatus = tr("Ready");
		break;
	case DirectoryProcessingStatus::Error:
		sStatus = tr("Error");
		break;
	default:
		assert(!"Unexpected value");
	}

	return sStatus;
}

void
KFileSystemModel::SetDirectoryInfo(const KDirectoryInfo& dirInfo)
{
	// Привести путь к общему виду
	const QString& fullPath = getUnifiedPathName(dirInfo.fullPath);

	// Обновить данные о директории
	auto iter = m_dirData.find(fullPath);
	if (iter != m_dirData.end())
		iter->second = static_cast<KDirectoryData>(dirInfo);
	else
		m_dirData.insert(std::make_pair(fullPath, static_cast<KDirectoryData>(dirInfo)));

	// Уведомить view
	auto dirIndex = index(fullPath);
	if (dirIndex.isValid())
	{
		emit dataChanged(dirIndex.siblingAtColumn(1), dirIndex.siblingAtColumn(2));
	}
}
