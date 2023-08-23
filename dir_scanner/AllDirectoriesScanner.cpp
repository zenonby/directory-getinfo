#include <QObject>
#include <thread>
#include "utils.h"
#include "DirectoryScanner.h"
#include "AllDirectoriesScanner.h"

AllDirectoriesScanner::AllDirectoriesScanner()
    : m_ignoreCallbackComplete(false)
{
}

AllDirectoriesScanner::~AllDirectoriesScanner()
{
}

AllDirectoriesScanner*
AllDirectoriesScanner::instance()
{
	static AllDirectoriesScanner s_instance;
	return &s_instance;
}

void
AllDirectoriesScanner::fini()
{
    waitForActiveFutureToFinish();
}

void AllDirectoriesScanner::waitForActiveFutureToFinish()
{
    decltype(m_activeFuture) fut;

    {
        std::scoped_lock lock_(m_sync);
        fut = m_activeFuture;
        m_activeFuture.reset();
    }

    if (fut.has_value())
        fut.value().get();
}

void
AllDirectoriesScanner::resetActiveFuture()
{
    std::scoped_lock lock_(m_sync);
    m_activeFuture.reset();
}

DirectoryProcessingStatus
AllDirectoriesScanner::setAndGetActiveFuture(std::future<DirectoryProcessingStatus>&& fut)
{
    std::shared_future<DirectoryProcessingStatus> futClone;

    // Assign to shared future
    {
        std::scoped_lock lock_(m_sync);
        m_activeFuture = futClone = std::move(fut);
    }

    return futClone.get();
}

void
AllDirectoriesScanner::scanDirectoriesSequentially(
    const std::vector<QString>& directories,
    std::function<void()> callbackComplete)
{
    // Check that there's no other sequential scan is running
    {
        std::scoped_lock lock_(m_sync);
        assert(!m_activeFuture.has_value());
    }

    waitForActiveFutureToFinish();

    std::thread th(
        std::bind(&AllDirectoriesScanner::scanDirectoriesSequentiallyWorker, this, directories, callbackComplete)
    );
    th.detach();
}

void
AllDirectoriesScanner::scanDirectoriesSequentiallyWorker(
    const std::vector<QString> directories,
    std::function<void()> callbackComplete)
{
    for (auto path : directories)
    {
        auto fut = DirectoryScanner::instance()->setFocusedPathAndGetFuture(path);

        try
        {
            auto resetActiveFuture_ = scope_guard([&](auto) {
                resetActiveFuture();
            });

            // Store this future in a member shared future and wait for it's result
            auto status = setAndGetActiveFuture(std::move(fut));

            // Pending status means that scanning was cancelled
            if (DirectoryProcessingStatus::Pending == status)
                break;
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
    }

    std::scoped_lock lock_(m_sync);
    if (!m_ignoreCallbackComplete)
    {
        // Call under a lock so that a possible caller of ignoreCallbackComplete()
        //  in some dtor would block until execution of the callback is finished
        callbackComplete();
    }
}

void
AllDirectoriesScanner::ignoreCallbackComplete()
{
    std::scoped_lock lock_(m_sync);
    m_ignoreCallbackComplete = true;
}
