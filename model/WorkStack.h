#ifndef WORKSTACK_H
#define WORKSTACK_H

#include <stack>
#include <memory>
#include <filesystem>
#include <QString>

#include "model/DirectoryDetails.h"

// ��������� ������������
struct WorkState
{
	// ��������������� ����
	QString	fullPath;

	std::shared_ptr<std::filesystem::directory_iterator> pDirIterator;

	unsigned long subDirCount = 0;
	TMimeDetailsList mimeSizes;
};

class WorkStack
{
public:
	bool empty() const noexcept;

	const WorkState& top() const noexcept;
	WorkState& top() noexcept;

	// unifiedPath - ��������������� ����
	void setFocusedPath(const QString& unifiedPath);

	bool isAboveFocusedPath(const QString& unifiedPath) const noexcept;

	// ��������� ������� ������� �� ����
	void pushScanDirectory(const WorkState& workState);

	// ������� ������� �� �����
	void popScanDirectory(DirectoryProcessingStatus status);

	// ������ ��� ����������� ������ �� ����� ��� ������� ���������� ������
	void popReadyScanDirectory();

	// ��������� ������ ����������� ������ � ������������ (��������)
	void copyReadyScanDirectoryDataToParent(const WorkState& workState);

	// ������ ������, ��������� ���������� �� �����
	void popErrorScanDirectory(const QString& workDirPath);

private:
	std::stack<WorkState> m_scanDirectories;

	// ������������� ����� �����
	QString m_focusedParentPath;
};

#endif // WORKSTACK_H
