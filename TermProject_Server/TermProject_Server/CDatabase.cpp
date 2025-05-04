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

    if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO)
    {
        std::cout << "Failed to allocate environment handle." << std::endl;
        return false;
    }

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
					cout << "DB Connected\n";
					retcode = SQLAllocHandle(SQL_HANDLE_STMT, m_hdbc, &m_hstmt);
				}


			}
		}
	}
	return true;
}

void CDatabase::GetPlayerInfo(string name, USER_INFO& userInfo)
{
    // player_name, pos_x, pos_y, player_level, player_exp, player_maxhp, player_hp
    SQLINTEGER user_level = -1, user_exp = -1, user_maxhp = -1, user_hp = -1, user_maxmp = -1, user_mp = -1, moneycnt = -1;
    SQLSMALLINT user_pos_x = -1, user_pos_y = -1, item1 = -1, item2 = -1, item3 = -1, item4 = -1, item5 = -1, item6 = -1;
    SQLWCHAR user_name[NAME_SIZE + 1] = { '\0' };
    SQLLEN cb_pos_x = 0, cb_pos_y = 0, cb_name = 0, cb_level = 0, cb_exp = 0, cb_maxhp = 0, cb_hp = 0, cb_maxmp = 0, cb_mp = 0;
    SQLLEN cb_moneycnt = 0, cb_item1 = 0, cb_item2 = 0, cb_item3 = 0, cb_item4 = 0, cb_item5 = 0, cb_item6 = 0;
    string temp = "EXEC get_user_data " + name;
    wstring sql;
    sql.assign(temp.cbegin(), temp.cend());

    SQLRETURN retcode = SQLExecDirect(m_hstmt, (SQLWCHAR*)sql.c_str(), SQL_NTS);
    if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
        retcode = SQLBindCol(m_hstmt, 1, SQL_C_WCHAR, &user_name, NAME_SIZE + 1, &cb_name);
        retcode = SQLBindCol(m_hstmt, 2, SQL_C_SHORT, &user_pos_x, sizeof(short), &cb_pos_x);
        retcode = SQLBindCol(m_hstmt, 3, SQL_C_SHORT, &user_pos_y, sizeof(short), &cb_pos_y);

        retcode = SQLBindCol(m_hstmt, 4, SQL_C_LONG, &user_level, sizeof(int), &cb_level);
        retcode = SQLBindCol(m_hstmt, 5, SQL_C_LONG, &user_exp, sizeof(int), &cb_exp);
        retcode = SQLBindCol(m_hstmt, 6, SQL_C_LONG, &user_maxhp, sizeof(int), &cb_maxhp);
        retcode = SQLBindCol(m_hstmt, 7, SQL_C_LONG, &user_hp, sizeof(int), &cb_hp);
        retcode = SQLBindCol(m_hstmt, 8, SQL_C_LONG, &user_maxmp, sizeof(int), &cb_maxmp);
        retcode = SQLBindCol(m_hstmt, 9, SQL_C_LONG, &user_mp, sizeof(int), &cb_mp);

        retcode = SQLBindCol(m_hstmt, 10, SQL_C_SHORT, &item1, sizeof(short), &cb_item1);
        retcode = SQLBindCol(m_hstmt, 11, SQL_C_SHORT, &item2, sizeof(short), &cb_item2);
        retcode = SQLBindCol(m_hstmt, 12, SQL_C_SHORT, &item3, sizeof(short), &cb_item3);
        retcode = SQLBindCol(m_hstmt, 13, SQL_C_SHORT, &item4, sizeof(short), &cb_item4);
        retcode = SQLBindCol(m_hstmt, 14, SQL_C_SHORT, &item5, sizeof(short), &cb_item5);
        retcode = SQLBindCol(m_hstmt, 15, SQL_C_SHORT, &item6, sizeof(short), &cb_item6);
        retcode = SQLBindCol(m_hstmt, 16, SQL_C_LONG, &moneycnt, sizeof(int), &cb_moneycnt);

        retcode = SQLFetch(m_hstmt);
        if (retcode == SQL_ERROR || retcode == SQL_SUCCESS_WITH_INFO)
            Show_Error(m_hstmt, SQL_HANDLE_STMT, retcode);
        if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO)
        {
            wprintf(L"%s %3d %3d %3d %3d %3d %3d %3d %3d\n", user_name, user_pos_x, user_pos_y, user_level, user_exp, user_maxhp, user_hp, user_maxmp, user_mp);
           
            strcpy_s(userInfo.name, (char*)user_name);
            userInfo.pos_x = user_pos_x;
            userInfo.pos_y = user_pos_y;
            userInfo.level = user_level;
            userInfo.exp = user_exp;
            userInfo.max_hp = user_maxhp;
            userInfo.cur_hp = user_hp;
            userInfo.max_mp = user_maxmp;
            userInfo.cur_mp = user_mp;
            userInfo.item1 = item1;
            userInfo.item2 = item2;
            userInfo.item3 = item3;
            userInfo.item4 = item4;
            userInfo.item5 = item5;
            userInfo.item6 = item6;
            userInfo.moneycnt = moneycnt;
        }
        else {
            if (user_name[0] == '\0') {
                userInfo.pos_x = -1;
                userInfo.pos_y = -1;
                userInfo.level = -1;
                userInfo.exp = -1;
                userInfo.max_hp = -1;
                userInfo.cur_hp = -1;
                userInfo.max_mp = -1;
                userInfo.cur_mp = -1;
                userInfo.item1 = -1;
                userInfo.item2 = -1;
                userInfo.item3 = -1;
                userInfo.item4 = -1;
                userInfo.item5 = -1;
                userInfo.item6 = -1;
                userInfo.moneycnt = -1;
                SQLCloseCursor(m_hstmt);
                SQLFreeStmt(m_hstmt, SQL_UNBIND);
            }
        }

    }
    else 
        Show_Error(m_hstmt, SQL_HANDLE_STMT, retcode);

    if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
        SQLCloseCursor(m_hstmt);
        SQLFreeStmt(m_hstmt, SQL_UNBIND);
    }
    SQLCloseCursor(m_hstmt);
    SQLFreeStmt(m_hstmt, SQL_UNBIND);
}

