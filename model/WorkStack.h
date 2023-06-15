#ifndef WORKSTACK_H
#define WORKSTACK_H

#include <stack>
#include <memory>
#include <filesystem>
#include <QString>

#include "model/DirectoryDetails.h"

// Состояние сканирования
struct WorkState
{
	// Унифицированный путь
	QString	fullPath;

	std::shared_ptr<std::filesystem::directory_iterator> pDirIterator;

	unsigned long subDirCount = 0;
	TMimeDetailsList mimeSizes;
};

class WorkStack
{
public:
	bool empty() const noexcept;

	const WorkState& top() const noexcept;
	WorkState& top() noexcept;

	// unifiedPath - унифицированный путь
	void setFocusedPath(const QString& unifiedPath);

	bool isAboveFocusedPath(const QString& unifiedPath) const noexcept;

	// Добавляет рабочее задание на стек
	void pushScanDirectory(const WorkState& workState);

	// Удаляет задание из стека
	void popScanDirectory(DirectoryProcessingStatus status);

	// Удалет уже завершенную задачу из стека при попытке повторного вызова
	void popReadyScanDirectory();

	// Переносит данные завершенной задачи в родительскую (суммрует)
	void copyReadyScanDirectoryDataToParent(const WorkState& workState);

	// Удалет задачу, вызвавшую исключение из стека
	void popErrorScanDirectory(const QString& workDirPath);

private:
	std::stack<WorkState> m_scanDirectories;

	// Установленный фокус скана
	QString m_focusedParentPath;
};

#endif // WORKSTACK_H
