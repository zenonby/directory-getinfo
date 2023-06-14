#ifndef KDIRECTORYINFO_H
#define KDIRECTORYINFO_H

#include <memory>
#include <optional>
#include <QString>
#include "model/DirectoryProcessingStatus.h"

// Данные, ассоциированные с директорией без имени последней
struct KDirectoryData
{
	// Статус сканирования директории
	DirectoryProcessingStatus status = DirectoryProcessingStatus::Pending;

	// Кол-во непосредственных поддиректорий
	std::optional<unsigned long> subDirCount = 0;
};

// Информация об отдельном узле дерева
struct KDirectoryInfo : KDirectoryData
{
	// Полный путь директории
	QString fullPath;
};

typedef std::shared_ptr<KDirectoryInfo> KDirectoryInfoPtr;

Q_DECLARE_METATYPE(KDirectoryInfoPtr)

#endif // KDIRECTORYINFO_H
