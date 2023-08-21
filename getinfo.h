#ifndef GETINFO_H
#define GETINFO_H

#include <exception>
#include <thread>
#include <future>
#include <mutex>
#include <QMainWindow>
#include <QItemSelectionModel>

#include "ProgressDlg.h"
#include "view_model/kfilesystemmodel.h"
#include "view_model/kmimesizesmodel.h"
#include "dir_scanner/IDirectoryScannerEventSink.h"

QT_BEGIN_NAMESPACE
namespace Ui { class GetInfo; }
QT_END_NAMESPACE

class GetInfo : public QMainWindow, IDirectoryScannerEventSink
{
    Q_OBJECT

public:
    GetInfo(QWidget* parent = nullptr);
    ~GetInfo();

    //
    // IDirectoryScannerEventSink interface
    //

    virtual void onUpdateDirectoryInfo(KDirectoryInfoPtr pInfo) override;
    virtual void onUpdateMimeSizes(KMimeSizesInfoPtr pInfo) override;
    virtual void onWorkerException(std::exception_ptr&& pEx) override;

private:

    Ui::GetInfo *ui;

    // Unified path of currently selected path
    QString m_unifiedSelectedPath;

    // Models for views
    KFileSystemModel m_fsModel;
    KMimeSizesModel m_msModel;

    Q_INVOKABLE void updateDirectoryInfo(KDirectoryInfoPtr pInfo);
    Q_INVOKABLE void updateMimeSizes(KMimeSizesInfoPtr pInfo);
    Q_INVOKABLE void workerException(const std::exception_ptr& pEx);

    // Restores 'Scan All' button state
    Q_INVOKABLE void restoreScanAllButton();

    void readSettings();
    void writeSettings();

    std::unique_ptr<ProgressDlg> m_progressDlg;

    void saveSnapshot();
    void onCompleteSavingSnapshot();

protected:
    virtual void showEvent(QShowEvent* event) override;
    virtual void closeEvent(QCloseEvent* event) override;

private slots:
    void treeDirectoriesSelectionChanged(
        const QItemSelection& selected, const QItemSelection& deselected);

    void switchToBytes();
    void switchToKBytes();
    void switchToMBytes();

    // Saves scan results to DB
    void startSavingSnapshot();

    // Starts scanning all directories
    void scanAllDirectories();
};

#endif // GETINFO_H
