#include <cassert>
#include <thread>
#include <future>
#include <chrono>
#include <algorithm>
#include <memory>
#include <QObject>
#include <QString>
#include <QDebug>

#include "DirectoryScanner.h"
#include "KDirectoryInfo.h"
#include "model/DirectoryScanSwitch.h"
#include "model/DirectoryStore.h"
#include "view_model/kmapper.h"
#include "utils.h"

using namespace std::chrono_literals;

DirectoryScanner*
DirectoryScanner::instance()
{
	static DirectoryScanner s_instance;
	return &s_instance;
}

DirectoryScanner::DirectoryScanner()
: m_threadWorker(&DirectoryScanner::worker, this),
  m_threadNotifier(&DirectoryScanner::notifier, this)
{
}

DirectoryScanner::~DirectoryScanner()
{
    assert(!m_threadWorker.joinable());
    assert(!m_threadNotifier.joinable());
}

void
DirectoryScanner::setRootPath(const QString& rootPath)
{
    std::scoped_lock lock_(m_sync);
    m_rootPath = getUnifiedPathName(rootPath);
}

void
DirectoryScanner::fini()
{
    // Check if any event subscribers/sinks are left
	{
		std::scoped_lock lock_(m_sync);

		assert(m_eventSinks.empty());
		m_eventSinks.clear();
	}

    stopWorker();
}

void
DirectoryScanner::subscribe(IDirectoryScannerEventSink* eventSink)
{
	std::scoped_lock lock_(m_sync);

	assert(m_eventSinks.find(eventSink) == m_eventSinks.end());
	m_eventSinks.emplace(eventSink);
}

void
DirectoryScanner::unsubscribe(IDirectoryScannerEventSink* eventSink)
{
	std::scoped_lock lock_(m_sync);

	auto iter = m_eventSinks.find(eventSink);
	assert(iter != m_eventSinks.end());

	if (iter != m_eventSinks.end())
		m_eventSinks.erase(iter);
}

void
DirectoryScanner::prepareDtoAndNotifyEventSinks(
    const QString& dirPath,
    const DirectoryDetails& dirDetails,
    bool acquireLock)
{
    //
    // Prepare DTO
    //

    auto pDirInfo = std::make_shared<KDirectoryInfo>();
    pDirInfo->fullPath = dirPath;
    pDirInfo->assignStatsWithStatus(dirDetails);

    bool sendMimeSizes = false;
    KMimeSizesInfoPtr pMimeInfo;
    if (dirDetails.mimeDetailsList.has_value())
    {
        sendMimeSizes = true;

        pMimeInfo = std::make_shared<KMimeSizesInfo>();
        pMimeInfo->fullPath = dirPath;

        KMapper::mapTMimeDetailsListToKMimeSizesList(
            dirDetails.mimeDetailsList.value(), pMimeInfo->mimeSizes);
    }

    {
        std::unique_lock lock_(m_sync, std::defer_lock);
        if (acquireLock)
            lock_.lock();

        postDirInfo(pDirInfo);

        if (sendMimeSizes)
            postMimeSizesInfo(pMimeInfo);
    }
}

void
DirectoryScanner::postDirInfo(KDirectoryInfoPtr pDirInfo)
{
    const auto& dirPath = pDirInfo->fullPath;

    auto iter = m_dirInfos.find(dirPath);
    if (iter == m_dirInfos.end())
        m_dirInfos.emplace(std::make_pair(dirPath, pDirInfo));
    else
        // Repace with newer one
        iter->second = pDirInfo;
}

void
DirectoryScanner::postMimeSizesInfo(KMimeSizesInfoPtr pMimeSizesInfo)
{
    const auto& dirPath = pMimeSizesInfo->fullPath;

    auto iter = m_mimeSizesInfos.find(dirPath);
    if (iter == m_mimeSizesInfos.end())
        m_mimeSizesInfos.emplace(std::make_pair(dirPath, pMimeSizesInfo));
    else
        // Repace with newer one
        iter->second = pMimeSizesInfo;
}

