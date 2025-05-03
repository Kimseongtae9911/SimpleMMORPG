#pragma once
#include "COverlapEx.h"

class CClientStat;

class CSession
{
public:
	CSession();
	~CSession();

	void Initialize(const SOCKET& sock);

	int GetRemainBuf() const { return m_remainBufSize; }
	const SOCKET& GetSocket() const { return m_socket; }
	void SetRemainBuf(const int size) { m_remainBufSize = size; }
	std::vector<char>& GetRecvPacketBuf() { return m_recvPacketBuf; }

	void RecvPacket();
	void SendPacket(void* packet);

	void SendLoginInfoPacket(int id, int posX, int posY, const char* name, const CClientStat* stat);
	void SendStatChangePacket(const CClientStat* stat);
	void SendDamagePacket(int cid, int powerlv, int curHp);
	void SendItemUsePacket(int inven);
	void SendLoginFailPacket();
	void SendMovePacket(int c_id, int posX, int posY, unsigned int lastMoveTime);
	void SendChatPacket(int c_id, const char* mess);

private:
	SOCKET m_socket;
	int	m_remainBufSize;
	COverlapEx m_recvOver;
	std::vector<char> m_recvPacketBuf;
};

