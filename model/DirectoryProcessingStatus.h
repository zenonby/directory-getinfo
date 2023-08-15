#ifndef DIRECTORYPROCESSINGSTATUS_H
#define DIRECTORYPROCESSINGSTATUS_H

// Directory scan status
enum class DirectoryProcessingStatus
{
	Pending = 0,
	Scanning,
	Ready,
	Skipped,
	Error,
};

#endif // DIRECTORYPROCESSINGSTATUS_H
