#include "pch.h"
#include "ChatUtil.h"
#include "CNetworkMgr.h"
#include "CClient.h"

void ChatUtil::SendDamageMsg(int targetID, int power, char* name)
{
	char msg[CHAT_SIZE];

	CClient* targetClient = reinterpret_cast<CClient*>(CNetworkMgr::GetInstance()->GetCObject(targetID));
	for (int i = 0; i < MAX_USER; ++i) {
		CClient* otherClient = reinterpret_cast<CClient*>(CNetworkMgr::GetInstance()->GetCObject(i));
		if (otherClient->GetState() != CL_STATE::ST_INGAME)
			continue;
		if (i == targetClient->GetID()) {
			sprintf_s(msg, CHAT_SIZE, "%dDamage from %s", power, name);
		}
		else {
			sprintf_s(msg, CHAT_SIZE, "%s has been damaged %d by %s", targetClient->GetName(), power, name);
		}
		otherClient->GetSession()->SendChatPacket(i, msg);
	}
}

void ChatUtil::SendNpcDamageMsg(int npcID, int clientID)
{
	vector<int> chatClients = CNetworkMgr::GetInstance()->GetClientsCanSeeNpc(npcID);

	CObject* npc = CNetworkMgr::GetInstance()->GetCObject(npcID);
	CObject* client = CNetworkMgr::GetInstance()->GetCObject(clientID);

	for (const auto id : chatClients) {
		CClient* otherClient = reinterpret_cast<CClient*>(CNetworkMgr::GetInstance()->GetCObject(id));
		otherClient->GetSession()->SendDamagePacket(npcID, 3, npc->GetStat()->GetCurHp());
		char msg[CHAT_SIZE];
		if (id == clientID) {
			sprintf_s(msg, CHAT_SIZE, "Damaged %d at %s", client->GetStat()->GetPower() * 2, npc->GetName());
		}
		else {
			sprintf_s(msg, CHAT_SIZE, "%s Damaged %d at %s", client->GetName(), client->GetStat()->GetPower() * 2, npc->GetName());
		}
		otherClient->GetSession()->SendChatPacket(id, msg);
	}
}

void ChatUtil::SendNpcKillMsg(int npcID, int exp, int clientID)
{
	vector<int> chatClients = CNetworkMgr::GetInstance()->GetClientsCanSeeNpc(npcID);
	
	CObject* npc = CNetworkMgr::GetInstance()->GetCObject(npcID);
	CObject* client = CNetworkMgr::GetInstance()->GetCObject(clientID);

	for (const auto id : chatClients) {
		CClient* otherClient = reinterpret_cast<CClient*>(CNetworkMgr::GetInstance()->GetCObject(id));
		otherClient->RemoveObjectFromView(npcID);
		char msg[CHAT_SIZE];
		if (id == clientID) {
			sprintf_s(msg, CHAT_SIZE, "Obtained %dExp by killing %s", exp, npc->GetName());
		}
		else {
			sprintf_s(msg, CHAT_SIZE, "%s Killed %s", client->GetName(), npc->GetName());
		}
		otherClient->GetSession()->SendChatPacket(id, msg);
	}
}
