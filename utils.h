#ifndef UTILS_H
#define UTILS_H

#include <memory>
#include <QString>

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

#endif // UTILS_H
