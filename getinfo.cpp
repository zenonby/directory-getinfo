﻿#include <future>
#include <QDebug>
#include "getinfo.h"
#include "./ui_getinfo.h"
#include "dir_scanner/DirectoryScanner.h"
#include "utils.h"

GetInfo::GetInfo(QWidget *parent)
    : QMainWindow(parent),
      ui(new Ui::GetInfo)
{
    ui->setupUi(this);

    QString rootPath = m_fsModel.rootPath();
    DirectoryScanner::instance()->setRootPath(rootPath);

    ui->treeDirectories->setModel(&m_fsModel);
    ui->treeDirectories->setRootIndex(m_fsModel.index(rootPath));
    ui->treeDirectories->setColumnWidth(0, 250);

    connect(
        ui->treeDirectories->selectionModel(),
        SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)),
        this, SLOT(treeDirectoriesSelectionChanged(const QItemSelection&, const QItemSelection&)));

    ui->tableMimeSizes->setModel(&m_msModel);

    DirectoryScanner::instance()->subscribe(this);
}

GetInfo::~GetInfo()
{
    DirectoryScanner::instance()->unsubscribe(this);

    delete ui;
    ui = nullptr;
}

void
GetInfo::treeDirectoriesSelectionChanged(
    const QItemSelection& selected, const QItemSelection& deselected)
{
    // Сбросить размеры файлов по типам
    m_msModel.setMimeSizes(KMimeSizesInfo::KMimeSizesList());

    auto selectedindexes = selected.indexes();
    int selectedCount = selectedindexes.count();

    if (0 == selectedCount)
    {
        m_unifiedSelectedPath.clear();
    }
    else
    {
        auto selectedIndex = selectedindexes.first();
        m_unifiedSelectedPath = getUnifiedPathName(m_fsModel.filePath(selectedIndex));

        DirectoryScanner::instance()->setFocusedPath(m_unifiedSelectedPath);
    }
}


void
GetInfo::onUpdateDirectoryInfo(KDirectoryInfoPtr pInfo)
{
    QMetaObject::invokeMethod(
        this, "updateDirectoryInfo", Qt::QueuedConnection,
        Q_ARG(KDirectoryInfoPtr, pInfo));
}

void
GetInfo::onUpdateMimeSizes(KMimeSizesInfoPtr pInfo)
{
    QMetaObject::invokeMethod(
        this, "updateMimeSizes", Qt::QueuedConnection,
        Q_ARG(KMimeSizesInfoPtr, pInfo));
}

void
GetInfo::updateDirectoryInfo(KDirectoryInfoPtr pInfo)
{
    assert(!!ui);

    m_fsModel.SetDirectoryInfo(*pInfo);
}

void
GetInfo::updateMimeSizes(KMimeSizesInfoPtr pInfo)
{
    assert(!!ui);

    // Обновленная директория
    QString updatedPath = getUnifiedPathName(pInfo->fullPath);

    if (m_unifiedSelectedPath == updatedPath)
        m_msModel.setMimeSizes(std::move(pInfo->mimeSizes));
}
