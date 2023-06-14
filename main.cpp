#include "getinfo.h"
#include "dir_scanner/DirectoryScanner.h"

#include <QDebug>
#include <QApplication>

int
main(int argc, char *argv[])
{
    int res = 1;

    try
    {
        qRegisterMetaType<KDirectoryInfoPtr>("KDirectoryInfoPtr");
        qRegisterMetaType<KMimeSizesInfoPtr>("KMimeSizesInfoPtr");

        // Вызвать все dtor
        {
            QApplication a(argc, argv);
            GetInfo w;
            w.show();
            res = a.exec();
        }

        DirectoryScanner::instance()->fini();
    }
    catch (const std::exception& ex)
    {
        qFatal(ex.what());
        return res;
    }

    return res;
}
