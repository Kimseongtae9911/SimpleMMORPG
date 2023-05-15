#include "pch.h"
#include "CPacketMgr.h"
#include "CNetworkMgr.h"
#include "CDatabase.h"
#include "GameUtil.h"
#include "CObject.h"

std::unique_ptr<CPacketMgr> CPacketMgr::m_instance;

bool CPacketMgr::Initialize()
{
	try {
		m_packetfunc.insert({ CS_LOGIN, [this](BASE_PACKET* p, CPlayer* c) {LoginPacket(p, c); } });
		m_packetfunc.insert({ CS_MOVE ,[this](BASE_PACKET* p, CPlayer* c) {MovePacket(p, c); } });
		m_packetfunc.insert({ CS_CHAT ,[this](BASE_PACKET* p, CPlayer* c) {ChatPacket(p, c); } });
		m_packetfunc.insert({ CS_TELEPORT, [this](BASE_PACKET* p, CPlayer* c) {TeleportPacket(p, c); } });
		m_packetfunc.insert({ CS_LOGOUT, [this](BASE_PACKET* p, CPlayer* c) {LogoutPacket(p, c); } });
		m_packetfunc.insert({ CS_ATTACK, [this](BASE_PACKET* p, CPlayer* c) {AttackPacket(p, c); } });
		m_packetfunc.insert({ CS_USE_ITEM, [this](BASE_PACKET* p, CPlayer* c) {UseItemPacket(p, c); } });

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

void CPacketMgr::PacketProcess(BASE_PACKET* packet, CPlayer* client)
{
	if (true == m_packetfunc.contains(packet->type)) {
		m_packetfunc[packet->type](packet, client);
	}
	else {
		cout << static_cast<int>(packet->type) << ": Undefined Packet Type" << endl;
	}
}

void CPacketMgr::LoginPacket(BASE_PACKET* packet, CPlayer* client)
{
	CS_LOGIN_PACKET* p = reinterpret_cast<CS_LOGIN_PACKET*>(packet);

	if (strcmp(p->name, "Stress_Test")) {
		USER_INFO* user_info = reinterpret_cast<USER_INFO*>(CNetworkMgr::GetInstance()->GetDatabase()->GetPlayerInfo(p->name));

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
				client->SetExp(user_info->exp);
				{
					lock_guard<mutex> ll{ client->m_StateLock };
					client->SetPos(user_info->pos_x, user_info->pos_y);
					client->SetLevel(user_info->level);
					client->SetMaxHp(user_info->max_hp);
					client->SetCurHp(user_info->cur_hp);
					client->SetMaxExp(INIT_EXP + (user_info->level - 1) * EXP_UP);
					client->SetMaxMp(user_info->max_mp);
					client->SetMp(user_info->cur_mp);
					client->SetState(CL_STATE::ST_INGAME);
				}

				client->Send_LoginInfo_Packet();
				for (auto& pl : CNetworkMgr::GetInstance()->GetAllObject()) {
					{
						lock_guard<mutex> ll(pl->m_StateLock);
						if (CL_STATE::ST_INGAME != pl->GetState())
							continue;
					}
					if (pl->GetID() == client->GetID())
						continue;
					if (false == client->CanSee(pl->GetID()))
						continue;
					if (pl->GetID() < MAX_USER)
						pl->Send_AddObject_Packet(client->GetID());
					else
						reinterpret_cast<CNpc*>(pl)->WakeUp(client->GetID());
					client->Send_AddObject_Packet(pl->GetID());
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
					client->SendPacket(&p);
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
					client->SendPacket(&p);
				}
				if (user_info->item3 != 3) {
					client->SetItem(2, static_cast<ITEM_TYPE>(user_info->item3 + 1), 0, true);
					SC_ITEM_GET_PACKET p;
					p.size = sizeof(p);
					p.type = SC_ITEM_GET;
					p.inven_num = 2;
					p.item_type = static_cast<ITEM_TYPE>(user_info->item3 + 1);
					p.x = client->GetPosX();
					p.y = client->GetPosY();
					client->SendPacket(&p);
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
					client->SendPacket(&p);
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
					client->SendPacket(&p);
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
					client->SendPacket(&p);
				}
			}
		}
	}
	else {
		client->SetName(p->name);
		client->SetPos(rand() % W_WIDTH, rand() % W_HEIGHT);
		client->SetMaxHp(1000000);
		client->SetCurHp(1000000);
		client->SetMaxExp(100);
		client->SetMaxMp(100);
		client->SetMp(100);
		{
			lock_guard<mutex> ll{ client->m_StateLock };
			client->SetState(CL_STATE::ST_INGAME);
		}
		client->Send_LoginInfo_Packet();
		for (auto& pl : CNetworkMgr::GetInstance()->GetAllObject()) {
			{
				lock_guard<mutex> ll(pl->m_StateLock);
				if (CL_STATE::ST_INGAME != pl->GetState())
					continue;
			}
			if (pl->GetID() == client->GetID())
				continue;
			if (false == client->CanSee(pl->GetID()))
				continue;
			if (pl->GetID() < MAX_USER)
				pl->Send_AddObject_Packet(client->GetID());
			else
				reinterpret_cast<CNpc*>(pl)->WakeUp(client->GetID());
			client->Send_AddObject_Packet(pl->GetID());
		}
	}
}

