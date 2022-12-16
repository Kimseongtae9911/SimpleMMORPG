#include "pch.h"
#include "LuaFunc.h"
#include "CObject.h"

extern array<CObject*, MAX_USER + MAX_NPC> clients;

int API_get_x(lua_State* L)
{
	int id = (int)lua_tointeger(L, -1);
	lua_pop(L, 2);
	int x = clients[id]->GetPosX();
	lua_pushnumber(L, x);
	return 1;
}

int API_get_y(lua_State* L)
{
	int id = (int)lua_tointeger(L, -1);
	lua_pop(L, 2);
	int y = clients[id]->GetPosY();
	lua_pushnumber(L, y);
	return 1;
}

int API_SendMessage(lua_State* L)
{
	int my_id = (int)lua_tointeger(L, -3);
	int user_id = (int)lua_tointeger(L, -2);
	char* mess = (char*)lua_tostring(L, -1);
	lua_pop(L, 4);
	clients[user_id]->Send_Chat_Packet(my_id, mess);
	return 0;
}