void CDatabase::SavePlayerInfo(USER_INFO& ui)
{
    // player_name, pos_x, pos_y, player_level, player_exp, player_maxhp, player_hp
    string name{ ui.name };
    SQLINTEGER user_level = -1, user_exp = -1, user_maxhp = -1, user_hp = -1, user_maxmp = -1, user_mp = -1, moneycnt = -1;
    SQLSMALLINT user_pos_x = -1, user_pos_y = -1, item1 = -1, item2 = -1, item3 = -1, item4 = -1, item5 = -1, item6 = -1;
    SQLWCHAR user_name[NAME_SIZE + 1] = { '\0' };
    SQLLEN cb_pos_x = 0, cb_pos_y = 0, cb_name = 0, cb_level = 0, cb_exp = 0, cb_maxhp = 0, cb_hp = 0, cb_maxmp = 0, cb_mp = 0;
    SQLLEN cb_moneycnt = 0, cb_item1 = 0, cb_item2 = 0, cb_item3 = 0, cb_item4 = 0, cb_item5 = 0, cb_item6 = 0;
    string temp = "EXEC save_user_data @Param1 = " + name + ", @Param2 = " + to_string(ui.pos_x) + ", @Param3 = " + to_string(ui.pos_y) + ", @Param4 = " + to_string(ui.level)
        + ", @Param5 = " + to_string(ui.exp) + ", @Param6 = " + to_string(ui.max_hp) + ", @Param7 = " + to_string(ui.cur_hp) + ", @Param8 = " + to_string(ui.max_mp) + ", @Param9 = " + to_string(ui.cur_mp)
        + ", @Param10 = " + to_string(ui.item1) + ", @Param11 = " + to_string(ui.item2) + ", @Param12 = " + to_string(ui.item3) + ", @Param13 = " + to_string(ui.item4) + ", @Param14 = " + to_string(ui.item5)
        + ", @Param15 = " + to_string(ui.item6) + ", @Param16 = " + to_string(ui.moneycnt);
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
        retcode = SQLBindCol(m_hstmt, 8, SQL_C_LONG, &user_maxmp, sizeof(int), &cb_maxmp);
        retcode = SQLBindCol(m_hstmt, 9, SQL_C_LONG, &user_mp, sizeof(int), &cb_mp);

        retcode = SQLBindCol(m_hstmt, 10, SQL_C_SHORT, &item1, sizeof(short), &cb_item1);
        retcode = SQLBindCol(m_hstmt, 11, SQL_C_SHORT, &item2, sizeof(short), &cb_item2);
        retcode = SQLBindCol(m_hstmt, 12, SQL_C_SHORT, &item3, sizeof(short), &cb_item3);
        retcode = SQLBindCol(m_hstmt, 13, SQL_C_SHORT, &item4, sizeof(short), &cb_item4);
        retcode = SQLBindCol(m_hstmt, 14, SQL_C_SHORT, &item5, sizeof(short), &cb_item5);
        retcode = SQLBindCol(m_hstmt, 15, SQL_C_SHORT, &item6, sizeof(short), &cb_item6);
        retcode = SQLBindCol(m_hstmt, 16, SQL_C_LONG, &moneycnt, sizeof(int), &cb_moneycnt);

        retcode = SQLFetch(m_hstmt);
        if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO)
        {
            cout << "Save user data" << endl;
        }
        else {

            SQLCloseCursor(m_hstmt);
            SQLFreeStmt(m_hstmt, SQL_UNBIND);

        }

    }
    else
        Show_Error(m_hstmt, SQL_HANDLE_STMT, retcode);

    if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
        SQLCloseCursor(m_hstmt);
        SQLFreeStmt(m_hstmt, SQL_UNBIND);
    }

    SQLCloseCursor(m_hstmt);
    SQLFreeStmt(m_hstmt, SQL_UNBIND);
}

void CDatabase::MakeNewInfo(char* name)
{
    string s(name);
    //string temp = "EXEC make_new_info @Param = " + s;
    string temp = "EXEC get_user_data " + s;
    
    wstring sql;
    sql.assign(temp.cbegin(), temp.cend());
    
    SQLRETURN retcode = SQLExecDirect(m_hstmt, (SQLWCHAR*)sql.c_str(), SQL_NTS);
    if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {

        retcode = SQLFetch(m_hstmt);
        if (retcode == SQL_ERROR || retcode == SQL_SUCCESS_WITH_INFO)
            Show_Error(m_hstmt, SQL_HANDLE_STMT, retcode);
        if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO || retcode == SQL_NO_DATA)
        {
            cout << "Made New User Data" << endl;
        }
        else {
            cout << "Failed to make new Data" << endl;
        }

    }
    else
        Show_Error(m_hstmt, SQL_HANDLE_STMT, retcode);

    if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
        SQLCloseCursor(m_hstmt);
        SQLFreeStmt(m_hstmt, SQL_UNBIND);
    }

    SQLCloseCursor(m_hstmt);
    SQLFreeStmt(m_hstmt, SQL_UNBIND);
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