void
DirectoryScanner::requestCancellationAndWait(std::unique_lock<std::mutex>& lock_)
{
    auto resetCancellationRequest = scope_guard([&](auto) {
        m_isCancellationRequested = false;
    });

    // Wait for scanDirectory() to finish before changing the task stack
    m_isCancellationRequested = true;
    m_scanningDone.wait(lock_, [&] { return !m_isScanRunning; });
}

void
DirectoryScanner::setFocusedPath(const QString& dirPath)
{
    std::unique_lock lock_(m_syncPendingFocusedParentPath);

    m_pendingFocusedParentPath = PendingFocusedParentPath{ dirPath };
}

std::future<DirectoryProcessingStatus>
DirectoryScanner::setFocusedPathAndGetFuture(const QString& dirPath)
{
    std::unique_lock lock_(m_syncPendingFocusedParentPath);

    auto pPromise = std::make_shared<WorkState::TPromise>();
    auto fut = pPromise->get_future();

    m_pendingFocusedParentPath = PendingFocusedParentPath{ dirPath, pPromise };

    // Wait until pending value is picked up by a worker thread
    m_cvFocusedParentPath.wait(lock_);
    lock_.unlock();

    return fut;
}

void
DirectoryScanner::checkPendingFocusedParentPathAssignment()
{
    std::optional<PendingFocusedParentPath> pendingFocusedParentPath;
    {
        std::scoped_lock lock_(m_syncPendingFocusedParentPath);

        if (m_pendingFocusedParentPath.has_value())
        {
            pendingFocusedParentPath = std::move(m_pendingFocusedParentPath.value());
            m_pendingFocusedParentPath.reset();
        }
    }

    if (pendingFocusedParentPath.has_value())
    {
        setFocusedPathWithLocking(
            pendingFocusedParentPath.value().path,
            pendingFocusedParentPath.value().pPromise);

        // Notify UI thread
        m_cvFocusedParentPath.notify_all();
    }
}

void
DirectoryScanner::resetFocusedPathWithLocking()
{
    std::unique_lock lock_(m_sync);

    m_workStack.setFocusedPath();

    requestCancellationAndWait(lock_);

    while (!m_workStack.empty())
        popScanDirectoryAndSetPending();
}

void
DirectoryScanner::setFocusedPathWithLocking(
    const QString& dirPath,
    WorkState::TPromisePtr pPromise)
{
    QString unifiedPath = getUnifiedPathName(dirPath);

    std::unique_lock lock_(m_sync);

    m_workStack.setFocusedPath(unifiedPath);

    requestCancellationAndWait(lock_);

    // Cancel all tasks up to the shared parent (between current and focused)
    while (!m_workStack.empty())
    {
        const WorkState& workState = m_workStack.top();
        auto workDirPath = workState.fullPath;

        // If matches any of of tasks from the stack do nothing
        if (workDirPath == unifiedPath)
            break;

        if (!isParentPath(unifiedPath, workDirPath))
        {
            popScanDirectoryAndSetPending();
        }
        else
        {
            break;
        }
    }

    // Top parent task
    WorkState topParentWorkState;
    if (!m_workStack.empty())
    {
        topParentWorkState = m_workStack.top();
        m_workStack.pauseTopDirectory();
    }

    //
    // Add task starting from the parent to dirPath
    // (one per directory).
    //

    // Unroll unifiedPath in reverse order
    std::vector<QString> tmp;
    QString path = unifiedPath;
    while (path != topParentWorkState.fullPath)
    {
        tmp.emplace_back(path);

        path = getImmediateParent(path);
        if (path.isEmpty() || path == m_rootPath)
            break;

        assert(m_rootPath.isEmpty() || !m_rootPath.startsWith(path));
    }

    // Add directories while restoring order from top down to child
    for (auto ii = tmp.crbegin(); ii != tmp.crend(); ++ii)
    {
        const QString& path = *ii;

        WorkState wState;
        wState.fullPath = path;

        m_workStack.pushScanDirectory(wState);
    }

    // Assign promise
    if (pPromise)
    {
        assert(!m_workStack.empty());
        m_workStack.top().pPromise = pPromise;
    }
}

