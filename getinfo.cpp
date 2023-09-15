#include <functional>
#include <QDebug>
#include <QMessageBox>

#include "getinfo.h"
#include "./ui_getinfo.h"
#include "dir_scanner/DirectoryScanner.h"
#include "dir_scanner/DirectoriesScanOrchestrator.h"
#include "model/DirectoryStore.h"
#include "utils.h"
#include "settings.h"

#define WINDOW_PREFIX "window"
#define DIVISOR_NAME WINDOW_PREFIX "/divisor"
#define DIVISOR_VALUE_B "B"
#define DIVISOR_VALUE_KB "KB"
#define DIVISOR_VALUE_MB "MB"

using namespace std::placeholders;

GetInfo::GetInfo(QWidget* parent)
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

    ui->actionSwitchToBytes->setChecked(true);
    connect(ui->actionSwitchToBytes, SIGNAL(triggered()), this, SLOT(switchToBytes()));
    connect(ui->actionSwitchToKBytes, SIGNAL(triggered()), this, SLOT(switchToKBytes()));
    connect(ui->actionSwitchToMBytes, SIGNAL(triggered()), this, SLOT(switchToMBytes()));

    connect(ui->actionSaveSnapshot, SIGNAL(triggered()), this, SLOT(startSavingSnapshot()));
    connect(ui->actionScanAll, SIGNAL(triggered()), this, SLOT(scanAllDirectories()));

    DirectoryScanner::instance()->subscribe(this);
}

GetInfo::~GetInfo()
{
    DirectoryScanner::instance()->unsubscribe(this);

    // Before destroying ui
    // so that a possibly executing callbackComplete lambda in scanAllDirectories()
    // which requires ui would finish before ui is destroyed.
    DirectoriesScanOrchestrator::instance()->ignoreCallbackComplete();

    delete ui;
    ui = nullptr;
}

void
GetInfo::treeDirectoriesSelectionChanged(
    const QItemSelection& selected, const QItemSelection& deselected)
{
    // Reset MIME type total sizes
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

        std::vector<QString> dirs;
        dirs.push_back(m_unifiedSelectedPath);
        DirectoriesScanOrchestrator::instance()->scanDirectoriesSequentially(dirs, [self = this]()
        {
#if 0
                bool res = QMetaObject::invokeMethod(
                    self, "", Qt::QueuedConnection);
                assert(res);
#endif
        });
    }
}

void
GetInfo::switchToBytes()
{
    if (!ui->actionSwitchToBytes->isChecked())
        ui->actionSwitchToBytes->setChecked(true);

    ui->actionSwitchToKBytes->setChecked(false);
    ui->actionSwitchToMBytes->setChecked(false);

    m_fsModel.setFileSizeDivisor(FileSizeDivisor::Bytes);
    m_msModel.setFileSizeDivisor(FileSizeDivisor::Bytes);
}

void
GetInfo::switchToKBytes()
{
    if (!ui->actionSwitchToKBytes->isChecked())
        ui->actionSwitchToKBytes->setChecked(true);

    ui->actionSwitchToBytes->setChecked(false);
    ui->actionSwitchToMBytes->setChecked(false);

    m_fsModel.setFileSizeDivisor(FileSizeDivisor::KBytes);
    m_msModel.setFileSizeDivisor(FileSizeDivisor::KBytes);
}

void
GetInfo::switchToMBytes()
{
    if (!ui->actionSwitchToMBytes->isChecked())
        ui->actionSwitchToMBytes->setChecked(true);

    ui->actionSwitchToBytes->setChecked(false);
    ui->actionSwitchToKBytes->setChecked(false);

    m_fsModel.setFileSizeDivisor(FileSizeDivisor::MBytes);
    m_msModel.setFileSizeDivisor(FileSizeDivisor::MBytes);
}

void
GetInfo::onUpdateDirectoryInfo(KDirectoryInfoPtr pInfo)
{
    bool res = QMetaObject::invokeMethod(
        this, "updateDirectoryInfo", Qt::QueuedConnection,
        Q_ARG(KDirectoryInfoPtr, pInfo));
    assert(res);
}

void
GetInfo::onUpdateMimeSizes(KMimeSizesInfoPtr pInfo)
{
    bool res = QMetaObject::invokeMethod(
        this, "updateMimeSizes", Qt::QueuedConnection,
        Q_ARG(KMimeSizesInfoPtr, pInfo));
    assert(res);
}

