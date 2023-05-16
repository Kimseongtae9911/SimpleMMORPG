#include "pch.h"
#include "LuaFunc.h"
#include "CNetworkMgr.h"
#include "GameUtil.h"
#include "CObject.h"

int API_Initialize(lua_State* L)
{
	uniform_int_distribution<> uid;
	int id = (int)lua_tointeger(L, -1);
	lua_pop(L, 2);

	CNpc* npc = reinterpret_cast<CNpc*>(CNetworkMgr::GetInstance()->GetCObject(id));

	while (true) {
		bool can_put = false;
		npc->SetPos(rand() % W_WIDTH, rand() % W_HEIGHT);

		//리스폰 장소에 있는 경우
		if ((npc->GetPosX() % 100 >= 40 && npc->GetPosX() % 100 <= 54) &&
			(npc->GetPosY() % 100 >= 40 && npc->GetPosY() % 100 <= 54))
			continue;

		if (GameUtil::GetTile(npc->GetPosY(), npc->GetPosX()) == 'G' || GameUtil::GetTile(npc->GetPosY(), npc->GetPosX()) == 'D')
			can_put = true;

		if (can_put == true)
			break;
	}
	npc->SetRespawnPos(npc->GetPosX(), npc->GetPosY());
	int sectionX = static_cast<int>(npc->GetPosX() / SECTION_SIZE);
	int sectionY = static_cast<int>(npc->GetPosY() / SECTION_SIZE);
	npc->SetSection(sectionX, sectionY);
	GameUtil::RegisterToSection(sectionY, sectionX, id);
	int n = rand() % 5;
	int level = (npc->GetPosX() / 100) * 5 + (npc->GetPosY() / 100) * 5 + n;
	npc->SetLevel(level + 5);
	npc->SetMaxHp(80 + npc->GetLevel() * 50);
	npc->SetCurHp(npc->GetMaxHp());
	npc->SetPower(10 + npc->GetLevel() * 5);
	
	if(id <= (MAX_NPC / 2) + MAX_USER)
		npc->SetMonType(MONSTER_TYPE::AGRO);
	else
		npc->SetMonType(MONSTER_TYPE::PEACE);

	return 1;
}

int API_get_x(lua_State* L)
{
	int id = (int)lua_tointeger(L, -1);
	lua_pop(L, 2);
	int x = CNetworkMgr::GetInstance()->GetCObject(id)->GetPosX();
	lua_pushnumber(L, x);
	return 1;
}

int API_get_y(lua_State* L)
{
	int id = (int)lua_tointeger(L, -1);
	lua_pop(L, 2);
	int y = CNetworkMgr::GetInstance()->GetCObject(id)->GetPosY();
	lua_pushnumber(L, y);
	return 1;
}

int API_SendMessage(lua_State* L)
{
	int my_id = (int)lua_tointeger(L, -3);
	int user_id = (int)lua_tointeger(L, -2);
	char* mess = (char*)lua_tostring(L, -1);
	lua_pop(L, 4);
	CNetworkMgr::GetInstance()->GetCObject(user_id)->Send_Chat_Packet(my_id, mess);
	return 0;
}