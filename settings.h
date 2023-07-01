#ifndef SETTINGS_H
#define SETTINGS_H

#include <vector>
#include <mutex>
#include <QSettings>

class Settings : protected QSettings
{
public:
	~Settings() = default;

	static Settings* instance();

	// Call to save changes explicitly
	void fini();

	void setValue(const QString& key, const QVariant& value);
	QVariant value(const QString& key, const QVariant& defaultValue = QVariant()) const;

	typedef std::vector<QVariant> TValueArray;
	TValueArray readArray(const QString& prefix);
	void writeArray(const QString& prefix, const TValueArray& arr);

private:
	Settings();
	Settings(const Settings&) = delete;
	Settings& operator=(const Settings&) = delete;

	mutable std::mutex m_sync;

	static QString getArraySizeValueName(const QString& prefix);
};

#endif // SETTINGS_H
