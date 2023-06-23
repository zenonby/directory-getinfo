#ifndef DIRECTORYPROCESSINGSTATUS_H
#define DIRECTORYPROCESSINGSTATUS_H

// Статусы обработки директорий
enum class DirectoryProcessingStatus
{
	Pending = 0,
	Scanning,
	Ready,
	Skipped,
	Error,
};

#endif // DIRECTORYPROCESSINGSTATUS_H
