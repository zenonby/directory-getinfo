#include "getinfo.h"
#include "dir_scanner/DirectoryScanner.h"

#include <QApplication>

int
main(int argc, char *argv[])
{
    int res;

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

    return res;
}
