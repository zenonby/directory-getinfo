#include <cassert>
#include "SqliteTransaction.h"
#include "SqliteDb.h"

SqliteTransaction::SqliteTransaction(SqliteDb* db)
	: m_db(db),
	  m_complete(false)
{
	assert(!!m_db);

	m_db->execute(L"BEGIN IMMEDIATE TRANSACTION");
}

SqliteTransaction::~SqliteTransaction()
{
	assert(m_complete);

	if (!m_complete)
		rollback();
}

void
SqliteTransaction::commit()
{
	assert(!m_complete);

	m_db->execute(L"COMMIT");
	m_complete = true;
}

void
SqliteTransaction::rollback()
{
	assert(!m_complete);

	m_db->execute(L"ROLLBACK");
	m_complete = true;
}
