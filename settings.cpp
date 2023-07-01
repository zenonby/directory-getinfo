#include <QDebug>
#include <QCoreApplication>
#include "settings.h"
#include "utils.h"

#define ARR_ITEM_NAME "item"

Settings::Settings()
	: QSettings(
		QSettings::IniFormat,
		QSettings::UserScope,
		QCoreApplication::organizationName(),
		QCoreApplication::applicationName())
{
}

Settings*
Settings::instance()
{
	static Settings s_instance;
	return &s_instance;
}

void
Settings::fini()
{
	std::scoped_lock lock_(m_sync);
	sync();
}

void
Settings::setValue(const QString& key, const QVariant& value)
{
	std::scoped_lock lock_(m_sync);

	QSettings::setValue(key, value);
}

QVariant
Settings::value(const QString& key, const QVariant& defaultValue) const
{
	std::scoped_lock lock_(m_sync);

	return QSettings::value(key, defaultValue);
}

QString
Settings::getArraySizeValueName(const QString& prefix)
{
	return prefix + "/size";
}

Settings::TValueArray
Settings::readArray(const QString& prefix)
{
	std::scoped_lock lock_(m_sync);

	// Read array size
	const int itemCount = QSettings::value(getArraySizeValueName(prefix)).toInt();

	QSettings::beginReadArray(prefix);
	auto scopedEndArray = scope_guard([&](auto) {
		try
		{
			QSettings::endArray();
		}
		catch (...)
		{
			assert(!"Exception in QSettings::endArray");
		}
	});

	//
	// Read values
	//

	TValueArray arr;
	arr.reserve(itemCount);

	for (int i = 0; i < itemCount; ++i)
	{
		QSettings::setArrayIndex(i);

		const auto& value_ = QSettings::value(ARR_ITEM_NAME);
		arr.push_back(value_);
	}

	return arr;
}

void
Settings::writeArray(const QString& prefix, const TValueArray& arr)
{
	std::scoped_lock lock_(m_sync);

	// Read old array size
	const int itemCountOld = QSettings::value(getArraySizeValueName(prefix)).toInt();

	// New array size
	const int itemCount = static_cast<int>(arr.size());

	// Write values
	{
		QSettings::beginWriteArray(prefix);
		auto scopedEndArray = scope_guard([&](auto) {
			try
			{
				QSettings::endArray();
			}
			catch (...)
			{
				assert(!"Exception in QSettings::endArray");
			}
			});

		// If item count reduced, clear values
		for (int i = itemCount; i < itemCountOld; ++i)
		{
			QSettings::setArrayIndex(i);
			QSettings::remove(ARR_ITEM_NAME);
		}

		// Write values
		for (int i = 0; i < itemCount; ++i) {
			QSettings::setArrayIndex(i);

			const auto& value_ = arr[i];
			QSettings::setValue(ARR_ITEM_NAME, value_);
		}
	}

	// Write new array size (AFTER updating/removing values)
	QSettings::setValue(getArraySizeValueName(prefix), itemCount);
}
