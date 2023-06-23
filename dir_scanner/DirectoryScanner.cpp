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
	// Проверить не осталось ли потребителей сообщений
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
    // Подготовить DTO
    //

    auto pDirInfo = std::make_shared<KDirectoryInfo>();
    pDirInfo->fullPath = dirPath;
    pDirInfo->status = dirDetails.status;
    pDirInfo->subDirCount = dirDetails.subdirectoryCount;

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
        // Заменить на более новое
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
        // Заменить на более новое
        iter->second = pMimeSizesInfo;
}

void
DirectoryScanner::requestCancellationAndWait(std::unique_lock<std::mutex>& lock_)
{
    auto resetCancellationRequest = scope_guard([&](auto) {
        m_isCancellationRequested = false;
        });

    // Дождаться завершения выполнения scanDirectory перед манипуляциями со стеком задач
    m_isCancellationRequested = true;
    m_scanningDone.wait(lock_, [&] { return !m_isScanRunning; });
}

void
DirectoryScanner::setFocusedPath(const QString& dirPath)
{
    std::scoped_lock lock_(m_syncFocusedParentPath);
    m_pendingFocusedParentPath = dirPath;
}

void
DirectoryScanner::checkPendingFocusedParentPathAssignment()
{
    std::optional<QString> pendingFocusedParentPath;
    {
        std::scoped_lock lock_(m_syncFocusedParentPath);
        pendingFocusedParentPath = m_pendingFocusedParentPath;
        m_pendingFocusedParentPath.reset();
    }

    if (pendingFocusedParentPath.has_value())
        setFocusedPathWithLocking(pendingFocusedParentPath.value());
}

void
DirectoryScanner::setFocusedPathWithLocking(const QString& dirPath)
{
    QString unifiedPath = getUnifiedPathName(dirPath);

    std::unique_lock lock_(m_sync);

    m_workStack.setFocusedPath(unifiedPath);

    requestCancellationAndWait(lock_);

    // Отменить все задачи до общей родительской
    while (!m_workStack.empty())
    {
        const WorkState& workState = m_workStack.top();
        auto workDirPath = workState.fullPath;

        // Если совпадает с одной из задач из стека, больше ничего не делать
        if (workDirPath == unifiedPath)
            return;

        if (!isParentPath(unifiedPath, workDirPath))
        {
            m_workStack.popScanDirectory(DirectoryProcessingStatus::Pending);

            // Уведомить потребителей сообщений
            DirectoryDetails workDirDetails;
            bool res = DirectoryStore::instance()->tryGetDirectory(workDirPath, false, workDirDetails);
            assert(res);

            workDirDetails.status = DirectoryProcessingStatus::Pending;
            prepareDtoAndNotifyEventSinks(workDirPath, workDirDetails, false);
        }
        else
        {
            break;
        }
    }

    // Крайняя родительская задача
    WorkState topParentWorkState;
    if (!m_workStack.empty())
        topParentWorkState = m_workStack.top();

    //
    // Добавить задачи от крайней родительской до dirPath
    //

    std::vector<QString> tmp;
    QString path = unifiedPath;
    do
    {
        tmp.emplace_back(path);

        path = getImmediateParent(path);
        if (path.isEmpty() || path == m_rootPath)
            break;

        assert(m_rootPath.isEmpty() || !m_rootPath.startsWith(path));

    } while (path != topParentWorkState.fullPath);

    for (auto ii = tmp.crbegin(); ii != tmp.crend(); ++ii)
    {
        const QString& path = *ii;

        WorkState wState;
        wState.fullPath = path;

        m_workStack.pushScanDirectory(wState);
    }
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
}

void
DirectoryScanner::worker()
{
    try
    {
        while (!isDestroying())
        {
            // При выходе из scope
            auto scopedNotify = scope_guard([&](auto) {
                setScanRunning(false);
                m_scanningDone.notify_all();
                });

            // Выбрать задачу
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

                // Не сканировать выше пути в фокусе
                if (m_workStack.isAboveFocusedPath(workDirPath))
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
                    // После scanDirectory стек может изменитсья
                    auto unsetWorkState = scope_guard([&](auto) {
                        workState = 0;
                        });

                    prepareDtoAndNotifyEventSinks(workDirPath, workDirDetails);

                    bool scanned = scanDirectory(workState);

                    workDirDetails.subdirectoryCount = workState->subDirCount;
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

            // Проверить, не был ли изменен фокус
            checkPendingFocusedParentPathAssignment();

            std::scoped_lock lock_(m_sync);

            //
            // Выбрать текущие накопленные сообщения
            //

            TDirInfoDTOs dirInfos;
            dirInfos.swap(m_dirInfos);

            TMimeSizesInfoDTOs mimeSizesInfos;
            mimeSizesInfos.swap(m_mimeSizesInfos);

            // Уведомить всех потребителей сообщений
            for (auto sink : m_eventSinks)
            {
                assert(!!sink);

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

    // Уведомить всех потребителей сообщений
    for (auto sink : m_eventSinks)
    {
        assert(!!sink);

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
    assert(isUnifiedPath(workState->fullPath));

    if (isCancellationRequested())
        return false;

    unsigned long itemCount = 0;

    // Если сканирование не было начато, создать итератор
    if (!workState->pDirIterator)
    {
        workState->pDirIterator = std::make_shared<std::filesystem::directory_iterator>(
            workState->fullPath.toStdWString());
    }

    auto& dirIterator = *workState->pDirIterator;
    for (const auto& entry : dirIterator)
    {
        if (entry.is_symlink())
            continue;

        const auto& fullPath_ = entry.path().wstring();
        const QString& fullPath = getUnifiedPathName(QString::fromStdWString(fullPath_));

        if (entry.is_directory())
        {
            ++workState->subDirCount;

            // Новая задача
            WorkState wState;
            wState.fullPath = fullPath;

            // Еще валиден, но сбросить
            workState = 0;

            std::scoped_lock lock_(m_sync);
            m_workStack.pushScanDirectory(wState);

            // Сканирование этой директории будет возобновлено,
            //  когда завершится только что созданная задача

            // dirIterator будет продвинут вперед в случае успешного завершения дочерней задачи

            return false;
        }
        else if (entry.is_regular_file())
        {
            auto extension_ = entry.path().extension().wstring();
            if (L'.' == extension_[0])
                extension_ = extension_.substr(1);

            const QString& extension = QString::fromStdWString(extension_);

            auto fileSize = entry.file_size();

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
