#ifndef DIRECTORYSCANNER_H
#define DIRECTORYSCANNER_H

#include <set>
#include <mutex>
#include <condition_variable>
#include <stack>
#include <filesystem>

#include "IDirectoryScannerEventSink.h"
#include "model/WorkStack.h"

class DirectoryScanner
{
public:
	~DirectoryScanner();

	static DirectoryScanner* instance();

	void setRootPath(const QString& rootPath);

	// Call before exiting from the program for the sake of graceful work thread completion
	void fini();

	void subscribe(IDirectoryScannerEventSink* eventSink);
	void unsubscribe(IDirectoryScannerEventSink* eventSink);

	void setFocusedPath(const QString& dirPath);
	std::future<DirectoryProcessingStatus> setFocusedPathAndGetFuture(const QString& dirPath);
	void resetFocusedPathWithLocking();

private:
	DirectoryScanner();
	DirectoryScanner(const DirectoryScanner&) = delete;
	DirectoryScanner& operator=(const DirectoryScanner&) = delete;
	DirectoryScanner(DirectoryScanner&&) = delete;
	DirectoryScanner& operator=(DirectoryScanner&&) = delete;

	mutable std::mutex m_sync;
	QString m_rootPath;

	// A request to set new focused parent path
	struct PendingFocusedParentPath
	{
		QString path;
		WorkState::TPromisePtr pPromise;
	};

	mutable std::mutex m_syncPendingFocusedParentPath;
	mutable std::condition_variable m_cvFocusedParentPath; // For notification that value is picked up
	std::optional<PendingFocusedParentPath> m_pendingFocusedParentPath; // Waiting for being set

	void checkPendingFocusedParentPathAssignment();
	void setFocusedPathWithLocking(
		const QString& dirPath,
		WorkState::TPromisePtr pPromise);

	void popScanDirectoryAndSetPending();

	// Stack of directories being scanned. Child directories are on top of the stack.
	WorkStack m_workStack;

	// Data update event subscribers
	std::set<IDirectoryScannerEventSink*> m_eventSinks;

	void prepareDtoAndNotifyEventSinks(
		const QString& dirPath,
		const DirectoryDetails& dirDetails,
		bool acquireLock = true);
	void postDirInfo(KDirectoryInfoPtr pDirInfo);
	void postMimeSizesInfo(KMimeSizesInfoPtr pDirInfo);

	// Returns true in case of scuccessful completion.
	// Retuns false in case of cancellation or pause.
	bool scanDirectory(WorkState* workState);

	//
	// Work thread -related members
	//

	std::thread m_threadWorker;
	bool m_stopWorker = false;
	bool m_isCancellationRequested = false;
	bool m_isScanRunning = false;
	std::condition_variable m_scanningDone;

	void setScanRunning(bool running);

	void requestCancellationAndWait(std::unique_lock<std::mutex>& lock_);
	bool isCancellationRequested() const noexcept;

	bool isDestroying() const noexcept;
	void stopWorker() noexcept;

	void worker();

	//
	// Notification thread -related members
	//

	std::thread m_threadNotifier;

	// Directory update DTOs (for directory tree widget)
	typedef std::map<
		QString,	// Unified path
		KDirectoryInfoPtr
	> TDirInfoDTOs;
	TDirInfoDTOs m_dirInfos;

	// MIME type total size update DTOs (for MIME total sizes table)
	typedef std::map<
		QString,	// Unified path
		KMimeSizesInfoPtr
	> TMimeSizesInfoDTOs;
	TMimeSizesInfoDTOs m_mimeSizesInfos;

	void notifier();

	// Work threads' errors handling
	void handleWorkerException(std::exception_ptr&& pEx) noexcept;
};

#endif // DIRECTORYSCANNER_H
