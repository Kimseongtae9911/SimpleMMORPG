#include "pch.h"
#include "CPacketMgr.h"
#include "CNetworkMgr.h"
#include "CDatabase.h"
#include "GameUtil.h"
#include "CClient.h"
#include "CNpc.h"

std::unique_ptr<CPacketMgr> CPacketMgr::m_instance;

bool CPacketMgr::Initialize()
{
	try {
		m_packetfunc.insert({ CS_LOGIN, [this](BASE_PACKET* p, CClient* c) {LoginPacket(p, c); } });
		m_packetfunc.insert({ CS_MOVE ,[this](BASE_PACKET* p, CClient* c) {MovePacket(p, c); } });
		m_packetfunc.insert({ CS_CHAT ,[this](BASE_PACKET* p, CClient* c) {ChatPacket(p, c); } });
		m_packetfunc.insert({ CS_TELEPORT, [this](BASE_PACKET* p, CClient* c) {TeleportPacket(p, c); } });
		m_packetfunc.insert({ CS_LOGOUT, [this](BASE_PACKET* p, CClient* c) {LogoutPacket(p, c); } });
		m_packetfunc.insert({ CS_ATTACK, [this](BASE_PACKET* p, CClient* c) {AttackPacket(p, c); } });
		m_packetfunc.insert({ CS_USE_ITEM, [this](BASE_PACKET* p, CClient* c) {UseItemPacket(p, c); } });

		return true;
	}
	catch (std::exception ex) {
		cout << ex.what() << endl;
		return false;
	}
}

bool CPacketMgr::Release()
{
    return false;
}

void CPacketMgr::PacketProcess(BASE_PACKET* packet, CClient* client)
{
	if (true == m_packetfunc.contains(packet->type)) {
		m_packetfunc[packet->type](packet, client);
	}
	else {
		cout << static_cast<int>(packet->type) << ": Undefined Packet Type" << endl;
	}
}

