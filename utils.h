#ifndef UTILS_H
#define UTILS_H

#include <memory>
#include <chrono>
#include <QString>
#include <QDateTime>
#include "config.h"

#if K_USE_THREAD_NAMES
#define KDBG_CURRENT_THREAD_NAME(wsz) { dbgCurrentThreadName(L"* " wsz); }
#else
#define KDBG_CURRENT_THREAD_NAME(wsz) {}
#endif

// Converts to unified path in order to avoid ambiguities
QString getUnifiedPathName(const QString& path);

// Checks if path is unified
bool isUnifiedPath(const QString& path);

// Checks if parentUnifiedPath is one parent firectories for childUnifiedPath
bool isParentPath(const QString& childUnifiedPath, const QString& parentUnifiedPath);

// Immediate parent directory or null if no parent
QString getImmediateParent(const QString unifiedPath);

template<class TFunc>
auto scope_guard(TFunc&& func) {
    return std::unique_ptr<void, typename std::decay<TFunc>::type>{reinterpret_cast<void*>(1), std::forward<TFunc>(func)};
}

/// Converts utc_clock::time_point to QDateTime
QDateTime convertToQDateTime(std::chrono::utc_clock::time_point dt);

// In debug mode sets thread name visible in VS debugger
void dbgCurrentThreadName(const wchar_t* wszThreadName);

#endif // UTILS_H
