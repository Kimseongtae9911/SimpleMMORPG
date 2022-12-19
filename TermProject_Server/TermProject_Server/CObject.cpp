#include "pch.h"
#include "CObject.h"
#include "LuaFunc.h"
#include "CItem.h"

extern HANDLE h_iocp;
extern array<CObject*, MAX_USER + MAX_NPC> clients;

ITEM_TYPE itemmap[W_HEIGHT][W_WIDTH] = { NONE, };

CObject::CObject()
{
	m_ID = -1;
	m_Socket = 0;
	m_PosX = m_PosY = 0;
	m_Name[0] = 0;
	m_State = CL_STATE::ST_FREE;
	m_RemainBuf_Size = 0;
}

CObject::~CObject()
{
}

void CObject::RecvPacket()
{
	DWORD recv_flag = 0;
	memset(&m_RecvOver.m_over, 0, sizeof(m_RecvOver.m_over));
	m_RecvOver.m_Wsabuf.len = BUF_SIZE - m_RemainBuf_Size;
	m_RecvOver.m_Wsabuf.buf = m_RecvOver.m_send_buf + m_RemainBuf_Size;
	WSARecv(m_Socket, &m_RecvOver.m_Wsabuf, 1, 0, &recv_flag, &m_RecvOver.m_over, 0);
}

void CObject::SendPacket(void* packet)
{
	OVER_EXP* sdata = new OVER_EXP{ reinterpret_cast<char*>(packet) };
	WSASend(m_Socket, &sdata->m_Wsabuf, 1, 0, 0, &sdata->m_over, 0);
}

void CObject::Send_LoginFail_Packet()
{
	SC_LOGIN_FAIL_PACKET p;
	p.size = sizeof(SC_LOGIN_FAIL_PACKET);
	p.type = SC_LOGIN_FAIL;
	SendPacket(&p);
}

void CObject::Send_Move_Packet(int c_id)
{
	SC_MOVE_OBJECT_PACKET p;
	p.id = c_id;
	p.size = sizeof(SC_MOVE_OBJECT_PACKET);
	p.type = SC_MOVE_OBJECT;
	p.x = clients[c_id]->GetPosX();
	p.y = clients[c_id]->GetPosY();
	p.move_time = last_move_time;
	SendPacket(&p);
}

void CObject::Send_AddObject_Packet(int c_id)
{
	SC_ADD_OBJECT_PACKET add_packet;
	add_packet.id = c_id;
	strcpy_s(add_packet.name, clients[c_id]->GetName());
	add_packet.size = sizeof(add_packet);
	add_packet.type = SC_ADD_OBJECT;
	add_packet.x = clients[c_id]->GetPosX();
	add_packet.y = clients[c_id]->GetPosY();

	if (c_id >= MAX_USER) {
		add_packet.level = clients[c_id]->GetLevel();
		add_packet.hp = clients[c_id]->GetCurHp();
		add_packet.max_hp = clients[c_id]->GetMaxHp();
		if (add_packet.hp <= 0) {
			return;
		}
	}
	else {
		add_packet.level = 0;
		add_packet.hp = 0;
		add_packet.max_hp = 0;
	}

	m_ViewLock.lock();
	m_view_list.insert(c_id);
	m_ViewLock.unlock();
	SendPacket(&add_packet);
}

void CObject::Send_Chat_Packet(int c_id, const char* mess)
{
	SC_CHAT_PACKET packet;
	packet.id = c_id;
	packet.size = sizeof(packet);
	packet.type = SC_CHAT;
	strcpy_s(packet.mess, mess);
	SendPacket(&packet);
}

void CObject::Send_RemoveObject_Packet(int c_id)
{
	m_ViewLock.lock();
	if (m_view_list.count(c_id))
		m_view_list.erase(c_id);
	else {
		m_ViewLock.unlock();
		return;
	}
	m_ViewLock.unlock();
	SC_REMOVE_OBJECT_PACKET p;
	p.id = c_id;
	p.size = sizeof(p);
	p.type = SC_REMOVE_OBJECT;
	SendPacket(&p);
}

const bool CObject::CanSee(int from, int to) const
{
	if (abs(clients[from]->GetPosX() - clients[to]->GetPosX()) > VIEW_RANGE)
		return false;

	return abs(clients[from]->GetPosY() - clients[to]->GetPosY()) <= VIEW_RANGE;
}

CNpc::CNpc(int id)
{
	m_ID = id;
	sprintf_s(m_Name, "NPC%d", id);
	m_State = CL_STATE::ST_INGAME;

	auto L = m_L = luaL_newstate();
	luaL_openlibs(L);

	luaL_loadfile(L, "npc.lua");
	lua_pcall(L, 0, 0, 0);

	lua_register(L, "API_Initialize", API_Initialize);
	lua_register(L, "API_SendMessage", API_SendMessage);
	lua_register(L, "API_get_x", API_get_x);
	lua_register(L, "API_get_y", API_get_y);
}

