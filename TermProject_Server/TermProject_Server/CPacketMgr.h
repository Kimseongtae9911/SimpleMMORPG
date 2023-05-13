#pragma once

class CPlayer;

class CPacketMgr
{
	SINGLETON(CPacketMgr);
public:
	bool Initialize();
	bool Release();

	void PacketProcess(BASE_PACKET* packet, CPlayer* client);

private:
	void LoginPacket(BASE_PACKET* packet, CPlayer* client);
	void MovePacket(BASE_PACKET* packet, CPlayer* client);
	void ChatPacket(BASE_PACKET* packet, CPlayer* client);
	void TeleportPacket(BASE_PACKET* packet, CPlayer* client);
	void LogoutPacket(BASE_PACKET* packet, CPlayer* client);
	void AttackPacket(BASE_PACKET* packet, CPlayer* client);
	void UseItemPacket(BASE_PACKET* packet, CPlayer* client);

	bool CheckLoginFail(char* name);

private:
	std::unordered_map<char, std::function<void(BASE_PACKET*, CPlayer*)>> m_packetfunc;
};