void CPacketMgr::LoginPacket(BASE_PACKET* packet, CClient* client)
{
	CS_LOGIN_PACKET* p = reinterpret_cast<CS_LOGIN_PACKET*>(packet);

	if (strcmp(p->name, "Stress_Test")) {
#ifdef DATABASE
		USER_INFO* user_info = reinterpret_cast<USER_INFO*>(CNetworkMgr::GetInstance()->GetDatabase()->GetPlayerInfo(p->name));
#else
		USER_INFO* user_info = new USER_INFO{ 100, 100, 50, 50, 1, 0, 30, 30, "KST", -1, -1, -1, -1, -1, -1, 0 };
#endif
		if (user_info->max_hp == -1) {
			CNetworkMgr::GetInstance()->GetDatabase()->MakeNewInfo(p->name);
		}
		else {
			if (CheckLoginFail(p->name))
			{
				//disconnect(c_id);
			}
			else {
				client->SetName(p->name);
				CClientStat* stat = reinterpret_cast<CClientStat*>(client->GetStat()); 
				stat->SetExp(user_info->exp);
				{
					lock_guard<mutex> ll{ client->m_StateLock };
					client->SetPos(user_info->pos_x, user_info->pos_y);
					stat->SetLevel(user_info->level);
					stat->SetMaxHp(user_info->max_hp);
					stat->SetCurHp(user_info->cur_hp);
					stat->SetMaxExp(INIT_EXP + (user_info->level - 1) * EXP_UP);
					stat->SetMaxMp(user_info->max_mp);
					stat->SetMp(user_info->cur_mp);
					client->SetState(CL_STATE::ST_INGAME);
				}

				client->GetSession()->SendLoginInfoPacket(client->GetID(), user_info->pos_x, user_info->pos_y, p->name, stat);
				for (auto& pl : CNetworkMgr::GetInstance()->GetAllObject()) {
					if (pl->GetID() == client->GetID() || pl->GetID() == -1)
						continue;
					if (false == client->CanSee(pl->GetID()))
						continue;
					if (pl->GetID() < MAX_USER) {
						CClient* cl = reinterpret_cast<CClient*>(pl);
						{
							lock_guard<mutex> ll(pl->m_StateLock);
							if (CL_STATE::ST_INGAME != cl->GetState())
								continue;
						}
						cl->AddObjectToView(client->GetID());
					}
					else
						reinterpret_cast<CNpc*>(pl)->WakeUp(client->GetID());
					client->AddObjectToView(pl->GetID());
				}
				if (user_info->item1 != -1) {
					client->SetItem(0, static_cast<ITEM_TYPE>(user_info->item1 + 1), user_info->moneycnt, true);
					SC_ITEM_GET_PACKET p;
					p.size = sizeof(p);
					p.type = SC_ITEM_GET;
					p.inven_num = 0;
					p.item_type = static_cast<ITEM_TYPE>(user_info->item1 + 1);
					p.x = client->GetPosX();
					p.y = client->GetPosY();
					client->GetSession()->SendPacket(&p);
				}
				if (user_info->item2 != -1) {
					client->SetItem(1, static_cast<ITEM_TYPE>(user_info->item2 + 1), 0, true);
					SC_ITEM_GET_PACKET p;
					p.size = sizeof(p);
					p.type = SC_ITEM_GET;
					p.inven_num = 1;
					p.item_type = static_cast<ITEM_TYPE>(user_info->item2 + 1);
					p.x = client->GetPosX();
					p.y = client->GetPosY();
					client->GetSession()->SendPacket(&p);
				}
				if (user_info->item3 != -1) {
					client->SetItem(2, static_cast<ITEM_TYPE>(user_info->item3 + 1), 0, true);
					SC_ITEM_GET_PACKET p;
					p.size = sizeof(p);
					p.type = SC_ITEM_GET;
					p.inven_num = 2;
					p.item_type = static_cast<ITEM_TYPE>(user_info->item3 + 1);
					p.x = client->GetPosX();
					p.y = client->GetPosY();
					client->GetSession()->SendPacket(&p);
				}
				if (user_info->item4 != -1) {
					client->SetItem(3, static_cast<ITEM_TYPE>(user_info->item4 + 1), 0, true);
					SC_ITEM_GET_PACKET p;
					p.size = sizeof(p);
					p.type = SC_ITEM_GET;
					p.inven_num = 3;
					p.item_type = static_cast<ITEM_TYPE>(user_info->item4 + 1);
					p.x = client->GetPosX();
					p.y = client->GetPosY();
					client->GetSession()->SendPacket(&p);
				}
				if (user_info->item5 != -1) {
					client->SetItem(4, static_cast<ITEM_TYPE>(user_info->item5 + 1), 0, true);
					SC_ITEM_GET_PACKET p;
					p.size = sizeof(p);
					p.type = SC_ITEM_GET;
					p.inven_num = 4;
					p.item_type = static_cast<ITEM_TYPE>(user_info->item5 + 1);
					p.x = client->GetPosX();
					p.y = client->GetPosY();
					client->GetSession()->SendPacket(&p);
				}
				if (user_info->item6 != -1) {
					client->SetItem(5, static_cast<ITEM_TYPE>(user_info->item6 + 1), 0, true);
					SC_ITEM_GET_PACKET p;
					p.size = sizeof(p);
					p.type = SC_ITEM_GET;
					p.inven_num = 5;
					p.item_type = static_cast<ITEM_TYPE>(user_info->item6 + 1);
					p.x = client->GetPosX();
					p.y = client->GetPosY();
					client->GetSession()->SendPacket(&p);
				}
				int sectionX = client->GetPosX() / SECTION_SIZE;
				int sectionY = client->GetPosY() / SECTION_SIZE;
				client->SetSection(sectionX, sectionY);
				GameUtil::RegisterToSection(-1, -1, sectionY, sectionX, client->GetID());
			}
		}
	}
	else {
		char name[NAME_SIZE];
		sprintf_s(name, NAME_SIZE, "%s%d", p->name, client->GetID());
		client->SetName(name);
		client->SetPos(rand() % W_WIDTH, rand() % W_HEIGHT);
		CClientStat* stat = reinterpret_cast<CClientStat*>(client->GetStat());
		stat->SetMaxHp(1000000);
		stat->SetCurHp(1000000);
		stat->SetMaxExp(100);
		stat->SetMaxMp(100);
		stat->SetMp(100);
		{
			lock_guard<mutex> ll{ client->m_StateLock };
			client->SetState(CL_STATE::ST_INGAME);
		}
		client->GetSession()->SendLoginInfoPacket(client->GetID(), client->GetPosX(), client->GetPosY(), p->name, stat);
		for (auto& pl : CNetworkMgr::GetInstance()->GetAllObject()) {
			CClient* cl = reinterpret_cast<CClient*>(pl);
			{
				lock_guard<mutex> ll(pl->m_StateLock);
				if (CL_STATE::ST_INGAME != cl->GetState())
					continue;
			}
			if (pl->GetID() == client->GetID())
				continue;
			if (false == client->CanSee(pl->GetID()))
				continue;
			if (pl->GetID() < MAX_USER)
				cl->AddObjectToView(client->GetID());
			else
				reinterpret_cast<CNpc*>(pl)->WakeUp(client->GetID());
			client->AddObjectToView(pl->GetID());
		}
	}
}

void CPacketMgr::MovePacket(BASE_PACKET* packet, CClient* client)
{
	CS_MOVE_PACKET* p = reinterpret_cast<CS_MOVE_PACKET*>(packet);
	client->lastMoveTime = p->move_time;

	client->Move(p->direction);
}

void CPacketMgr::ChatPacket(BASE_PACKET* packet, CClient* client)
{
}

void CPacketMgr::TeleportPacket(BASE_PACKET* packet, CClient* client)
{
}

void CPacketMgr::LogoutPacket(BASE_PACKET* packet, CClient* client)
{
}

void CPacketMgr::AttackPacket(BASE_PACKET* packet, CClient* client)
{
	CS_ATTACK_PACKET* p = reinterpret_cast<CS_ATTACK_PACKET*>(packet);
	client->UseSkill(static_cast<int>(p->skill));
	client->GetSession()->SendStatChangePacket(reinterpret_cast<CClientStat*>(client->GetStat()));
}

void CPacketMgr::UseItemPacket(BASE_PACKET* packet, CClient* client)
{
	CS_USE_ITEM_PACKET* p = reinterpret_cast<CS_USE_ITEM_PACKET*>(packet);
	client->UseItem(p->inven);
}

bool CPacketMgr::CheckLoginFail(char* name)
{
	if (!strcmp("Stress_Test", name))
		return false;
	for (int i = 0; i < MAX_USER; ++i) {
		CClient* client = reinterpret_cast<CClient*>(CNetworkMgr::GetInstance()->GetCObject(i));
		client->m_StateLock.lock();
		if (client->GetState() == CL_STATE::ST_INGAME) {
			client->m_StateLock.unlock();
			if (!strcmp(client->GetName(), name)) {
				client->GetSession()->SendLoginFailPacket();
				return true;
			}
		}
		else {
			client->m_StateLock.unlock();
		}
	}

	return false;
}
