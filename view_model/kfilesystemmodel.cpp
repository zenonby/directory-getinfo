#include <QDebug>
#include "kfilesystemmodel.h"
#include "model/DirectoryScanSwitch.h"
#include "utils.h"

KFileSystemModel::KFileSystemModel()
{
	QString rootPath;
#ifndef Q_OS_WIN
	rootPath = QDir::rootPath();
#endif // !Q_OS_WIN

#if _DEBUG
	rootPath = "W:/Temp/5";
#endif

	setRootPath(rootPath);

	setFilter(
		QDir::Dirs |
		QDir::Drives |
		QDir::NoSymLinks |
		QDir::NoDotAndDotDot);

	connect(this, SIGNAL(directoryLoaded(QString)), this, SLOT(onDirectoryLoaded(QString)));
}

Qt::ItemFlags
KFileSystemModel::flags(const QModelIndex& index) const
{
	if (!index.isValid())
		return 0;

	Qt::ItemFlags flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;

	if (index.column() == 0)
		flags |= Qt::ItemIsUserCheckable;

	return flags;
}

QVariant
KFileSystemModel::data(const QModelIndex& index, int role) const
{
	const int columnIndex = index.column();

	if (role == Qt::CheckStateRole &&
		index.isValid() &&
		index.column() == 0)
	{
		const auto& path = getUnifiedPathName(filePath(index));
		bool enabled = DirectoryScanSwitch::instance()->isEnabled(path);
		return static_cast<int>(enabled ? Qt::Checked : Qt::Unchecked);
	}

	if (Qt::DisplayRole == role &&
		index.isValid() &&
		0 < columnIndex)
	{
		QVariant val;

		// Найти данные о директории, если были обновлены ранее
		auto dirData = lookupDirectoryData(filePath(index));

		switch (columnIndex)
		{
		case 1:
			return nullptr != dirData && dirData->subDirCount.has_value() ?
				QString("%L1").arg(static_cast<unsigned long long>(dirData->subDirCount.value())) :
				QString();
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

bool
KFileSystemModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
	if (role == Qt::CheckStateRole &&
		index.isValid() &&
		index.column() == 0)
	{
		assert(value.canConvert<bool>());
		bool checked = value.toBool();

		setDirectoryChecked(index, checked);

		return true;
	}

	return QFileSystemModel::setData(index, value, role);
}

void
KFileSystemModel::setDirectoryChecked(const QModelIndex& index_, bool checked)
{
	if (!index_.isValid())
		return;

	// Найти данные о директории, если были обновлены ранее
	const auto& fullPath = getUnifiedPathName(filePath(index_));
	DirectoryScanSwitch::instance()->setEnabled(fullPath, checked);

	emit dataChanged(index_, index_);

	// Also check/uncheck children
	QMetaObject::invokeMethod(
		this, "emitDataChangedForChildren", Qt::QueuedConnection,
		Q_ARG(const QModelIndex&, index_));
}

void
KFileSystemModel::emitDataChangedForChildren(const QModelIndex& parentIndex)
{
	int rowCount_ = rowCount(parentIndex);
	for (int i = 0; i < rowCount_; ++i)
	{
		auto index_ = parentIndex.child(i, 0);
		emit dataChanged(index_, index_);

		// Also check/uncheck children
		QMetaObject::invokeMethod(
			this, "emitDataChangedForChildren", Qt::QueuedConnection,
			Q_ARG(const QModelIndex&, index_));
	}
}

void
KFileSystemModel::onDirectoryLoaded(QString path)
{
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
	case DirectoryProcessingStatus::Skipped:
		sStatus = tr("Skipped");
		break;
	case DirectoryProcessingStatus::Error:
		sStatus = tr("Error");
		break;
	default:
		assert(!"Unexpected value");
	}

	return sStatus;
}

const KDirectoryData*
KFileSystemModel::lookupDirectoryData(const QString& path) const
{
	// Привести путь к общему виду
	const QString& fullPath = getUnifiedPathName(path);

	// Найти данные о директории
	auto iter = m_dirData.find(fullPath);

	// Найти данные о директории, если были обновлены ранее
	const KDirectoryData* dirData = nullptr;
	if (iter != m_dirData.end())
		dirData = &iter->second;

	return dirData;
}

KDirectoryData*
KFileSystemModel::lookupDirectoryData(const QString& path)
{
	// Привести путь к общему виду
	const QString& fullPath = getUnifiedPathName(path);

	// Найти данные о директории
	auto iter = m_dirData.find(fullPath);

	// Найти данные о директории, если были обновлены ранее
	KDirectoryData* dirData = nullptr;
	if (iter != m_dirData.end())
		dirData = &iter->second;

	return dirData;
}

void
KFileSystemModel::updateDirectoryData(const QString& unifiedPath, KDirectoryData&& dirData)
{
	assert(isUnifiedPath(unifiedPath));

	auto oldDirData = lookupDirectoryData(unifiedPath);

	// Обновить данные о директории
	if (nullptr != oldDirData)
		*oldDirData = std::move(dirData);
	else
		m_dirData.insert(std::make_pair(unifiedPath, std::move(dirData)));
}

void
KFileSystemModel::emitDataChanged(const QString& unifiedPath)
{
	assert(isUnifiedPath(unifiedPath));

	// Notify view
	auto dirIndex = index(unifiedPath);
	if (dirIndex.isValid())
	{
		emit dataChanged(dirIndex, dirIndex.siblingAtColumn(NumColumns - 1));
	}
}

void
KFileSystemModel::SetDirectoryInfo(const KDirectoryInfo& dirInfo)
{
	const auto& unifiedPath = dirInfo.fullPath;

	updateDirectoryData(unifiedPath, static_cast<KDirectoryData>(dirInfo));
	emitDataChanged(unifiedPath);
}