CNpc::~CNpc()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

CPlayer::CPlayer()
{
	m_usedTime = { };
	m_power = 20;
	m_poweruptime = {};
	m_powerup = false;
	for (int i = 0; i < m_items.size(); ++i) {
		m_items[i] = new CItem();
	}
}

CPlayer::~CPlayer()
{
}

void CPlayer::PlayerAccept(int id, SOCKET sock)
{
	m_PosX = 0;
	m_PosY = 0;
	m_ID = id;
	m_Name[0] = 0;
	m_RemainBuf_Size = 0;
	m_Socket = sock;
	CreateIoCompletionPort(reinterpret_cast<HANDLE>(m_Socket), h_iocp, id, 0);
	RecvPacket();
}

void CPlayer::CheckItem()
{
	if (ITEM_TYPE::NONE != itemmap[m_PosY][m_PosX]) {
		int index = -1;
		bool money = false;
		if (ITEM_TYPE::MONEY == itemmap[m_PosY][m_PosX]) {
			for (int i = 0; i < m_items.size(); ++i) {
				if (false == m_items[i]->GetEnable() && -1 == index) {
					index = i;
				}
				if (ITEM_TYPE::MONEY == m_items[i]->GetItemType()) {
					money = true;
					index = i;
					m_itemLock.lock();
					m_items[i]->SetNum(m_items[i]->GetNum() + 1);
					m_itemLock.unlock();
					break;
				}
			}
			if (false == money) {
				m_itemLock.lock();
				m_items[index]->SetEnable(true);
				m_items[index]->SetItemType(itemmap[m_PosY][m_PosX]);
				m_itemLock.unlock();
			}
		}
		else {
			for (int i = 0; i < m_items.size(); ++i) {
				if (false == m_items[i]->GetEnable()) {
					index = i;
					m_itemLock.lock();
					m_items[i]->SetEnable(true);
					m_items[i]->SetItemType(itemmap[m_PosY][m_PosX]);
					m_itemLock.unlock();
 					break;
				}
			}
		}

		if (-1 != index) {
			SC_ITEM_GET_PACKET p;
			p.size = sizeof(p);
			p.type = SC_ITEM_GET;
			p.inven_num = index;
			p.item_type = m_items[index]->GetItemType();
			p.x = m_PosX;
			p.y = m_PosY;
			SendPacket(&p);
		}
		itemmap[m_PosY][m_PosX] = ITEM_TYPE::NONE;
	}
}

void CPlayer::Attack()
{
	m_ViewLock.lock();
	auto v_list = m_view_list;
	m_ViewLock.unlock();

	for (const auto id : v_list) {
		if (id < MAX_USER)
			continue;
		int cid = -1;
		if (clients[id]->GetPosX() == m_PosX && clients[id]->GetPosY() + 1 == m_PosY) {
			//아래있는 몬스터
			cid = id;
		}
		else if (clients[id]->GetPosX() == m_PosX && clients[id]->GetPosY() - 1 == m_PosY) {
			//위에 있는 몬스터
			cid = id;
		}
		else if (clients[id]->GetPosY() == m_PosY && clients[id]->GetPosX() + 1 == m_PosX) {
			//오른족에 있는 몬스터
			cid = id;
		}
		else if (clients[id]->GetPosY() == m_PosY && clients[id]->GetPosX() - 1 == m_PosX) {
			//왼쪽에 있는 몬스터
			cid = id;
		}
		if (-1 != cid) {
			if (true == m_powerup) {
				if (chrono::duration_cast<chrono::seconds>(chrono::system_clock::now() - m_poweruptime).count() >= POWERUP_TIME) {
					m_powerup = false;
					m_power /= 2;
				}
			}

			vector<int> see_monster;
			for (auto& obj : clients) {
				if (CL_STATE::ST_INGAME != obj->GetState())
					continue;
				if (obj->GetID() >= MAX_USER)
					break;
				if (true == obj->CanSee(clients[id]->GetID(), obj->GetID()))
					see_monster.emplace_back(obj->GetID());
			}

			clients[cid]->SetCurHp(clients[cid]->GetCurHp() - m_power * 3);

			bool remove = false;
			if (clients[cid]->GetCurHp() <= 0.f) {
				remove = true;
				dynamic_cast<CNpc*>(clients[cid])->m_active = false;
				GainExp(clients[cid]->GetLevel() * clients[cid]->GetLevel() * 2);
				Send_StatChange_Packet();
				CreateItem(clients[cid]->GetPosX(), clients[cid]->GetPosY());
			}

			for (const auto id : see_monster) {
				dynamic_cast<CPlayer*>(clients[id])->Send_Damage_Packet(cid, 3);
				if (true == remove)
					clients[id]->Send_RemoveObject_Packet(cid);
			}
		}
	}	
}

