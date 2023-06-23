#include "utils.h"
#include "DirectoryScanSwitch.h"

DirectoryScanSwitch::DirectoryScanSwitch()
{
}

DirectoryScanSwitch*
DirectoryScanSwitch::instance()
{
	static DirectoryScanSwitch s_instance;
	return &s_instance;
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
				{
					// Nothing to be set then. Value is inherited from this parent.
					insertValue = false;
					break;
				}
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
