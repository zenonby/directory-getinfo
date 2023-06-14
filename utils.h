#ifndef UTILS_H
#define UTILS_H

#include <memory>
#include <QString>

// Унифицированный путь для сравнения путей
QString getUnifiedPathName(const QString& path);

// Яв-ся ли путь унифицированным
bool isUnifiedPath(const QString& path);

// Является ли parentUnifiedPath одной из родительских для childUnifiedPath
bool isParentPath(const QString& childUnifiedPath, const QString& parentUnifiedPath);

// Непосредственная родительская директория или пустая строка
QString getImmediateParent(const QString unifiedPath);

template<class TFunc>
auto scope_guard(TFunc&& func) {
    return std::unique_ptr<void, typename std::decay<TFunc>::type>{reinterpret_cast<void*>(1), std::forward<TFunc>(func)};
}

#endif // UTILS_H
