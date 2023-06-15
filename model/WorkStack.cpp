#include "WorkStack.h"
#include "utils.h"
#include "DirectoryStore.h"

bool
WorkStack::empty() const noexcept
{
    return m_scanDirectories.empty();
}

const WorkState&
WorkStack::top() const noexcept
{
    return m_scanDirectories.top();
}

WorkState&
WorkStack::top() noexcept
{
    return m_scanDirectories.top();
}

void
WorkStack::setFocusedPath(const QString& unifiedPath)
{
    assert(isUnifiedPath(unifiedPath));

    // Сам путь не нужен, достаточно родительского пути
    m_focusedParentPath = getImmediateParent(unifiedPath);
}

bool
WorkStack::isAboveFocusedPath(const QString& unifiedPath) const noexcept
{
    bool res = m_focusedParentPath.startsWith(unifiedPath);
    return res;
}

void
WorkStack::pushScanDirectory(const WorkState& workState)
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

    // Обновить статус
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
WorkStack::popScanDirectory(DirectoryProcessingStatus status)
{
    auto workState = m_scanDirectories.top();

    // Обновить статус задачи
    DirectoryDetails dirDetails;
    dirDetails.status = status;

    if (DirectoryProcessingStatus::Ready == status)
    {
        dirDetails.subdirectoryCount = workState.subDirCount;
        dirDetails.mimeDetailsList = workState.mimeSizes;
    }

    DirectoryStore::instance()->upsertDirectory(workState.fullPath, dirDetails);

    m_scanDirectories.pop();

    // В случае успешного завершения добавить значения к родительской директории
    if (DirectoryProcessingStatus::Ready == status)
    {
        copyReadyScanDirectoryDataToParent(workState);
    }
}

void
WorkStack::popReadyScanDirectory()
{
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

void
WorkStack::popErrorScanDirectory(const QString& workDirPath)
{
    // Убрать из стека потенциально добавленные туда дочерние директории
    while (workDirPath != m_scanDirectories.top().fullPath)
        popScanDirectory(DirectoryProcessingStatus::Error);

    // Убрать саму директорию
    assert(!m_scanDirectories.empty() && workDirPath == m_scanDirectories.top().fullPath);
    popScanDirectory(DirectoryProcessingStatus::Error);
}

void
WorkStack::copyReadyScanDirectoryDataToParent(const WorkState& workState)
{
    const auto& workDirPath = workState.fullPath;

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
