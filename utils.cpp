#include <QDebug>
#include <QDir>
#include "utils.h"

QString
getUnifiedPathName(const QString& path)
{
	if (path.isEmpty())
		return path;

	QDir dir(path.endsWith(":") ? (path + "/") : path);

	QString unifiedPath = dir.canonicalPath();

	return unifiedPath;
}

bool
isUnifiedPath(const QString& path)
{
	bool res = path == getUnifiedPathName(path);
	return res;
}

bool
isParentPath(const QString& childUnifiedPath, const QString& parentUnifiedPath)
{
	assert(isUnifiedPath(childUnifiedPath));
	assert(isUnifiedPath(parentUnifiedPath));

	// Избежать случая "C:/Program Files (x86)" и "C:/Program Files"
	const auto& parentPath = getImmediateParent(childUnifiedPath);

	bool res = parentPath.startsWith(parentUnifiedPath);
	return res;
}

QString
getImmediateParent(const QString unifiedPath)
{
	assert(isUnifiedPath(unifiedPath) && !unifiedPath.contains('\\'));

	QString parentPath;

	int len = unifiedPath.length();
	if (1 < len)
	{
		int pos = unifiedPath.lastIndexOf('/', unifiedPath[len - 1] == '/' ? (len - 2) : -1);
		if (0 <= pos)
			parentPath = unifiedPath.left(pos + 1); // Включая slash

		parentPath = getUnifiedPathName(parentPath);
	}

	// Make sure that parent path is also in unified format
	assert(isUnifiedPath(parentPath));

	return parentPath;
}