void CPlayer::Skill1()
{
	m_poweruptime = chrono::system_clock::now();
	m_powerup = true;
}

void CPlayer::Skill2()
{
	m_ViewLock.lock();
	auto v_list = m_view_list;
	m_ViewLock.unlock();

	//Collision
	for (const auto id : v_list) {
		if (id < MAX_USER)
			continue;

		int cid = -1;
		for (int i = 1; i < 5; ++i) {
			switch (m_dir) {
			case DIR::DOWN:
				if (clients[id]->GetPosX() == m_PosX && clients[id]->GetPosY() - i == m_PosY) {
					cid = id;
				}
				break;
			case DIR::UP:
				if (clients[id]->GetPosX() == m_PosX && clients[id]->GetPosY() + i == m_PosY) {
					cid = id;
				}
				break;
			case DIR::LEFT:
				if (clients[id]->GetPosX() + i == m_PosX && clients[id]->GetPosY() == m_PosY) {
					cid = id;
				}
				break;
			case DIR::RIGHT:
				if (clients[id]->GetPosX() - i == m_PosX && clients[id]->GetPosY() == m_PosY) {
					cid = id;
				}
				break;
			}
		}
		if (-1 != cid) {
			if (true == m_powerup) {
				if (chrono::duration_cast<chrono::seconds>(chrono::system_clock::now() - m_poweruptime).count() >= POWERUP_TIME) {
					m_powerup = false;
					m_power /= 2;
				}
			}
			
			vector<int> see_monster;
			for (auto& obj : clients) {
				if (CL_STATE::ST_INGAME != obj->GetState())
					continue;
				if (obj->GetID() >= MAX_USER)
					break;
				if (true == obj->CanSee(clients[id]->GetID(), obj->GetID()))
					see_monster.emplace_back(obj->GetID());
			}

			clients[cid]->SetCurHp(clients[cid]->GetCurHp() - m_power * 3);

			bool remove = false;
			if (clients[cid]->GetCurHp() <= 0) {
				remove = true;
				dynamic_cast<CNpc*>(clients[cid])->m_active = false;
				GainExp(clients[cid]->GetLevel() * clients[cid]->GetLevel() * 2);
				Send_StatChange_Packet();
				CreateItem(clients[cid]->GetPosX(), clients[cid]->GetPosY());
			}

			for (const auto id : see_monster) {
				dynamic_cast<CPlayer*>(clients[id])->Send_Damage_Packet(cid, 3);
				if (true == remove)
					clients[id]->Send_RemoveObject_Packet(cid);
			}
		}
	}
}

void CPlayer::Skill3()
{
	m_ViewLock.lock();
	auto v_list = m_view_list;
	m_ViewLock.unlock();

	for (const auto id : v_list) {
		if (id < MAX_USER)
			continue;
		int cid = -1;
		for (int i = -2; i < 3; ++i) {
			for (int j = -2; j < 3; ++j) {
				if (clients[id]->GetPosX() + i == m_PosX && clients[id]->GetPosY() + j == m_PosY) {
					cid = id;
					break;
				}
			}
			if (cid != -1)
				break;
		}
		if (-1 != cid) {
			if (true == m_powerup) {
				if (chrono::duration_cast<chrono::seconds>(chrono::system_clock::now() - m_poweruptime).count() >= POWERUP_TIME) {
					m_powerup = false;
					m_power /= 2;
				}
			}
			vector<int> see_monster;
			for (auto& obj : clients) {
				if (CL_STATE::ST_INGAME != obj->GetState())
					continue;
				if (obj->GetID() >= MAX_USER)
					break;
				if (true == obj->CanSee(clients[id]->GetID(), obj->GetID()))
					see_monster.emplace_back(obj->GetID());
			}
			clients[cid]->SetCurHp(clients[cid]->GetCurHp() - m_power * 3);
			
			bool remove = false;
			if (clients[cid]->GetCurHp() <= 0) {
				remove = true;
				dynamic_cast<CNpc*>(clients[cid])->m_active = false;
				GainExp(clients[cid]->GetLevel() * clients[cid]->GetLevel() * 2);
				Send_StatChange_Packet();
				CreateItem(clients[cid]->GetPosX(), clients[cid]->GetPosY());
			}

			for (const auto id : see_monster) {
				dynamic_cast<CPlayer*>(clients[id])->Send_Damage_Packet(cid, 3);
				if (true == remove)
					clients[id]->Send_RemoveObject_Packet(cid);
			}
		}
	}
}

