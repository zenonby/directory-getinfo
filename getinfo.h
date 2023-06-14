#ifndef GETINFO_H
#define GETINFO_H

#include <thread>
#include <mutex>
#include <QMainWindow>
#include <QItemSelectionModel>

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
    GetInfo(QWidget *parent = nullptr);
    ~GetInfo();

    //
    // IDirectoryScannerEventSink interface
    //

    virtual void onUpdateDirectoryInfo(KDirectoryInfoPtr pInfo) override;
    virtual void onUpdateMimeSizes(KMimeSizesInfoPtr pInfo) override;

private:
    Ui::GetInfo *ui;

    // Унифицированный путь текущей выбранной директории
    QString m_unifiedSelectedPath;

    // Модели для view
    KFileSystemModel m_fsModel;
    KMimeSizesModel m_msModel;

    Q_INVOKABLE void updateDirectoryInfo(KDirectoryInfoPtr pInfo);
    Q_INVOKABLE void updateMimeSizes(KMimeSizesInfoPtr pInfo);

private slots:
    void treeDirectoriesSelectionChanged(
        const QItemSelection& selected, const QItemSelection& deselected);
};

#endif // GETINFO_H
