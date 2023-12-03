#pragma once
class ChatUtil
{
public:
	static void SendDamageMsg(int targetID, int power, char* name);
	static void SendNpcDamageMsg(int npcID, int clientID);
	static void SendNpcKillMsg(int npcID, int exp, int clientID);
};

