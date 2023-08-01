#ifndef PROGRESSFLG_H
#define PROGRESSFLG_H

#include <functional>
#include <future>
#include <QProgressDialog>

class ProgressDlg : public QObject
{
    Q_OBJECT;

public:
    typedef std::function<void(void)> TCallback;

    ProgressDlg(QWidget* parent,
        const QString& windowTitle,
        const QString& labelText,
        TCallback worker,
        TCallback onComplete,
        TCallback onCancel);

private:
    QWidget* m_parent;
    const QString& m_windowTitle;
    const QString& m_labelText;

    // Callbacks
    TCallback m_worker;
    TCallback m_onComplete;
    TCallback m_onCancel;

    typedef std::unique_ptr<QProgressDialog> TProgressDialogPtr;
    TProgressDialogPtr m_progressDlg;

    typedef std::unique_ptr<std::promise<void>> TProgressPromisePtr;
    TProgressPromisePtr m_progressPromise;

    void start();
    void workerWrapper();
    void setProgressPercentage(int progressPercentage);

    Q_INVOKABLE void complete();
    Q_INVOKABLE void setProgressPercentageImpl(int progressPercentage);
};

#endif // PROGRESSFLG_H