void
DirectoryScanner::popScanDirectoryAndSetPending()
{
    const WorkState& workState = m_workStack.top();
    auto workDirPath = workState.fullPath;

    m_workStack.popScanDirectory(DirectoryProcessingStatus::Pending);

    // Notify event subscribers
    DirectoryDetails workDirDetails;
    bool res = DirectoryStore::instance()->tryGetDirectory(workDirPath, false, workDirDetails);
    assert(res);

    workDirDetails.status = DirectoryProcessingStatus::Pending;
    prepareDtoAndNotifyEventSinks(workDirPath, workDirDetails, false);
}

bool
DirectoryScanner::isDestroying() const noexcept
{
    std::scoped_lock lock_(m_sync);
    return m_stopWorker;
}

bool
DirectoryScanner::isCancellationRequested() const noexcept
{
    std::scoped_lock lock_(m_sync);
    return m_isCancellationRequested;
}

void
DirectoryScanner::stopWorker() noexcept
{
    {
        std::scoped_lock lock_(m_sync);
        m_stopWorker = true;
        m_isCancellationRequested = true;
    }

    m_threadWorker.join();
    m_threadNotifier.join();

    // Pop remaining work items from the stack so that possible promises would be handled
    {
        std::scoped_lock lock_(m_sync);
        while (!m_workStack.empty())
        {
            m_workStack.popScanDirectory(DirectoryProcessingStatus::Pending);
        }
    }
}

void
DirectoryScanner::worker()
{
    try
    {
        while (!isDestroying())
        {
            // On scope exit
            auto scopedNotify = scope_guard([&](auto) {
                setScanRunning(false);
                m_scanningDone.notify_all();
            });

            // Select task
            WorkState* workState = nullptr;
            QString workDirPath;
            {
                std::unique_lock lock_(m_sync);

                if (m_workStack.empty())
                {
                    lock_.unlock();
                    std::this_thread::sleep_for(100ms);
                    continue;
                }

                m_isScanRunning = true;
                workState = &m_workStack.top();
                workDirPath = workState->fullPath;

                // Do not scan paths in focus
                if (m_workStack.isAboveOrEqualFocusedPath(workDirPath))
                {
                    lock_.unlock();
                    std::this_thread::sleep_for(100ms);
                    continue;
                }
            }

            DirectoryDetails workDirDetails;
            bool res = false;
            bool enableScan = false;

            // Check if scanned before
            if ((res = DirectoryStore::instance()->tryGetDirectory(workDirPath, true, workDirDetails)) &&
                (workDirDetails.status == DirectoryProcessingStatus::Ready ||
                 workDirDetails.status == DirectoryProcessingStatus::Error))
            {
                std::scoped_lock lock_(m_sync);
                m_workStack.popReadyScanDirectory();
            }
            // Check if skipped (only after checking if scanned before in order to avoid multiple scans)
            else if (!(enableScan = DirectoryScanSwitch::instance()->isEnabled(workDirPath)))
            {
                std::scoped_lock lock_(m_sync);

                workDirDetails.status = DirectoryProcessingStatus::Skipped;
                m_workStack.popDisabledScanDirectory();
            }
            else
            {
                try
                {
                    // After scanDirectory the stack might change
                    auto unsetWorkState = scope_guard([&](auto) {   
                        workState = nullptr;
                    });

                    prepareDtoAndNotifyEventSinks(workDirPath, workDirDetails);

                    bool scanned = scanDirectory(workState);

                    workDirDetails.DirectoryStats::assignStats(*workState);
                    workDirDetails.mimeDetailsList = workState->mimeSizes;

                    if (scanned)
                    {
                        workDirDetails.status = DirectoryProcessingStatus::Ready;

                        std::scoped_lock lock_(m_sync);

                        assert(workDirPath == m_workStack.top().fullPath);
                        m_workStack.popScanDirectory(!m_isCancellationRequested ?
                            DirectoryProcessingStatus::Ready :
                            DirectoryProcessingStatus::Pending);
                    }
                }
                catch (const std::exception& x)
                {
                    qCritical() << "ERROR: " << x.what() << endl;

                    workDirDetails.status = DirectoryProcessingStatus::Error;

                    std::scoped_lock lock_(m_sync);

                    m_workStack.popErrorScanDirectory(workDirPath);
                }
            }

            prepareDtoAndNotifyEventSinks(workDirPath, workDirDetails);
        }
    }
    catch (...)
    {
        handleWorkerException(std::current_exception());
    }
}

