#ifndef FILESIZEDIVISOR_H
#define FILESIZEDIVISOR_H

#include <QString>

// File size roundings
enum class FileSizeDivisor
{
	Bytes = 0, // Round to bytes
	KBytes,    // Round to KBytes
	MBytes     // Round to MBytes
};

struct FileSizeDivisorUtils
{
	static QString getDivisorSuffix(FileSizeDivisor divisor);
	static unsigned int getDivisorValue(FileSizeDivisor divisor);
};

#endif // FILESIZEDIVISOR_H
