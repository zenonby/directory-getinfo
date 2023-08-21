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
WorkStack::checkedPopScanDirectory()
{
    assert(!m_scanDirectories.top().pPromise && "Promise must be set_value() and reset() before popping a work item");

    m_scanDirectories.pop();
}

void
WorkStack::setFocusedPath(const QString& unifiedPath)
{
    assert(isUnifiedPath(unifiedPath));

    // The path itself is not required, just a parent path
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
    // Check task status and update if needed
    //

    DirectoryDetails dirDetails;
    bool res = DirectoryStore::instance()->tryGetDirectory(workDirPath, false, dirDetails);

    // Update values in the stack about possibly cancelled scan
    if (res)
    {
        auto& workState_ = m_scanDirectories.top();

        if (dirDetails.subdirectoryCount.has_value())
            workState_.subDirCount = dirDetails.subdirectoryCount.value();

        if (dirDetails.mimeDetailsList.has_value())
            workState_.mimeSizes = dirDetails.mimeDetailsList.value();
    }

    // Update status
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

    // Update task status
    DirectoryDetails dirDetails;
    dirDetails.status = status;

    if (DirectoryProcessingStatus::Ready == status)
    {
        dirDetails.subdirectoryCount = workState.subDirCount;
        dirDetails.mimeDetailsList = workState.mimeSizes;
    }

    DirectoryStore::instance()->upsertDirectory(workState.fullPath, dirDetails);

    auto pPromise = std::move(top().pPromise);
    checkedPopScanDirectory();

    // In case of successful completion add values (scan results)
    //  of this directory to the parent directory
    if (DirectoryProcessingStatus::Ready == status)
    {
        copyReadyScanDirectoryDataToParent(workState);
    }

    if (pPromise)
        pPromise->set_value(status);
}

void
WorkStack::popReadyScanDirectory()
{
    auto pPromise = std::move(top().pPromise);

    // Just remove without changing status
    checkedPopScanDirectory();

    // Also move forward parent's directory iterator
    if (!m_scanDirectories.empty())
    {
        auto& pDirIterator = m_scanDirectories.top().pDirIterator;
        if (pDirIterator)
            (*pDirIterator)++;
    }

    if (pPromise)
        pPromise->set_value(DirectoryProcessingStatus::Ready);
}

void
WorkStack::popDisabledScanDirectory()
{
    popScanDirectory(DirectoryProcessingStatus::Skipped);

    // Also move forward parent's directory iterator
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
    // Remove from the stack child directories that could be probably added
    while (workDirPath != m_scanDirectories.top().fullPath)
        popScanDirectory(DirectoryProcessingStatus::Error);

    // Remove the directory itself
    assert(!m_scanDirectories.empty() && workDirPath == m_scanDirectories.top().fullPath);
    popScanDirectory(DirectoryProcessingStatus::Error);
}

void
WorkStack::copyReadyScanDirectoryDataToParent(const WorkState& workState)
{
    const auto& workDirPath = workState.fullPath;

    // In the work stack
    if (!m_scanDirectories.empty())
    {
        auto& parentWorkState = m_scanDirectories.top();
        parentWorkState.mimeSizes.addMimeDetails(workState.mimeSizes);

        // Also move forward the iterator
        if (parentWorkState.pDirIterator)
            (*parentWorkState.pDirIterator)++;
    }

    // Update the data store
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

        if (!parentDirDetails.mimeDetailsList.has_value())
            parentDirDetails.mimeDetailsList = workState.mimeSizes;
        else
            parentDirDetails.mimeDetailsList.value().addMimeDetails(workState.mimeSizes);

        DirectoryStore::instance()->upsertDirectory(parentDirPath, parentDirDetails);
    }
}

void
WorkStack::pauseTopDirectory()
{
    assert(!empty());

    auto& workState = top();

    // Handle promise
    if (workState.pPromise)
    {
        // Cancel previous promise since focusing a child directory or
        //  a new promise is assigned if requested
        workState.pPromise->set_value(DirectoryProcessingStatus::Pending);
        workState.pPromise.reset();
    }

    // Also update status
    DirectoryDetails dirDetails;
    bool res = DirectoryStore::instance()->tryGetDirectory(workState.fullPath, true, dirDetails);
    if (res)
    {
        dirDetails.status = DirectoryProcessingStatus::Pending;
        DirectoryStore::instance()->upsertDirectory(workState.fullPath, dirDetails);
    }
    else
    {
        assert(!"Direcory no found in the directory store");
    }
}
