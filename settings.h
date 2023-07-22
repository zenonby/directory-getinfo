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

	// Directory where ini file, DB file and other possible files are stored
	const QString& directory() const;
	const std::string& dbFileName() const;

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

	QString m_settingsDirectory;
	std::string m_dbFileName;

	static QString getArraySizeValueName(const QString& prefix);
};

#endif // SETTINGS_H
