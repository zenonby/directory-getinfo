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
//	rootPath = "";
#endif

	setRootPath(rootPath);

	setFilter(
		QDir::Dirs |
		QDir::Drives |
		QDir::NoSymLinks |
		QDir::NoDotAndDotDot);

	connect(this, SIGNAL(directoryLoaded(QString)), this, SLOT(onDirectoryLoaded(QString)));
}

void
KFileSystemModel::setFileSizeDivisor(FileSizeDivisor divisor)
{
	bool changed = m_divisor != divisor;
	m_divisor = divisor;

	if (changed)
	{
		emit dataChanged(index(0, 0), index(rowCount() - 1, NumColumns - 1));
		emit headerDataChanged(Qt::Horizontal, 0, NumColumns - 1);
	}
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

	if (role == Qt::TextAlignmentRole &&
		index.isValid() &&
		1 <= index.column() && index.column() <= 3)
	{
		return static_cast<int>(Qt::AlignVCenter | Qt::AlignRight);
	}

	if (role == Qt::CheckStateRole &&
		index.isValid() &&
		index.column() == 0)
	{
		const auto& path = getUnifiedPathName(filePath(index));
		bool enabled = DirectoryScanSwitch::instance()->isEnabled(path);
		return static_cast<int>(enabled ? Qt::Checked : Qt::Unchecked);
	}

	unsigned int divisorValue = FileSizeDivisorUtils::getDivisorValue(m_divisor);

	if (Qt::DisplayRole == role &&
		index.isValid() &&
		0 < columnIndex)
	{
		QVariant val;

		// Lookup directory data if they were updated before
		auto dirData = lookupDirectoryData(filePath(index));

		switch (columnIndex)
		{
		case 1:
			return nullptr != dirData && dirData->subdirectoryCount.has_value() ?
				QString("%L1").arg(static_cast<unsigned long long>(dirData->subdirectoryCount.value())) :
				QString();
		case 2:
			return nullptr != dirData && dirData->totalFileCount.has_value() ?
				QString("%L1").arg(static_cast<qulonglong>(dirData->totalFileCount.value())) :
				QString();
		case 3:
			return nullptr != dirData && dirData->totalSize.has_value() ?
				QString("%L1").arg(static_cast<qulonglong>(dirData->totalSize.value() / divisorValue)) :
				QString();
		case 4:
			return translateDirectoryProcessingStatus(
				nullptr != dirData ? dirData->status : DirectoryProcessingStatus::Pending);
		default:
			assert(!"Unexpected");
		}

		return val;
	}
	else
	{
		// Base implementation
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
		case 2:
			returnValue = tr("Total files");
			break;
		case 3:
			returnValue = tr("Total size, ") + FileSizeDivisorUtils::getDivisorSuffix(m_divisor);
			break;
		case 4:
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
		// Base implementation
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

	// Lookup directory data if they were updated before
	const auto& fullPath = getUnifiedPathName(filePath(index_));
	DirectoryScanSwitch::instance()->setEnabled(fullPath, checked);

	emit dataChanged(index_, index_);

	// Also check/uncheck children
	bool res = QMetaObject::invokeMethod(
		this, "emitDataChangedForChildren", Qt::QueuedConnection,
		Q_ARG(const QModelIndex&, index_));
	assert(res);
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
		bool res = QMetaObject::invokeMethod(
			this, "emitDataChangedForChildren", Qt::QueuedConnection,
			Q_ARG(const QModelIndex&, index_));
		assert(res);
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
	// Translate path to a unified one
	const QString& fullPath = getUnifiedPathName(path);

	// Lookup directory data
	auto iter = m_dirData.find(fullPath);

	// Lookup directory data if they were update before
	const KDirectoryData* dirData = nullptr;
	if (iter != m_dirData.end())
		dirData = &iter->second;

	return dirData;
}

KDirectoryData*
KFileSystemModel::lookupDirectoryData(const QString& path)
{
	// Translate path to a unified one
	const QString& fullPath = getUnifiedPathName(path);

	// Lookup directory data
	auto iter = m_dirData.find(fullPath);

	// Lookup directory data if they were update before
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

	// Update directory data
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
