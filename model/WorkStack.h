#ifndef WORKSTACK_H
#define WORKSTACK_H

#include <stack>
#include <memory>
#include <filesystem>
#include <QString>

#include "model/DirectoryDetails.h"

// Scan state
struct WorkState
{
	// Unified path
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

	// unifiedPath - unified path
	void setFocusedPath(const QString& unifiedPath);

	bool isAboveFocusedPath(const QString& unifiedPath) const noexcept;

	// Pushes a work task on the work stack
	void pushScanDirectory(const WorkState& workState);

	// Removes a task from the work stack
	void popScanDirectory(DirectoryProcessingStatus status);

	// Pops already finished task from the stack in case of subsequent attempt to execute
	void popReadyScanDirectory();

	// Removes disabled scan directory from the stack
	void popDisabledScanDirectory();

	// Cumulatively transfers (adds) results of a finished task to a parrent task
	void copyReadyScanDirectoryDataToParent(const WorkState& workState);

	// pops a task which caused an error from the work stack
	void popErrorScanDirectory(const QString& workDirPath);

private:
	std::stack<WorkState> m_scanDirectories;

	// Focused (from UI) scan path
	QString m_focusedParentPath;
};

#endif // WORKSTACK_H
