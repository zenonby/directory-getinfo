TEMPLATE = subdirs

SUBDIRS = \
        GetInfo \
        sqlite3

sqlite3.subdir = libs/sqlite3
GetInfo.file = GetInfo.pro

GetInfo.depends = sqlite3
