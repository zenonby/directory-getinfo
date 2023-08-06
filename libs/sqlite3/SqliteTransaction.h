#ifndef SQLITETRANSACTION_H
#define SQLITETRANSACTION_H

class SqliteDb;

// Object lifetime cannot exceed lifetime of SqliteDb instance that was used to create the former
class SqliteTransaction
{
	friend class SqliteDb;

public:
	~SqliteTransaction();

	void commit();
	void rollback();

private:
	explicit SqliteTransaction(SqliteDb* db);

	SqliteTransaction(const SqliteTransaction&) = delete;
	SqliteTransaction(SqliteTransaction&&) = delete;
	SqliteTransaction& operator=(const SqliteTransaction&) = delete;
	SqliteTransaction& operator=(SqliteTransaction&&) = delete;

	SqliteDb* m_db;

	// Commited or rolled back
	bool m_complete;
};

#endif // SQLITETRANSACTION_H
