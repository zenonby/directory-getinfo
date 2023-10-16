#include <algorithm>
#include "model/DirectoryStore.h"
#include "HistoryProvider.h"
#include "utils.h"

HistoryProvider::HistoryProvider()
    : m_ignoreCallbackComplete(false)
{
}

HistoryProvider::~HistoryProvider()
{
}

HistoryProvider*
HistoryProvider::instance()
{
	static HistoryProvider s_instance;
	return &s_instance;
}

void
HistoryProvider::fini()
{
    waitForAllThreadsToComplete();
}

void
HistoryProvider::waitForAllThreadsToComplete()
{
    std::scoped_lock lock_(m_sync);

    // Join all threads
    for (auto& iter : m_threadPool)
    {
        auto& th = iter.second;
        assert(th.joinable());

        th.join();
    }

    // Purge the pool
    m_threadPool.clear();
}

void
HistoryProvider::addThreadToThePool(std::thread&& th)
{
    const auto& id = th.get_id();
    m_latestThreadId = id;
    m_threadPool.emplace(id, std::move(th));
}

void
HistoryProvider::removeThreadFromThePoolWithLock(const std::thread::id& threadId)
{
    // Attempting to remove current thread causes deadlock in join()
    assert(std::this_thread::get_id() != threadId);

    std::scoped_lock lock_(m_sync);

    if (m_latestThreadId.has_value() && m_latestThreadId.value() == threadId)
        m_latestThreadId.reset();

    auto iter = m_threadPool.find(threadId);
    if (iter != m_threadPool.end())
    {
        auto& th = iter->second;
        th.join();

        m_threadPool.erase(iter);
    }
    else
    {
        assert(!"invalid thread ID");
    }
}

void
HistoryProvider::getDirectoryHistoryAsync(
    const QString& dirPath,
    std::function<void(TDirectoryHistoryPtr)> callbackComplete)
{
    std::thread th(
        std::bind(&HistoryProvider::getDirectoryHistoryWorker, this, dirPath, callbackComplete)
    );

    addThreadToThePool(std::move(th));
}

void
HistoryProvider::getDirectoryHistoryWorker(
    const QString& unifiedPath,
    std::function<void(TDirectoryHistoryPtr)> callbackComplete)
{
    KDBG_CURRENT_THREAD_NAME(L"HistoryProvider::getDirectoryHistoryWorker");

    assert(isUnifiedPath(unifiedPath));

    auto history = std::make_shared<TDirectoryHistory>();

    try
    {
        const auto& storeHistory = DirectoryStore::instance()->getDirectoryStatsHistory(unifiedPath);
        for (const auto& iter : storeHistory)
        {
            auto utcTimestamp = iter.first;
            const auto& dirStats = iter.second;

            QDateTime dt = convertToQDateTime(utcTimestamp);

            assert(dirStats.totalSize.has_value());
            history->emplace(std::make_pair(dt, dirStats.totalSize.value()));
        }
    }
    catch (const std::exception& ex)
    {
        assert(!"Unexpected exception in a bg thread");
        qCritical(ex.what());
    }
    catch (...)
    {
        assert(false);
        qCritical("Unknown exception in a bg thread");
    }

    std::scoped_lock lock_(m_sync);

    const auto& threadId = std::this_thread::get_id();
    bool thisIsLatest = m_latestThreadId.has_value() && m_latestThreadId.value() == threadId;

    if (thisIsLatest && !m_ignoreCallbackComplete)
    {
        // We're handling so reset the latest thread ID
        m_latestThreadId.reset();

        // Call under a lock so that a possible caller of ignoreCallbackComplete()
        //  in some dtor would block until execution of the callback is finished
        callbackComplete(history);
    }

    // Remove this thread from the pool but on a separate thread to avoid deadlock
    //  while joining this thread
    std::thread([self = this] (auto threadId) {
        KDBG_CURRENT_THREAD_NAME(L"removeThreadFromThePoolWithLock");
        self->removeThreadFromThePoolWithLock(threadId);
    }, std::this_thread::get_id()).detach();
}

void
HistoryProvider::ignoreCallbackComplete()
{
    std::scoped_lock lock_(m_sync);
    m_ignoreCallbackComplete = true;
}
