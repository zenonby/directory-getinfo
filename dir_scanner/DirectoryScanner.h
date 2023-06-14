#ifndef DIRECTORYSCANNER_H
#define DIRECTORYSCANNER_H

#include <set>
#include <mutex>
#include <condition_variable>
#include <stack>
#include <filesystem>

#include "IDirectoryScannerEventSink.h"
#include "model/DirectoryDetails.h"

class DirectoryScanner
{
public:
	~DirectoryScanner();

	static DirectoryScanner* instance();

	void setRootPath(const QString& rootPath);

	// Вызвать перед завершением программы для завершения рабочего потока
	void fini();

	void subscribe(IDirectoryScannerEventSink* eventSink);
	void unsubscribe(IDirectoryScannerEventSink* eventSink);

	void setFocusedPath(const QString& dirPath);

private:
	DirectoryScanner();
	DirectoryScanner(const DirectoryScanner&) = delete;
	DirectoryScanner& operator=(const DirectoryScanner&) = delete;

	mutable std::mutex m_sync;
	QString m_rootPath;

	mutable std::mutex m_syncFocusedParentPath;
	std::optional<QString> m_pendingFocusedParentPath; // Ожидает установки
	QString m_focusedParentPath;		// Установленный фокус скана

	void checkPendingFocusedParentPathAssignment();
	void setFocusedPathWithLocking(const QString& dirPath);

	// Состояние сканирования
	struct WorkState
	{
		// Унифицированный путь
		QString	fullPath;

		std::shared_ptr<std::filesystem::directory_iterator> pDirIterator;

		unsigned long subDirCount = 0;
		TMimeDetailsList mimeSizes;
	};

	// Стэк сканируемых директорий, дочерняя директория наверху
	std::stack<WorkState> m_scanDirectories;

	// Потребители сообщений об обновлении данных
	std::set<IDirectoryScannerEventSink*> m_eventSinks;

	void prepareDtoAndNotifyEventSinks(
		const QString& dirPath,
		const DirectoryDetails& dirDetails,
		bool acquireLock = true);
	void postDirInfo(KDirectoryInfoPtr pDirInfo);
	void postMimeSizesInfo(KMimeSizesInfoPtr pDirInfo);

	// Возвращает true в случае успешного завершения, false в случае отмены или паузы
	bool scanDirectory(WorkState* workState);

	// Добавляет рабочее задание на стек
	void pushScanDirectory(const WorkState& workState);

	// Удаляет задание из стека
	void popScanDirectory(DirectoryProcessingStatus status);

	// Удалет уже завершенную задачу из стека при попытке повторного вызова
	void popReadyScanDirectory();

	// Переносит данные завершенной задачи в родительскую (суммрует)
	void copyReadyScanDirectoryDataToParent(const WorkState& workState);

	// Удалет задачу, вызвавшую исключение из стека
	void popErrorScanDirectory(const QString& workDirPath);

	//
	// Рабочий поток
	//

	std::thread m_threadWorker;
	bool m_stopWorker = false;
	bool m_isCancellationRequested = false;
	bool m_isScanRunning = false;
	std::condition_variable m_scanningDone;

	void setScanRunning(bool running);

	void requestCancellationAndWait(std::unique_lock<std::mutex>& lock_);
	bool isCancellationRequested() const noexcept;

	bool isDestroying() const noexcept;
	void stopWorker() noexcept;

	void worker();

	//
	// Поток уведомлений
	//

	std::thread m_threadNotifier;

	// DTOs обновлений директории
	typedef std::map<
		QString,	// Унифицированный путь
		KDirectoryInfoPtr
	> TDirInfoDTOs;
	TDirInfoDTOs m_dirInfos;

	// DTOs обновлений размеров по типам файлов
	typedef std::map<
		QString,	// Унифицированный путь
		KMimeSizesInfoPtr
	> TMimeSizesInfoDTOs;
	TMimeSizesInfoDTOs m_mimeSizesInfos;

	void notifier();
};

#endif // DIRECTORYSCANNER_H
