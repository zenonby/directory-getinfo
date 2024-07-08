#include <QObject>
#include "FileSizeDivisor.h"

QString
FileSizeDivisorUtils::getDivisorSuffix(FileSizeDivisor divisor)
{
    switch (divisor)
    {
    case FileSizeDivisor::Bytes:
        return QObject::tr("B");
    case FileSizeDivisor::KBytes:
        return QObject::tr("KiB");
    case FileSizeDivisor::MBytes:
        return QObject::tr("MiB");
    default:
        assert(!"Unexpected FileSizeDivisor value");
    }

    return "";
}

unsigned int
FileSizeDivisorUtils::getDivisorValue(FileSizeDivisor divisor)
{
    switch (divisor)
    {
    case FileSizeDivisor::Bytes:
        return 1;
    case FileSizeDivisor::KBytes:
        return 1024;
    case FileSizeDivisor::MBytes:
        return 1024 * 1024;
    default:
        assert(!"Unexpected FileSizeDivisor value");
    }

    return 1;
}
