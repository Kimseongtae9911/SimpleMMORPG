#include "pch.h"
#include "CSession.h"
#include "CStat.h"

CSession::CSession()
{
	m_socket = 0;
	m_remainBufSize = 0;
}

CSession::~CSession()
{
}

void CSession::Initialize(const SOCKET& sock)
{
	m_remainBufSize = 0;
	m_socket = sock;
	RecvPacket();
}

void CSession::RecvPacket()
{
	DWORD recvFlag = 0;
	memset(&m_recvOver.m_over, 0, sizeof(m_recvOver.m_over));
	m_recvOver.m_Wsabuf.len = BUF_SIZE - m_remainBufSize;
	m_recvOver.m_Wsabuf.buf = m_recvOver.m_send_buf + m_remainBufSize;
	WSARecv(m_socket, &m_recvOver.m_Wsabuf, 1, 0, &recvFlag, &m_recvOver.m_over, 0);
}

void CSession::SendPacket(void* packet)
{
	COverlapEx* sdata = new COverlapEx{ reinterpret_cast<char*>(packet) };
	WSASend(m_socket, &sdata->m_Wsabuf, 1, 0, 0, &sdata->m_over, 0);
}

void CSession::SendLoginInfoPacket(int id, int posX, int posY, const char* name, const CClientStat* stat)
{
	SC_LOGIN_INFO_PACKET p;
	p.id = id;
	p.size = sizeof(SC_LOGIN_INFO_PACKET);
	p.type = SC_LOGIN_INFO;
	p.x = posX;
	p.y = posY;

	p.max_hp = stat->GetMaxHp();
	p.hp = stat->GetCurHp();
	p.max_mp = stat->GetMaxMp();
	p.mp = stat->GetCurMp();
	p.exp = stat->GetExp();
	p.level = stat->GetLevel();
	strcpy_s(p.name, name);
	SendPacket(&p);
}

void CSession::SendStatChangePacket(const CClientStat* stat)
{
	SC_STAT_CHANGE_PACKET packet;

	packet.size = sizeof(packet);
	packet.type = SC_STAT_CHANGE;

	packet.max_hp = stat->GetMaxHp();
	packet.hp = stat->GetCurHp();
	packet.max_mp = stat->GetMaxMp();
	packet.mp = stat->GetCurMp();
	packet.exp = stat->GetExp();
	packet.level = stat->GetLevel();

	SendPacket(&packet);
}

void CSession::SendDamagePacket(int cid, int powerlv, int curHp)
{
	SC_DAMAGE_PACKET p;

	p.size = sizeof(p);
	p.type = SC_DAMAGE;
	p.id = cid;
	p.hp = curHp;

	SendPacket(&p);
}

void CSession::SendItemUsePacket(int inven)
{
	SC_ITEM_USED_PACKET p;

	p.size = sizeof(p);
	p.type = SC_ITEM_USED;
	p.inven_num = inven;

	SendPacket(&p);
}

void CSession::SendLoginFailPacket()
{
	SC_LOGIN_FAIL_PACKET p;
	p.size = sizeof(SC_LOGIN_FAIL_PACKET);
	p.type = SC_LOGIN_FAIL;
	SendPacket(&p);
}

void CSession::SendMovePacket(int c_id, int posX, int posY, unsigned int lastMoveTime)
{
	SC_MOVE_OBJECT_PACKET p;
	p.id = c_id;
	p.size = sizeof(SC_MOVE_OBJECT_PACKET);
	p.type = SC_MOVE_OBJECT;
	p.x = posX;
	p.y = posY;
	p.move_time = lastMoveTime;
	SendPacket(&p);
}

void CSession::SendChatPacket(int c_id, const char* mess)
{
	SC_CHAT_PACKET packet;
	packet.id = c_id;
	packet.size = sizeof(packet);
	packet.type = SC_CHAT;
	strcpy_s(packet.mess, mess);
	SendPacket(&packet);
}