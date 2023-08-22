#ifndef WORKSTACK_H
#define WORKSTACK_H

#include <stack>
#include <memory>
#include <future>
#include <filesystem>
#include <QString>

#include "model/DirectoryDetails.h"

// Scan state
struct WorkState : DirectoryStats
{
	// Unified path
	QString	fullPath;

	typedef std::shared_ptr<std::filesystem::directory_iterator> TDirIteratorPtr;
	TDirIteratorPtr pDirIterator;

	TMimeDetailsList mimeSizes;

	// Promise is optional and can be used by UI thread to wait for completion.
	typedef std::promise<DirectoryProcessingStatus> TPromise;
	typedef std::shared_ptr<TPromise> TPromisePtr;
	TPromisePtr pPromise;
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

	// Sets top directory status to paused due to switching to one of it's child
	void pauseTopDirectory();

private:
	std::stack<WorkState> m_scanDirectories;

	// Focused (from UI) scan path
	QString m_focusedParentPath;

	// Performs checks before popping an item
	void checkedPopScanDirectory();
};

#endif // WORKSTACK_H
