#ifndef GETINFO_CONFIG_H
#define GETINFO_CONFIG_H

// Thread names are only visible in Visual Studio 2017 version 15.6 and later versions.
#if _WIN32 && _MSC_VER >= 1913 && !defined(NDEBUG)
#define K_USE_THREAD_NAMES 1
#include <windows.h>
#else
#undef K_USE_THREAD_NAMES
#endif

#endif // GETINFO_CONFIG_H
