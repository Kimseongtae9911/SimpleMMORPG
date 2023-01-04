#pragma once
#include <sqlext.h>

class CDatabase
{
public:
	CDatabase();
	~CDatabase();

	bool Initialize();

	char* GetPlayerInfo(string name);
	void SavePlayerInfo(USER_INFO& ui);
	void MakeNewInfo(char* name);

private:
	void Show_Error(SQLHANDLE hHandle, SQLSMALLINT hType, RETCODE RetCode);

private:
	SQLHENV m_henv;
	SQLHDBC m_hdbc;
	SQLHSTMT m_hstmt;
};

