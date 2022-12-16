#include "pch.h"
#include "CObject.h"
#include "LuaFunc.h"

extern HANDLE h_iocp;
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
	p.x = m_PosX;
	p.y = m_PosY;
	p.move_time = last_move_time;
	SendPacket(&p);
}

void CObject::Send_AddObject_Packet(int c_id)
{
	SC_ADD_OBJECT_PACKET add_packet;
	add_packet.id = c_id;
	strcpy_s(add_packet.name, m_Name);
	add_packet.size = sizeof(add_packet);
	add_packet.type = SC_ADD_OBJECT;
	add_packet.x = m_PosX;
	add_packet.y = m_PosY;
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
	p.exp = m_exp;
	p.level = m_level;
	strcpy_s(p.name, m_Name);
	SendPacket(&p);
}