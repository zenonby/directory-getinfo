#ifndef HISTORYPROVIDER_H
#define HISTORYPROVIDER_H

#include <map>
#include <unordered_map>
#include <mutex>
#include <optional>
#include <future>
#include <QDateTime>

/// Retrieves history info about directories from the HistoryProvider
class HistoryProvider
{
public:
	~HistoryProvider();

	static HistoryProvider* instance();

	// Call before exiting from the program for the sake of graceful work thread completion
	void fini();

	typedef std::map<
		QDateTime,				// timestamp
		unsigned long long		// total directory size
	> TDirectoryHistory;

	typedef std::shared_ptr<TDirectoryHistory> TDirectoryHistoryPtr;

	void getDirectoryHistoryAsync(
		const QString& unifiedPath,
		std::function<void (TDirectoryHistoryPtr)> callbackComplete);

	// Cancels execution of callbackComplete specified in scanDirectoriesSequentially()
	void ignoreCallbackComplete();

private:
	HistoryProvider();
	HistoryProvider(const HistoryProvider&) = delete;
	HistoryProvider& operator=(const HistoryProvider&) = delete;

	mutable std::mutex m_sync;
	bool m_ignoreCallbackComplete;

	/// All threads currently executing DirectoryStore requests
	typedef std::unordered_map<
		std::thread::id,
		std::thread
	> TThreadPool;
	TThreadPool m_threadPool;

	/// Latest started thread ID
	std::optional<std::thread::id> m_latestThreadId;

	void waitForAllThreadsToComplete();

	/// No locking
	void addThreadToThePool(std::thread&& th);

	/// threadId must not be this_thread::get_id()
	void removeThreadFromThePoolWithLock(const std::thread::id& threadId);

	void getDirectoryHistoryWorker(
		const QString& dirPath,
		std::function<void(TDirectoryHistoryPtr)> callbackComplete);
};

#endif // HISTORYPROVIDER_H