void CPacketMgr::MovePacket(BASE_PACKET* packet, CPlayer* client)
{
	CS_MOVE_PACKET* p = reinterpret_cast<CS_MOVE_PACKET*>(packet);
	client->last_move_time = p->move_time;
	short x = client->GetPosX();
	short y = client->GetPosY();
	if (GameUtil::CanMove(x, y, p->direction)) {
		switch (p->direction) {
		case 0: {
			client->SetDir(DIR::UP);
			y--; break;
		}
		case 1: {
			client->SetDir(DIR::DOWN);
			y++; break;
		}
		case 2: {
			client->SetDir(DIR::LEFT);
			x--; break;
		}
		case 3: {
			client->SetDir(DIR::RIGHT);
			x++; break;
		}
		}
		client->SetPos(x, y);
		client->CheckItem();

		unordered_set<int> near_list;
		client->m_ViewLock.lock_shared();
		unordered_set<int> old_vlist = client->GetViewList();
		client->m_ViewLock.unlock_shared();
		for (auto& cl : CNetworkMgr::GetInstance()->GetAllObject()) {
			if (cl->GetState() != CL_STATE::ST_INGAME)
				continue;
			if (cl->GetID() == client->GetID())
				continue;
			if (client->CanSee(cl->GetID()))
				near_list.insert(cl->GetID());
		}

		client->Send_Move_Packet(client->GetID());

		for (auto& pl : near_list) {
			CObject* cpl = CNetworkMgr::GetInstance()->GetCObject(pl);
			if (pl < MAX_USER) {
				cpl->m_ViewLock.lock_shared();
				if (cpl->GetViewList().count(client->GetID())) {
					cpl->m_ViewLock.unlock_shared();
					cpl->Send_Move_Packet(client->GetID());
				}
				else {
					cpl->m_ViewLock.unlock_shared();
					cpl->Send_AddObject_Packet(client->GetID());
				}
			}
			else {
				reinterpret_cast<CNpc*>(cpl)->WakeUp(client->GetID());
			}

			if (old_vlist.count(pl) == 0)
				client->Send_AddObject_Packet(pl);
		}

		for (auto& pl : old_vlist) {
			if (0 == near_list.count(pl)) {
				client->Send_RemoveObject_Packet(pl);
				if (pl < MAX_USER)
					CNetworkMgr::GetInstance()->GetCObject(pl)->Send_RemoveObject_Packet(client->GetID());
			}
		}
	}
}

void CPacketMgr::ChatPacket(BASE_PACKET* packet, CPlayer* client)
{
}

void CPacketMgr::TeleportPacket(BASE_PACKET* packet, CPlayer* client)
{
}

void CPacketMgr::LogoutPacket(BASE_PACKET* packet, CPlayer* client)
{
}

void CPacketMgr::AttackPacket(BASE_PACKET* packet, CPlayer* client)
{
	CS_ATTACK_PACKET* p = reinterpret_cast<CS_ATTACK_PACKET*>(packet);
	if (client->CanUse(p->skill, p->time)) {
		switch (p->skill) {
		case 0:
			client->Attack();
			break;
		case 1:
			if (false == client->GetPowerUp()) {
				client->Skill1();
				client->SetMp(client->GetMp() - 15);
				client->SetPower(client->GetPower() * 2);
				client->Send_StatChange_Packet();
			}
			break;
		case 2:
			client->Skill2();
			client->SetMp(client->GetMp() - 5);
			client->Send_StatChange_Packet();
			break;
		case 3:
			client->Skill3();
			client->SetMp(client->GetMp() - 10);
			client->Send_StatChange_Packet();
			break;
		}
	}
}

void CPacketMgr::UseItemPacket(BASE_PACKET* packet, CPlayer* client)
{
	CS_USE_ITEM_PACKET* p = reinterpret_cast<CS_USE_ITEM_PACKET*>(packet);
	client->UseItem(p->inven);
}

bool CPacketMgr::CheckLoginFail(char* name)
{
	if (!strcmp("Stress_Test", name))
		return false;
	for (int i = 0; i < MAX_USER; ++i) {
		CObject* client = CNetworkMgr::GetInstance()->GetCObject(i);
		client->m_StateLock.lock();
		if (client->GetState() == CL_STATE::ST_INGAME) {
			client->m_StateLock.unlock();
			if (!strcmp(client->GetName(), name)) {
				client->Send_LoginFail_Packet();
				return true;
			}
		}
		else {
			client->m_StateLock.unlock();
		}
	}

	return false;
}
