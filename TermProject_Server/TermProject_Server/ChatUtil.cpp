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
		otherClient->Send_Chat_Packet(i, msg);
	}
}
