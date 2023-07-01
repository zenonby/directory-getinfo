#include "utils.h"
#include "settings.h"
#include "DirectoryScanSwitch.h"

#define DIRECTORY_SCAN_SWITCH_PREFIX "directory_scan_switches"
#define DIRECTORY_SCAN_SWITCH_ENABLED_PREFIX DIRECTORY_SCAN_SWITCH_PREFIX "/enabled"
#define DIRECTORY_SCAN_SWITCH_DISABLED_PREFIX DIRECTORY_SCAN_SWITCH_PREFIX "/disabled"

DirectoryScanSwitch::DirectoryScanSwitch()
{
	readSettings();
}

DirectoryScanSwitch*
DirectoryScanSwitch::instance()
{
	static DirectoryScanSwitch s_instance;
	return &s_instance;
}

void
DirectoryScanSwitch::fini()
{
	std::scoped_lock lock_(m_sync);

	writeSettings();
}

void
DirectoryScanSwitch::readSettings()
{
	const auto& enabled = Settings::instance()->readArray(DIRECTORY_SCAN_SWITCH_ENABLED_PREFIX);
	const auto& disabled = Settings::instance()->readArray(DIRECTORY_SCAN_SWITCH_DISABLED_PREFIX);

	std::for_each(enabled.cbegin(), enabled.cend(), [&](auto path) {
		const QString& sPath = path.toString();
		if (!sPath.isEmpty())
			m_scanSwitches.emplace(std::make_pair(sPath, true));
	});

	std::for_each(disabled.cbegin(), disabled.cend(), [&](auto path) {
		const QString& sPath = path.toString();
		if (!sPath.isEmpty())
			m_scanSwitches.emplace(std::make_pair(sPath, false));
	});
}

void
DirectoryScanSwitch::writeSettings()
{
	Settings::TValueArray enabled, disabled;

	enabled.reserve(m_scanSwitches.size());
	disabled.reserve(m_scanSwitches.size());

	for (auto iter = m_scanSwitches.cbegin(); iter != m_scanSwitches.cend(); ++iter)
	{
		const auto& path = iter->first;
		bool isEnabled = iter->second;

		if (isEnabled)
			enabled.push_back(path);
		else
			disabled.push_back(path);
	}

	Settings::instance()->writeArray(DIRECTORY_SCAN_SWITCH_ENABLED_PREFIX, enabled);
	Settings::instance()->writeArray(DIRECTORY_SCAN_SWITCH_DISABLED_PREFIX, disabled);
}

bool
DirectoryScanSwitch::isEnabled(const QString& unifiedPath) const noexcept
{
	std::scoped_lock lock_(m_sync);

	for (auto path = unifiedPath; !path.isEmpty(); path = getImmediateParent(path))
	{
		auto iter = m_scanSwitches.find(path);
		if (iter != m_scanSwitches.end())
		{
			bool enabled = iter->second;
			return enabled;
		}
	}

	// By default scan is enabled
	return true;
}

void
DirectoryScanSwitch::setEnabled(const QString& unifiedPath, bool enableScan)
{
	std::scoped_lock lock_(m_sync);

	// Check if value is defined for this particular path (not for a parent)
	auto iter = m_scanSwitches.find(unifiedPath);
	if (iter != m_scanSwitches.end() &&
		iter->second != enableScan)
	{
		// Purge old value
		m_scanSwitches.erase(iter);
	}

	bool insertValue = true;
	if (!unifiedPath.isEmpty())
	{
		QString path;

		// Check if parent value is the same
		for (path = getImmediateParent(unifiedPath);
			 !path.isEmpty();
			 path = getImmediateParent(path))
		{
			auto iter = m_scanSwitches.find(path);
			if (iter != m_scanSwitches.end())
			{
				bool parentEnabled = iter->second;
				if (parentEnabled == enableScan)
					// Nothing to be set then. Value is inherited from this parent.
					insertValue = false;

				break;
			}
		}

		// Also if enableScan and there's no parent with disable scan state,
		//	nothing to be set (enabled by default)
		if (path.isEmpty() && enableScan)
			insertValue = false;
	}

	// Overwrite all children states
	for (auto iter = m_scanSwitches.begin(); iter != m_scanSwitches.end();)
	{
		const auto& path = iter->first;
		if (path.startsWith(unifiedPath))
			iter = m_scanSwitches.erase(iter);
		else
			++iter;
	}

	if (insertValue)
		m_scanSwitches.insert(std::make_pair(unifiedPath, enableScan));
}
