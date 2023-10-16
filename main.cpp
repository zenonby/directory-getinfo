#include "getinfo.h"
#include "dir_scanner/DirectoryScanner.h"
#include "dir_scanner/DirectoriesScanOrchestrator.h"
#include "model/DirectoryScanSwitch.h"
#include "model/HistoryProvider.h"
#include "settings.h"
#include "utils.h"

#include <exception>
#include <QDebug>
#include <QApplication>

int
main(int argc, char *argv[])
{
    KDBG_CURRENT_THREAD_NAME(L"main");

    int res = 1;

    try
    {
        QCoreApplication::setOrganizationName("zenonby");
        QCoreApplication::setApplicationName("directory-GetInfo");
//        QCoreApplication::setOrganizationDomain("example.com");

        qRegisterMetaType<std::exception_ptr>("std::exception_ptr");
        qRegisterMetaType<KDirectoryInfoPtr>("KDirectoryInfoPtr");
        qRegisterMetaType<KMimeSizesInfoPtr>("KMimeSizesInfoPtr");
        qRegisterMetaType<HistoryProvider::TDirectoryHistoryPtr>("HistoryProvider::TDirectoryHistoryPtr");

        // Call all dtor-s
        {
            QApplication a(argc, argv);
            GetInfo w;
            w.show();
            res = a.exec();
        }

        // Must be executed before DirectoriesScanOrchestrator::fini()
        DirectoryScanner::instance()->fini();
        DirectoriesScanOrchestrator::instance()->fini();
        HistoryProvider::instance()->fini();
        DirectoryScanSwitch::instance()->fini();
        Settings::instance()->fini();
    }
    catch (const std::exception& ex)
    {
        qFatal(ex.what());
        return res;
    }

    return res;
}
