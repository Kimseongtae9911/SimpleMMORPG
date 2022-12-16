#pragma once

class CScene;

class CPacketMgr
{
public:
	CPacketMgr(const char* ip, shared_ptr<CScene> scene);
	~CPacketMgr();

	void SendPacket(void* packet);
	void RecvPacket();

private:
	void PacketReasemble(char* net_buf, size_t io_byte);
	void ProcessPacket(char* ptr);

private:
	sf::TcpSocket m_sock;
	shared_ptr<CScene> m_scene;
};