void
DirectoryScanner::notifier()
{
    try
    {
        while (!isDestroying())
        {
            std::this_thread::sleep_for(500ms);

            // Check if focused path has not changed
            checkPendingFocusedParentPathAssignment();

            std::scoped_lock lock_(m_sync);

            //
            // Extract currently collected events
            //

            TDirInfoDTOs dirInfos;
            dirInfos.swap(m_dirInfos);

            TMimeSizesInfoDTOs mimeSizesInfos;
            mimeSizesInfos.swap(m_mimeSizesInfos);

            // Update all hungry event subscribers
            for (auto sink : m_eventSinks)
            {
                assert(sink);

                for (const auto& pairDirInfo : dirInfos)
                    sink->onUpdateDirectoryInfo(pairDirInfo.second);

                for (const auto& pairMimeSizesInfo : mimeSizesInfos)
                    sink->onUpdateMimeSizes(pairMimeSizesInfo.second);
            }
        }
    }
    catch (...)
    {
        handleWorkerException(std::current_exception());
    }
}

void
DirectoryScanner::handleWorkerException(std::exception_ptr&& pEx) noexcept
{
    std::scoped_lock lock_(m_sync);

    // Update all event subscribers
    for (auto sink : m_eventSinks)
    {
        assert(sink);

        try
        {
            sink->onWorkerException(std::move(pEx));
        }
        catch (...)
        {
            assert(0);
        }
    }
}

void
DirectoryScanner::setScanRunning(bool running)
{
    std::scoped_lock lock_(m_sync);
    m_isScanRunning = running;
}

bool
DirectoryScanner::scanDirectory(WorkState* workState)
{
    assert(workState && isUnifiedPath(workState->fullPath));

    if (isCancellationRequested())
        return false;

    unsigned long itemCount = 0;

    // If scanning is not started create a directory/file iterator
    if (!workState->pDirIterator)
    {
        workState->pDirIterator = std::make_shared<std::filesystem::directory_iterator>(
            workState->fullPath.toStdWString());
    }

    if (!workState->subdirectoryCount.has_value())
        workState->subdirectoryCount = 0;

    if (!workState->totalFileCount.has_value())
        workState->totalFileCount = 0;

    if (!workState->totalSize.has_value())
        workState->totalSize = 0;

    auto& dirIterator = *workState->pDirIterator;
    for (const auto& entry : dirIterator)
    {
        if (entry.is_symlink())
            continue;

        const auto& fullPath_ = entry.path().wstring();
        const QString& fullPath = getUnifiedPathName(QString::fromStdWString(fullPath_));

        if (entry.is_directory())
        {
            workState->subdirectoryCount = workState->subdirectoryCount.value() + 1;

            // New task
            WorkState wState;
            wState.fullPath = fullPath;

            // It's still valid but reset anyway
            workState = 0;

            std::scoped_lock lock_(m_sync);
            m_workStack.pushScanDirectory(wState);

            // Scanning of this directory will be resumed
            // when the just created task is complete.

            // dirIterator is moved forward in case of successful child task completion

            return false;
        }
        else if (entry.is_regular_file())
        {
            auto extension_ = entry.path().extension().wstring();
            if (L'.' == extension_[0])
                extension_ = extension_.substr(1);

            const QString& extension = QString::fromStdWString(extension_);

            auto fileSize = entry.file_size();

            workState->totalSize = workState->totalSize.value() + fileSize;
            workState->totalFileCount = workState->totalFileCount.value() + 1;

            workState->mimeSizes.addMimeDetails(TMimeDetailsList::ALL_MIMETYPE, fileSize, 1);
            workState->mimeSizes.addMimeDetails(extension, fileSize, 1);
        }

        if (0 == ++itemCount % 100 && isCancellationRequested())
        {
            return false;
        }
    }

    return true;
}
