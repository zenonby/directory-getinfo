#include "ProgressDlg.h"

ProgressDlg::ProgressDlg(QWidget* parent,
    const QString& windowTitle,
    const QString& labelText,
    std::function<void(void)> worker,
    std::function<void(std::future<void> futCompletion)> onComplete,
    std::function<void(void)> onCancel)
    : m_parent(parent),
      m_windowTitle(windowTitle),
      m_labelText(labelText),
      m_worker(worker),
      m_onComplete(onComplete),
      m_onCancel(onCancel)
{
    assert(!!m_worker);
    assert(!!m_onComplete);

    start();
}

void
ProgressDlg::setProgressPercentage(int progressPercentage)
{
    bool res = QMetaObject::invokeMethod(this, "setProgressPercentageImpl",
        Qt::QueuedConnection,
        Q_ARG(int, progressPercentage));
    assert(res);
}

void
ProgressDlg::setProgressPercentageImpl(int progressPercentage)
{
    assert(!!m_progressDlg);
    m_progressDlg->setValue(progressPercentage);
}

void
ProgressDlg::start()
{
    assert(!m_progressDlg);
    m_progressDlg = std::make_unique<QProgressDialog>(m_parent);

    m_progressDlg->setWindowModality(Qt::WindowModal);
    m_progressDlg->setLabelText(tr("Please wait..."));
    m_progressDlg->setWindowTitle(tr("Save results to database"));
    m_progressDlg->setRange(0, 100);

    // Disable [Cancel] button if onCancel handler is not specified
    if (!m_onCancel)
        m_progressDlg->setCancelButton(nullptr);

    // Disable native 'close window' button ([X])
    m_progressDlg->setWindowFlags(
        Qt::Window |
        Qt::WindowTitleHint |
        Qt::CustomizeWindowHint);

    m_progressDlg->show();

    // Create promise in order to manually handle exceptions
    //  since progress dialog must be closed in any case.
    m_progressPromise = std::make_unique<TProgressPromisePtr::element_type>();

    std::thread th(&ProgressDlg::workerWrapper, this);
    th.detach();
}

void
ProgressDlg::complete()
{
    assert(!!m_progressDlg);
    assert(!!m_progressPromise);

    m_progressDlg->close();
    m_progressDlg.reset();

    // Take ownership from the object being destroyed in the complete handler
    decltype(m_progressPromise) progressPromise = std::move(m_progressPromise);

    m_onComplete(progressPromise->get_future());
}

void
ProgressDlg::workerWrapper()
{
    assert(!!m_progressPromise);
    assert(!!m_worker);

    try
    {
        m_worker();

        m_progressPromise->set_value();
    }
    catch (...)
    {
        std::exception_ptr ex = std::current_exception();
        try
        {
            m_progressPromise->set_exception(ex);
        }
        catch (std::bad_exception const&)
        {
            assert(0);
        }
    }

    // Close progress on the UI thread
    bool res = QMetaObject::invokeMethod(this, "complete", Qt::QueuedConnection);
    assert(res);
}
