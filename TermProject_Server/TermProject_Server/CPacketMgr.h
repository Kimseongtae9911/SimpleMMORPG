#pragma once

class CClient;

class CPacketMgr
{
	SINGLETON(CPacketMgr);
public:
	bool Initialize();
	bool Release();

	void PacketProcess(BASE_PACKET* packet, CClient* client);

private:
	void LoginPacket(BASE_PACKET* packet, CClient* client);
	void MovePacket(BASE_PACKET* packet, CClient* client);
	void ChatPacket(BASE_PACKET* packet, CClient* client);
	void TeleportPacket(BASE_PACKET* packet, CClient* client);
	void LogoutPacket(BASE_PACKET* packet, CClient* client);
	void AttackPacket(BASE_PACKET* packet, CClient* client);
	void UseItemPacket(BASE_PACKET* packet, CClient* client);

	bool CheckLoginFail(char* name);

private:
	std::unordered_map<char, std::function<void(BASE_PACKET*, CClient*)>> m_packetfunc;
};