void CPlayer::GainExp(int exp)
{
	m_exp += exp;
	if (m_exp >= m_maxExp) {
		while (true) {
			m_exp -= m_maxExp;
			m_level++;
			m_maxExp = INIT_EXP + (m_level - 1) * EXP_UP;

			if (m_exp < m_maxExp)
				break;
		}
		m_maxHp = m_level * 100;
		m_curHp = m_maxHp;
		m_maxMp = m_level * 50;
		m_curMp = m_maxMp;
	}
}

random_device rd;
default_random_engine dre{ rd() };
void CPlayer::CreateItem(short x, short y)
{
	uniform_int_distribution<> uid;
	int item = uid(dre) % 100;	//Nothing:20%, Money:50%, Potion:12%/12%, Weapon:2%/2%/2%

	ITEM_TYPE item_type = ITEM_TYPE::NONE;

	if (item <= 49) { // Money
		item_type = ITEM_TYPE::MONEY;
	}
	else if (item >= 50 && item <= 67) { // Nothing

	}
	else if (item >= 68 && item <= 69) {
		item_type = ITEM_TYPE::HAT;
	}
	else if (item >= 70 && item <= 81) { // HP Potion
		item_type = ITEM_TYPE::HP_POTION;
	}
	else if (item >= 82 && item <= 93) { // MP Potion
		item_type = ITEM_TYPE::MP_POTION;
	}
	else if (item >= 94 && item <= 95) { // Wand
		item_type = ITEM_TYPE::WAND;
	}
	else if (item >= 96 && item <= 97) { // Cloth
		item_type = ITEM_TYPE::CLOTH;
	}
	else {	// ring
		item_type = ITEM_TYPE::RING;
	}

	if (ITEM_TYPE::NONE != item_type) {
		cout << "Generate Item" << endl;
		SC_ITEM_ADD_PACKET p;
		p.type = SC_ITEM_ADD;
		p.size = sizeof(p);
		p.x = x;
		p.y = y;
		p.item_type = static_cast<int>(item_type);
		itemmap[y][x] = item_type;
		SendPacket(&p);
	}
}

void CPlayer::UseItem(int inven)
{
	if (true == m_items[inven]->GetEnable()) {
		int use = false;
		switch (m_items[inven]->GetItemType()) {
		case ITEM_TYPE::HP_POTION:
			m_curHp -= 10;
			use = true;
			break;
		case ITEM_TYPE::MP_POTION:
			m_curMp -= 10;
			use = true;
			break;
		default:
			break;
		}

		if (true == use) {
			m_itemLock.lock();
			m_items[inven]->SetEnable(false);
			m_items[inven]->SetItemType(ITEM_TYPE::NONE);
			m_itemLock.unlock();
			Send_StatChange_Packet();
			Send_ItemUsed_Packet(inven);
		}
	}
}

void CPlayer::Send_LoginInfo_Packet()
{
	SC_LOGIN_INFO_PACKET p;
	p.id = m_ID;
	p.size = sizeof(SC_LOGIN_INFO_PACKET);
	p.type = SC_LOGIN_INFO;
	p.x = m_PosX;
	p.y = m_PosY;

	p.max_hp = m_maxHp;
	p.hp = m_curHp;
	p.max_mp = m_maxMp;
	p.mp = m_curMp;
	p.exp = m_exp;
	p.level = m_level;
	strcpy_s(p.name, m_Name);
	SendPacket(&p);
}

void CPlayer::Send_StatChange_Packet()
{
	SC_STAT_CHANGE_PACKET packet;

	packet.size = sizeof(packet);
	packet.type = SC_STAT_CHANGE;
	packet.max_hp = m_maxHp;
	packet.hp = m_curHp;
	packet.max_mp = m_maxMp;
	packet.mp = m_curMp;
	packet.level = m_level;
	packet.exp = m_exp;

	SendPacket(&packet);
}

void CPlayer::Send_Damage_Packet(int cid, int powerlv)
{
	SC_DAMAGE_PACKET p;

	p.size = sizeof(p);
	p.type = SC_DAMAGE;
	p.id = cid;
	p.hp = clients[cid]->GetCurHp();

	SendPacket(&p);
}

void CPlayer::Send_ItemUsed_Packet(int inven)
{
	SC_ITEM_USED_PACKET p;

	p.size = sizeof(p);
	p.type = SC_ITEM_USED;
	p.inven_num = inven;

	SendPacket(&p);
}