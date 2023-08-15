TEMPLATE = subdirs

SUBDIRS = \
        GetInfo \
        yasw

yasw.subdir = libs/yasw
GetInfo.file = GetInfo.pro

GetInfo.depends = yasw
