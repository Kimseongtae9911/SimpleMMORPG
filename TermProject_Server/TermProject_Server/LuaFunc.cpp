#include "pch.h"
#include "LuaFunc.h"
#include "CObject.h"

extern array<CObject*, MAX_USER + MAX_NPC> clients;
extern char g_tilemap[W_HEIGHT][W_WIDTH];
extern default_random_engine dre;

int API_Initialize(lua_State* L)
{
	uniform_int_distribution<> uid;
	int id = (int)lua_tointeger(L, -1);
	lua_pop(L, 2);

	while (true) {
		bool can_put = false;
		clients[id]->SetPos(rand() % W_WIDTH, rand() % W_HEIGHT);

		//리스폰 장소에 있는 경우
		if ((clients[id]->GetPosX() % 100 >= 40 && clients[id]->GetPosX() % 100 <= 54) &&
			(clients[id]->GetPosY() % 100 >= 40 && clients[id]->GetPosY() % 100 <= 54))
			continue;

		if (g_tilemap[clients[id]->GetPosY()][clients[id]->GetPosX()] == 'G' || g_tilemap[clients[id]->GetPosY()][clients[id]->GetPosX()] == 'D')
			can_put = true;

		if (can_put == true)
			break;
	}
	dynamic_cast<CNpc*>(clients[id])->SetRespawnPos(clients[id]->GetPosX(), clients[id]->GetPosY());
	int n = uid(dre) % 5;
	int level = (clients[id]->GetPosX() / 100) * 5 + (clients[id]->GetPosY() / 100) * 5 + n;
	clients[id]->SetLevel(level + 5);
	clients[id]->SetMaxHp(80 + clients[id]->GetLevel() * 50);
	clients[id]->SetCurHp(clients[id]->GetMaxHp());
	clients[id]->SetPower(10 + clients[id]->GetLevel() * 5);

	return 1;
}

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