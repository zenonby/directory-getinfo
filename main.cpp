#include "getinfo.h"
#include "dir_scanner/DirectoryScanner.h"
#include "model/DirectoryScanSwitch.h"
#include "settings.h"

#include <exception>
#include <QDebug>
#include <QApplication>

int
main(int argc, char *argv[])
{
    int res = 1;

    try
    {
        QCoreApplication::setOrganizationName("zenonby");
        QCoreApplication::setApplicationName("directory-GetInfo");
//        QCoreApplication::setOrganizationDomain("example.com");

        qRegisterMetaType<std::exception_ptr>("std::exception_ptr");
        qRegisterMetaType<KDirectoryInfoPtr>("KDirectoryInfoPtr");
        qRegisterMetaType<KMimeSizesInfoPtr>("KMimeSizesInfoPtr");

        // Call all dtor-s
        {
            QApplication a(argc, argv);
            GetInfo w;
            w.show();
            res = a.exec();
        }

        DirectoryScanner::instance()->fini();
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
