#ifndef ALLDIRECTORIESSCANNER_H
#define ALLDIRECTORIESSCANNER_H

#include <functional>
#include <vector>
#include <optional>
#include <future>
#include <QString>
#include "model/DirectoryProcessingStatus.h"

// Singleton which manages lifetime of a worker thread which performs full scanning
//	and it's iteration with UI initiated scans
class AllDirectoriesScanner
{
public:
	~AllDirectoriesScanner();

	static AllDirectoriesScanner* instance();

	// Call before exit to make sure the worker thread is complete
	void fini();

	void waitForActiveFutureToFinish();

	// Scans specified directories sequentially, calls callbackComplete when all dirs are scanned
	void scanDirectoriesSequentially(
		const std::vector<QString>& directories,
		std::function<void()> callbackComplete);

	// Cancels execution of callbackComplete specified in scanDirectoriesSequentially()
	void ignoreCallbackComplete();

private:
	AllDirectoriesScanner();
	AllDirectoriesScanner(const AllDirectoriesScanner&) = delete;
	AllDirectoriesScanner& operator=(const AllDirectoriesScanner&) = delete;
	AllDirectoriesScanner(AllDirectoriesScanner&&) = delete;
	AllDirectoriesScanner& operator=(AllDirectoriesScanner&&) = delete;

	std::mutex m_sync;
	bool m_ignoreCallbackComplete;

	// Worker thread, which performs sequential directory scanning
	void scanDirectoriesSequentiallyWorker(
		const std::vector<QString> directories, // Do not pass a ref, rather make a copy
		std::function<void()> callbackComplete);

	// Currently pending future
	std::optional< std::shared_future<DirectoryProcessingStatus> > m_activeFuture;

	// Clears active future
	void resetActiveFuture();

	// Assigns active future and wait for its completion
	DirectoryProcessingStatus setAndGetActiveFuture(std::future<DirectoryProcessingStatus>&& fut);
};

#endif // ALLDIRECTORIESSCANNER_H
