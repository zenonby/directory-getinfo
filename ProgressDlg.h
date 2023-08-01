#ifndef PROGRESSFLG_H
#define PROGRESSFLG_H

#include <functional>
#include <future>
#include <QProgressDialog>

class ProgressDlg : public QObject
{
    Q_OBJECT;

    std::function<void(void)> m_worker;
    std::function<void(std::future<void> futCompletion)> m_onComplete;
    std::function<void(void)> m_onCancel;

public:
    ProgressDlg(QWidget* parent,
        const QString& windowTitle,
        const QString& labelText,
        std::function<void(void)> worker,
        std::function<void(std::future<void> futCompletion)> onComplete,
        std::function<void(void)> onCancel);

private:
    QWidget* m_parent;
    const QString& m_windowTitle;
    const QString& m_labelText;

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
