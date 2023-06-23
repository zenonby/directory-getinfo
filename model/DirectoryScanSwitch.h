#ifndef DIRECTORYSCANSWITCH_H
#define DIRECTORYSCANSWITCH_H

#include <map>
#include <mutex>
#include <QString>

class DirectoryScanSwitch
{
	DirectoryScanSwitch();
	DirectoryScanSwitch(const DirectoryScanSwitch&) = delete;
	DirectoryScanSwitch(DirectoryScanSwitch&&) = delete;

	DirectoryScanSwitch& operator()(const DirectoryScanSwitch&) = delete;
	DirectoryScanSwitch& operator()(DirectoryScanSwitch&&) = delete;

public:
	static DirectoryScanSwitch* instance();

	bool isEnabled(const QString& unifiedPath) const noexcept;
	void setEnabled(const QString& unifiedPath, bool enableScan = true);

private:

	mutable std::mutex m_sync;

	std::map<
		QString,	// unified path
		bool		// scan directory
	> m_scanSwitches;
};

#endif // DIRECTORYSCANSWITCH_H
