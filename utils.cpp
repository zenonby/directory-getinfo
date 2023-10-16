#include <format>
#include <QDebug>
#include <QDir>
#include <QTimeZone>
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

	// Avoid "C:/Program Files (x86)" and "C:/Program Files" cases
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
			parentPath = unifiedPath.left(pos + 1); // Including slash

		parentPath = getUnifiedPathName(parentPath);
	}

	// Make sure that parent path is also in unified format
	assert(isUnifiedPath(parentPath));

	return parentPath;
}

QDateTime
convertToQDateTime(std::chrono::utc_clock::time_point dt)
{
	// Conversion from utc_clock::time_since_epoch to QDateTime via milliseconds 
	//	is not precise, thus first cast to_sys.
	auto tSys = std::chrono::utc_clock::to_sys(dt);
	auto mSecsSinceEpoch = duration_cast<std::chrono::milliseconds>(tSys.time_since_epoch()).count();
	QDateTime qDt = QDateTime::fromMSecsSinceEpoch(mSecsSinceEpoch, QTimeZone::utc());

#ifndef NDEBUG
	auto sDate = std::format("{0:%F}T{0:%T%z}", dt);
	auto sQtDate = qDt.toString("yyyy.MM.dd hh:mm:ss.zzz");
#endif

	return qDt;
}

void
dbgCurrentThreadName(const wchar_t* wszThreadName)
{
#ifdef K_USE_THREAD_NAMES
	::SetThreadDescription(::GetCurrentThread(), wszThreadName);
#endif
}
