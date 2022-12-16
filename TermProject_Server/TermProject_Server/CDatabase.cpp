#include "pch.h"
#include "CDatabase.h"

CDatabase::CDatabase()
{

}

CDatabase::~CDatabase()
{
    SQLFreeHandle(SQL_HANDLE_STMT, m_hstmt);
    SQLDisconnect(m_hdbc);
    SQLFreeHandle(SQL_HANDLE_DBC, m_hdbc);
    SQLFreeHandle(SQL_HANDLE_ENV, m_henv);
}

bool CDatabase::Initialize()
{
    setlocale(LC_ALL, "korean");

	SQLRETURN retcode = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &m_henv);

	// Set the ODBC version environment attribute  
	if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
		retcode = SQLSetEnvAttr(m_henv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER*)SQL_OV_ODBC3, 0);

		// Allocate connection handle  
		if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
			retcode = SQLAllocHandle(SQL_HANDLE_DBC, m_henv, &m_hdbc);

			// Set login timeout to 5 seconds  
			if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
				SQLSetConnectAttr(m_hdbc, SQL_LOGIN_TIMEOUT, (SQLPOINTER)5, 0);

				// Connect to data source  
				retcode = SQLConnect(m_hdbc, (SQLWCHAR*)L"ServerTermProject", SQL_NTS, (SQLWCHAR*)NULL, 0, NULL, 0);

				// Allocate statement handle  
				if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
					cout << "ODBC Connected\n";
					retcode = SQLAllocHandle(SQL_HANDLE_STMT, m_hdbc, &m_hstmt);
				}


			}
		}
	}
	return true;
}

char* CDatabase::GetPlayerInfo(string name)
{
    // player_name, pos_x, pos_y, player_level, player_exp, player_maxhp, player_hp
    SQLINTEGER user_level = -1, user_exp = -1, user_maxhp = -1, user_hp = -1;
    SQLSMALLINT user_pos_x = -1, user_pos_y = -1;
    SQLWCHAR user_name[NAME_SIZE + 1] = { '\0' };
    SQLLEN cb_pos_x = 0, cb_pos_y = 0, cb_name = 0, cb_level = 0, cb_exp = 0, cb_maxhp = 0, cb_hp = 0;

    string temp = "EXEC get_user_data " + name;
    wstring sql;
    sql.assign(temp.cbegin(), temp.cend());

    USER_INFO* user_info = new USER_INFO;

    SQLRETURN retcode = SQLExecDirect(m_hstmt, (SQLWCHAR*)sql.c_str(), SQL_NTS);
    if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
        retcode = SQLBindCol(m_hstmt, 1, SQL_C_WCHAR, &user_name, NAME_SIZE + 1, &cb_name);
        retcode = SQLBindCol(m_hstmt, 2, SQL_C_SHORT, &user_pos_x, sizeof(short), &cb_pos_x);
        retcode = SQLBindCol(m_hstmt, 3, SQL_C_SHORT, &user_pos_y, sizeof(short), &cb_pos_y);

        retcode = SQLBindCol(m_hstmt, 4, SQL_C_LONG, &user_level, sizeof(int), &cb_level);
        retcode = SQLBindCol(m_hstmt, 5, SQL_C_LONG, &user_exp, sizeof(int), &cb_exp);
        retcode = SQLBindCol(m_hstmt, 6, SQL_C_LONG, &user_maxhp, sizeof(int), &cb_maxhp);
        retcode = SQLBindCol(m_hstmt, 7, SQL_C_LONG, &user_hp, sizeof(int), &cb_hp);

        retcode = SQLFetch(m_hstmt);
        if (retcode == SQL_ERROR || retcode == SQL_SUCCESS_WITH_INFO)
            Show_Error(m_hstmt, SQL_HANDLE_STMT, retcode);
        if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO)
        {
            wprintf(L"%s %3d %3d %3d %3d %3d %3d\n", user_name, user_pos_x, user_pos_y, user_level, user_exp, user_maxhp, user_hp);
           
            strcpy_s(user_info->name, (char*)user_name);
            user_info->pos_x = user_pos_x;
            user_info->pos_y = user_pos_y;
            user_info->level = user_level;
            user_info->exp = user_exp;
            user_info->max_hp = user_maxhp;
            user_info->cur_hp = user_hp;
        }
        else {
            if (user_name[0] == '\0') {
                user_info->pos_x = -1;
                user_info->pos_y = -1;
                user_info->level = -1;
                user_info->exp = -1;
                user_info->max_hp = -1;
                user_info->cur_hp = -1;
            }
        }

    }
    else 
        Show_Error(m_hstmt, SQL_HANDLE_STMT, retcode);

    if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
        SQLCloseCursor(m_hstmt);
        SQLFreeStmt(m_hstmt, SQL_UNBIND);
    }

    return reinterpret_cast<char*>(user_info);
}

void CDatabase::SavePlayerInfo(int id, int x, int y)
{
}

void CDatabase::Show_Error(SQLHANDLE hHandle, SQLSMALLINT hType, RETCODE RetCode)
{
    SQLSMALLINT iRec = 0;
    SQLINTEGER iError;
    WCHAR wszMessage[1000];
    WCHAR wszState[SQL_SQLSTATE_SIZE + 1];
    if (RetCode == SQL_INVALID_HANDLE) {
        fwprintf(stderr, L"Invalid handle!\n");
        return;
    }
    while (SQLGetDiagRec(hType, hHandle, ++iRec, wszState, &iError, wszMessage,
        (SQLSMALLINT)(sizeof(wszMessage) / sizeof(WCHAR)), (SQLSMALLINT*)NULL) == SQL_SUCCESS) {
        // Hide data truncated..
        if (wcsncmp(wszState, L"01004", 5)) {
            fwprintf(stderr, L"[%5.5s] %s (%d)\n", wszState, wszMessage, iError);
        }
    }
}
