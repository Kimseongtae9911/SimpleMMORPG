#include "pch.h"
#include "CObject.h"
#include "LuaFunc.h"

extern HANDLE h_iocp;
extern array<CObject*, MAX_USER + MAX_NPC> clients;
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

CNpc::CNpc(int id)
{
	m_level = 1;
	m_curHp = 100;
	m_maxHp = 100;
	m_PosX = rand() % W_WIDTH;
	m_PosY = rand() % W_HEIGHT;
	m_ID = id;
	sprintf_s(m_Name, "NPC%d", id);
	m_State = CL_STATE::ST_INGAME;

	auto L = m_L = luaL_newstate();
	luaL_openlibs(L);

	luaL_loadfile(L, "npc.lua");
	lua_pcall(L, 0, 0, 0);

	lua_getglobal(L, "set_uid");
	lua_pushnumber(L, id);
	lua_pcall(L, 1, 0, 0);
	//lua_pop(L, 1);

	lua_register(L, "API_SendMessage", API_SendMessage);
	lua_register(L, "API_get_x", API_get_x);
	lua_register(L, "API_get_y", API_get_y);
}

CNpc::~CNpc()
{
}



CPlayer::CPlayer()
{
	m_usedTime = { };
	m_power = 20;
	m_poweruptime = {};
	m_powerup = false;
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
	p.hp = clients[cid]->GetCurHp() - m_power * powerlv;
	clients[cid]->SetCurHp(p.hp);

	SendPacket(&p);
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

			Send_Damage_Packet(cid, 1);

			if (clients[cid]->GetCurHp() <= 0.f) {
				dynamic_cast<CNpc*>(clients[cid])->m_active = false;
				Send_RemoveObject_Packet(cid);
				GainExp(clients[cid]->GetLevel() * clients[cid]->GetLevel() * 2);
				Send_StatChange_Packet();
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
			Send_Damage_Packet(cid, 3);

			if (clients[cid]->GetCurHp() <= 0) {
				dynamic_cast<CNpc*>(clients[cid])->m_active = false;
				Send_RemoveObject_Packet(cid);
				GainExp(clients[cid]->GetLevel() * clients[cid]->GetLevel() * 2);
				Send_StatChange_Packet();
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
			Send_Damage_Packet(cid, 2);

			if (clients[cid]->GetCurHp() <= 0) {
				dynamic_cast<CNpc*>(clients[cid])->m_active = false;
				Send_RemoveObject_Packet(cid);
				GainExp(clients[cid]->GetLevel() * clients[cid]->GetLevel() * 2);
				Send_StatChange_Packet();
			}
		}
	}
}

void CPlayer::GainExp(int exp)
{
	m_exp += exp;
	if (m_exp >= m_maxExp) {
		m_exp -= m_maxExp;
		m_level++;
		m_maxExp = INIT_EXP + (m_level-1) * EXP_UP;
		m_maxHp = m_level * 100;
		m_curHp = m_maxHp;
		m_maxMp = m_level * 50;
		m_curMp = m_maxMp;
	}
}
