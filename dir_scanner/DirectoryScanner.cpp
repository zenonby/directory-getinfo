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
#include "model/DirectoryStore.h"
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

        const auto& mimeDetailsList = dirDetails.mimeDetailsList.value();
        auto& mimeSizes = pMimeInfo->mimeSizes;
        mimeSizes.reserve(static_cast<int>(mimeDetailsList.size()));

        // Отобразить mimeDetailsList в элементы mimeSizes
        std::transform(mimeDetailsList.cbegin(), mimeDetailsList.cend(),
            std::back_inserter(mimeSizes), [](auto iter) {
                KMimeSize rv;
                auto& mimeDetails_ = iter.second;

                rv.mimeType = iter.first;
                rv.fileCount = mimeDetails_.fileCount;
                rv.totalSize = mimeDetails_.totalSize;
                rv.avgSize = mimeDetails_.fileCount ?
                    static_cast<float>(mimeDetails_.totalSize) / mimeDetails_.fileCount : 0;

                return rv;
            });

        // Перенести TMimeDetailsList::ALL_MIMETYPE в начало
        for (auto iter = mimeSizes.begin(); iter != mimeSizes.end(); ++iter)
        {
            const auto mimeSize = *iter;
            if (mimeSize.mimeType == TMimeDetailsList::ALL_MIMETYPE)
            {
                mimeSizes.erase(iter);
                mimeSizes.push_front(mimeSize);
                break;
            }
        }
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
DirectoryScanner::setFocusedPathWithLocking(const QString& dirPath)
{
    QString unifiedPath = getUnifiedPathName(dirPath);

    std::unique_lock lock_(m_sync);

    m_focusedParentPath = getImmediateParent(unifiedPath);

    requestCancellationAndWait(lock_);

    // Отменить все задачи до общей родительской
    while (!m_scanDirectories.empty())
    {
        const WorkState& workState = m_scanDirectories.top();
        auto workDirPath = workState.fullPath;

        // Если совпадает с одной из задач из стека, больше ничего не делать
        if (workDirPath == unifiedPath)
            return;

        if (!isParentPath(unifiedPath, workDirPath))
        {
            popScanDirectory(DirectoryProcessingStatus::Pending);

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
    if (!m_scanDirectories.empty())
        topParentWorkState = m_scanDirectories.top();

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

        pushScanDirectory(wState);
    }
}

void
DirectoryScanner::pushScanDirectory(const WorkState& workState)
{
    const auto& workDirPath = workState.fullPath;

    assert(m_scanDirectories.empty() || isParentPath(workDirPath, m_scanDirectories.top().fullPath));

    m_scanDirectories.push(workState);

    //
    // Проверить статус задачи и при необходимости обновить
    //

    DirectoryDetails dirDetails;
    bool res = DirectoryStore::instance()->tryGetDirectory(workDirPath, false, dirDetails);

    // Обновить значения в стеке о возможно прерванном сканировании
    if (res)
    {
        auto& workState_ = m_scanDirectories.top();

        if (dirDetails.subdirectoryCount.has_value())
            workState_.subDirCount = dirDetails.subdirectoryCount.value();

        if (dirDetails.mimeDetailsList.has_value())
            workState_.mimeSizes = dirDetails.mimeDetailsList.value();
    }

    if (!m_focusedParentPath.startsWith(workDirPath) &&
        (!res ||
         (DirectoryProcessingStatus::Ready != dirDetails.status &&
          DirectoryProcessingStatus::Error != dirDetails.status &&
          DirectoryProcessingStatus::Scanning != dirDetails.status)))
    {
        dirDetails.status = DirectoryProcessingStatus::Scanning;

        DirectoryStore::instance()->upsertDirectory(workState.fullPath, dirDetails);
    }
}

void
DirectoryScanner::popScanDirectory(DirectoryProcessingStatus status)
{
    auto workState = m_scanDirectories.top();
    const auto& workDirPath = workState.fullPath;

    // Обновить статус задачи
    DirectoryDetails dirDetails;
    dirDetails.status = status;

    if (DirectoryProcessingStatus::Ready == status)
    {
        dirDetails.subdirectoryCount = workState.subDirCount;
        dirDetails.mimeDetailsList = workState.mimeSizes;
    }

    DirectoryStore::instance()->upsertDirectory(workDirPath, dirDetails);

    m_scanDirectories.pop();

    // В случае успешного завершения добавить значения к родительской директории
    if (DirectoryProcessingStatus::Ready == status)
    {
        // В рабочем стеке
        if (!m_scanDirectories.empty())
        {
            auto& parentWorkState = m_scanDirectories.top();
            parentWorkState.mimeSizes.addMimeDetails(workState.mimeSizes);

            // Также продвинуть вперед итератор
            if (!!parentWorkState.pDirIterator)
                (*parentWorkState.pDirIterator)++;
        }

        // В базе
        const auto& parentDirPath = getImmediateParent(workDirPath);
        if (!parentDirPath.isEmpty())
        {
            DirectoryDetails parentDirDetails;
            bool res = DirectoryStore::instance()->tryGetDirectory(
                parentDirPath, false, parentDirDetails);
            if (!res)
            {
                parentDirDetails.status = DirectoryProcessingStatus::Pending;
            }

            assert(parentDirDetails.status != DirectoryProcessingStatus::Ready);

            if (!parentDirDetails.mimeDetailsList.has_value())
                parentDirDetails.mimeDetailsList = workState.mimeSizes;
            else
                parentDirDetails.mimeDetailsList.value().addMimeDetails(workState.mimeSizes);

            DirectoryStore::instance()->upsertDirectory(parentDirPath, parentDirDetails);
        }
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

            if (m_scanDirectories.empty())
            {
                lock_.unlock();
                std::this_thread::sleep_for(100ms);
                continue;
            }

            m_isScanRunning = true;
            workState = &m_scanDirectories.top();
            workDirPath = workState->fullPath;

            // Не сканировать выше пути в фокусе
            if (m_focusedParentPath == workDirPath)
            {
                lock_.unlock();
                std::this_thread::sleep_for(100ms);
                continue;
            }
        }

        // Проверить, не была ли уже просканирована
        DirectoryDetails workDirDetails;
        bool res = DirectoryStore::instance()->tryGetDirectory(workDirPath, true, workDirDetails);
        if (res &&
            (workDirDetails.status == DirectoryProcessingStatus::Ready ||
             workDirDetails.status == DirectoryProcessingStatus::Error))
        {
            std::scoped_lock lock_(m_sync);

            // Просто убрать задачу, не меняя ее статус
            m_scanDirectories.pop();

            // Также продвинуть вперед родительский итератор
            if (!m_scanDirectories.empty())
            {
                auto& pDirIterator = m_scanDirectories.top().pDirIterator;
                if (pDirIterator)
                    (*pDirIterator)++;
            }
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

                    assert(workDirPath == m_scanDirectories.top().fullPath);
                    popScanDirectory(!m_isCancellationRequested ?
                        DirectoryProcessingStatus::Ready :
                        DirectoryProcessingStatus::Pending);
                }
            }
            catch (const std::exception& x)
            {
                qCritical() << "ERROR: " << x.what() << endl;

//                assert(!"Excepton");

                workDirDetails.status = DirectoryProcessingStatus::Error;

                std::scoped_lock lock_(m_sync);

                // Убрать из стека потенциально добавленные туда дочерние директории
                while (workDirPath != m_scanDirectories.top().fullPath)
                    popScanDirectory(DirectoryProcessingStatus::Error);

                // Убрать саму директорию
                assert(!m_scanDirectories.empty() && workDirPath == m_scanDirectories.top().fullPath);
                popScanDirectory(DirectoryProcessingStatus::Error);
            }
        }

        prepareDtoAndNotifyEventSinks(workDirPath, workDirDetails);
    }
}

void
DirectoryScanner::notifier()
{
    while (!isDestroying())
    {
        std::this_thread::sleep_for(500ms);

        // Проверить, не был ли изменен фокус
        std::optional<QString> pendingFocusedParentPath;
        {
            std::scoped_lock lock_(m_syncFocusedParentPath);
            pendingFocusedParentPath = m_pendingFocusedParentPath;
            m_pendingFocusedParentPath.reset();
        }

        if (pendingFocusedParentPath.has_value())
            setFocusedPathWithLocking(pendingFocusedParentPath.value());

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
            pushScanDirectory(wState);

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
