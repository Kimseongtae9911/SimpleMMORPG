#pragma once
#include <sqlext.h>

class CDatabase
{
public:
	CDatabase();
	~CDatabase();

	bool Initialize();

	char* GetPlayerInfo(string name);
	void SavePlayerInfo(int id, int x, int y);

private:
	void Show_Error(SQLHANDLE hHandle, SQLSMALLINT hType, RETCODE RetCode);

private:
	SQLHENV m_henv;
	SQLHDBC m_hdbc;
	SQLHSTMT m_hstmt;
};