void
GetInfo::onWorkerException(std::exception_ptr&& pEx)
{
    bool res = QMetaObject::invokeMethod(
        this, "workerException", Qt::QueuedConnection,
        Q_ARG(std::exception_ptr, pEx));
    assert(res);
}

void
GetInfo::updateDirectoryInfo(KDirectoryInfoPtr pInfo)
{
    assert(ui);

    m_fsModel.SetDirectoryInfo(*pInfo);
}

void
GetInfo::updateMimeSizes(KMimeSizesInfoPtr pInfo)
{
    assert(ui);

    // Updated directory
    QString updatedPath = getUnifiedPathName(pInfo->fullPath);

    if (m_unifiedSelectedPath == updatedPath)
        m_msModel.setMimeSizes(std::move(pInfo->mimeSizes));
}

void
GetInfo::workerException(const std::exception_ptr& pEx)
{
    assert(ui);

    try
    {
        std::rethrow_exception(pEx);
    }
    catch (const std::exception& ex)
    {
        QMessageBox::critical(this, tr("Error"), ex.what());
        close();
    }
}

void
GetInfo::showEvent(QShowEvent* event)
{
    QMainWindow::showEvent(event);

    readSettings();
}

void
GetInfo::closeEvent(QCloseEvent* event)
{
    QMainWindow::closeEvent(event);

    if (event->isAccepted())
    {
        writeSettings();
    }
}

void
GetInfo::readSettings()
{
    Settings* settings = Settings::instance();

    QString sDivisor = settings->value(DIVISOR_NAME).toString();
    if (!sDivisor.isEmpty())
    {
        if (sDivisor == DIVISOR_VALUE_B)
            switchToBytes();
        else if (sDivisor == DIVISOR_VALUE_KB)
            switchToKBytes();
        else if (sDivisor == DIVISOR_VALUE_MB)
            switchToMBytes();
        else
        {
            assert(!"Unknown value");
            switchToBytes();
        }
    }
}

void
GetInfo::writeSettings()
{
    Settings* settings = Settings::instance();

    if (ui->actionSwitchToBytes->isChecked())
        settings->setValue(DIVISOR_NAME, DIVISOR_VALUE_B);
    else if (ui->actionSwitchToKBytes->isChecked())
        settings->setValue(DIVISOR_NAME, DIVISOR_VALUE_KB);
    else if (ui->actionSwitchToMBytes->isChecked())
        settings->setValue(DIVISOR_NAME, DIVISOR_VALUE_MB);
    else
        assert(!"Unexpected divisor selection");
}

void
GetInfo::startSavingSnapshot()
{
    assert(!m_progressDlg);
    m_progressDlg = std::make_unique<ProgressDlg>(this,
        tr("Save results to database"),
        tr("Please wait..."),
        std::bind(&GetInfo::saveSnapshot, this),
        std::bind(&GetInfo::onCompleteSavingSnapshot, this));
}

void
GetInfo::onCompleteSavingSnapshot()
{
    m_progressDlg.reset();
}

void
GetInfo::saveSnapshot()
{
    DirectoryStore::instance()->saveCurrentData();
}

void
GetInfo::scanAllDirectories()
{
    ui->actionScanAll->setChecked(true);
    ui->actionScanAll->setEnabled(false);

    ui->actionSaveSnapshot->setEnabled(false);

    // Get root directories
    std::vector<QString> topDirectories;
    auto rootIndex = ui->treeDirectories->rootIndex();
    const int count = m_fsModel.rowCount(rootIndex);
    for (int i = 0; i < count; ++i)
    {
        auto childIndex = m_fsModel.index(i, 0, rootIndex);
        auto childPath = m_fsModel.filePath(childIndex);
        topDirectories.emplace_back(childPath);
    }

    DirectoriesScanOrchestrator::instance()->scanDirectoriesSequentially(
        topDirectories, [self = this]()
        {
            bool res = QMetaObject::invokeMethod(
                self, "restoreScanAllButton", Qt::QueuedConnection);
            assert(res);
        });
}

void
GetInfo::restoreScanAllButton()
{
    ui->actionScanAll->setChecked(false);
    ui->actionScanAll->setEnabled(true);

    ui->actionSaveSnapshot->setEnabled(true);
}
