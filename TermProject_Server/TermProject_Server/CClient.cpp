#include "pch.h"
#include "CClient.h"
#include "CNpc.h"
#include "CItem.h"
#include "GameUtil.h"
#include "CNetworkMgr.h"

CClient::CClient()
{
	m_Socket = 0;
	m_State = CL_STATE::ST_FREE;
	m_RemainBuf_Size = 0;

	m_stat = make_unique<CClientStat>(20);
	m_usedTime = { };
	m_poweruptime = {};
	m_powerup = false;
	for (int i = 0; i < m_items.size(); ++i) {
		m_items[i] = new CItem();
	}
}

CClient::~CClient()
{
}

void CClient::PlayerAccept(int id, SOCKET sock)
{
	m_PosX = 0;
	m_PosY = 0;
	m_ID = id;
	m_name[0] = 0;
	m_RemainBuf_Size = 0;
	m_Socket = sock;
	RecvPacket();
}

void CClient::CheckItem()
{
	if (ITEM_TYPE::NONE != GameUtil::GetItemTile(m_PosY, m_PosX)) {
		int index = -1;
		bool money = false;
		if (ITEM_TYPE::MONEY == GameUtil::GetItemTile(m_PosY, m_PosX)) {
			for (int i = 0; i < m_items.size(); ++i) {
				if (false == m_items[i]->GetEnable() && -1 == index) {
					index = i;
				}
				if (ITEM_TYPE::MONEY == m_items[i]->GetItemType()) {
					money = true;
					index = i;
					m_itemLock.lock();
					m_items[i]->SetNum(m_items[i]->GetNum() + 1);
					m_itemLock.unlock();
					break;
				}
			}
			if (false == money) {
				m_itemLock.lock();
				m_items[index]->SetEnable(true);
				m_items[index]->SetItemType(GameUtil::GetItemTile(m_PosY, m_PosX));
				m_itemLock.unlock();
			}
		}
		else {
			for (int i = 0; i < m_items.size(); ++i) {
				if (false == m_items[i]->GetEnable()) {
					index = i;
					m_itemLock.lock();
					m_items[i]->SetEnable(true);
					m_items[i]->SetItemType(GameUtil::GetItemTile(m_PosY, m_PosX));
					m_itemLock.unlock();
					break;
				}
			}
		}

		if (-1 != index) {
			char msg[CHAT_SIZE];
			char item[11];
			SC_ITEM_GET_PACKET p;
			p.size = sizeof(p);
			p.type = SC_ITEM_GET;
			p.inven_num = index;
			p.item_type = m_items[index]->GetItemType();
			p.x = m_PosX;
			p.y = m_PosY;
			SendPacket(&p);

			switch (p.item_type) {
			case ITEM_TYPE::CLOTH:
				memcpy(item, "CLOTH", 10);
				break;
			case ITEM_TYPE::HAT:
				memcpy(item, "HAT", 10);
				break;
			case ITEM_TYPE::HP_POTION:
				memcpy(item, "HP_POTION", 10);
				break;
			case ITEM_TYPE::MONEY:
				memcpy(item, "MONEY", 10);
				break;
			case ITEM_TYPE::MP_POTION:
				memcpy(item, "MP_POTION", 10);
				break;
			case ITEM_TYPE::RING:
				memcpy(item, "RING", 10);
				break;
			case ITEM_TYPE::WAND:
				memcpy(item, "WAND", 10);
				break;
			}

			sprintf_s(msg, CHAT_SIZE, "%s Obtained", item);
			Send_Chat_Packet(m_ID, msg);
		}
		GameUtil::SetItemTile(m_PosY, m_PosX, ITEM_TYPE::NONE);
	}
}

void CClient::Attack()
{
	m_ViewLock.lock_shared();
	auto v_list = m_viewList;
	m_ViewLock.unlock_shared();

	for (const auto id : v_list) {
		CObject* client = CNetworkMgr::GetInstance()->GetCObject(id);
		if (id < MAX_USER)
			continue;
		int cid = -1;
		if (client->GetPosX() == m_PosX && client->GetPosY() + 1 == m_PosY) {
			//아래있는 몬스터
			cid = id;
		}
		else if (client->GetPosX() == m_PosX && client->GetPosY() - 1 == m_PosY) {
			//위에 있는 몬스터
			cid = id;
		}
		else if (client->GetPosY() == m_PosY && client->GetPosX() + 1 == m_PosX) {
			//오른족에 있는 몬스터
			cid = id;
		}
		else if (client->GetPosY() == m_PosY && client->GetPosX() - 1 == m_PosX) {
			//왼쪽에 있는 몬스터
			cid = id;
		}
		if (-1 != cid) {
			if (true == m_powerup) {
				if (chrono::duration_cast<chrono::seconds>(chrono::system_clock::now() - m_poweruptime).count() >= POWERUP_TIME) {
					m_powerup = false;
					m_stat->SetPower(m_stat->GetPower() / 2);
				}
			}
			CObject* client2 = CNetworkMgr::GetInstance()->GetCObject(cid);

			vector<int> see_monster;
			for (auto& obj : CNetworkMgr::GetInstance()->GetAllObject()) {
				if (CL_STATE::ST_INGAME != reinterpret_cast<CClient*>(obj)->GetState())
					continue;
				if (obj->GetID() >= MAX_USER)
					break;
				if (true == obj->CanSee(CNetworkMgr::GetInstance()->GetCObject(id)->GetID()))
					see_monster.emplace_back(obj->GetID());
			}

			client2->GetStat()->SetCurHp(client2->GetStat()->GetCurHp() - m_stat->GetPower() * 2);

			bool remove = false;
			int exp = 0;
			if (client2->GetStat()->GetCurHp() <= 0.f) {
				remove = true;
				reinterpret_cast<CNpc*>(client2)->SetDie(true);
				TIMER_EVENT ev = { cid, chrono::system_clock::now() + 30s, EV_MONSTER_RESPAWN, 0 };
				CNetworkMgr::GetInstance()->RegisterEvent(ev);
				exp = client2->GetStat()->GetLevel() * client2->GetStat()->GetLevel() * 2;
				if (reinterpret_cast<CNpc*>(client2)->GetMonType() == MONSTER_TYPE::AGRO)
					exp *= 2;
				reinterpret_cast<CClientStat*>(m_stat.get())->GainExp(exp);
				Send_StatChange_Packet();
				CreateItem(client2->GetPosX(), client2->GetPosY());
			}

			for (const auto id : see_monster) {
				CClient* client3 = reinterpret_cast<CClient*>(CNetworkMgr::GetInstance()->GetCObject(id));
				client3->Send_Damage_Packet(cid, 3);
				if (true == remove) {
					client3->Send_RemoveObject_Packet(cid);
					char msg[CHAT_SIZE];
					if (id == m_ID) {
						sprintf_s(msg, CHAT_SIZE, "Obtained %dExp by killing %s", exp, client2->GetName());
						client3->Send_Chat_Packet(id, msg);
					}
					else {
						sprintf_s(msg, CHAT_SIZE, "%s Killed %s", client3->GetName(), client2->GetName());
						client3->Send_Chat_Packet(id, msg);
					}
				}
				else {
					char msg[CHAT_SIZE];
					if (id == m_ID) {
						sprintf_s(msg, CHAT_SIZE, "Damaged %d at %s", client3->GetStat()->GetPower() * 2, client2->GetName());
						client3->Send_Chat_Packet(id, msg);
					}
					else {
						sprintf_s(msg, CHAT_SIZE, "%s Damaged %d at %s", client3->GetName(), client3->GetStat()->GetPower() * 2, client2->GetName());
						client3->Send_Chat_Packet(id, msg);
					}
				}
			}
		}
	}
}

void CClient::Skill1()
{
	m_poweruptime = chrono::system_clock::now();
	m_powerup = true;
}

void CClient::Skill2()
{
	m_ViewLock.lock_shared();
	auto v_list = m_viewList;
	m_ViewLock.unlock_shared();

	//Collision
	for (const auto id : v_list) {
		if (id < MAX_USER)
			continue;

		CObject* client = CNetworkMgr::GetInstance()->GetCObject(id);
		int cid = -1;
		for (int i = 1; i < 5; ++i) {
			switch (m_dir) {
			case DIR::DOWN:
				if (client->GetPosX() == m_PosX && client->GetPosY() - i == m_PosY) {
					cid = id;
				}
				break;
			case DIR::UP:
				if (client->GetPosX() == m_PosX && client->GetPosY() + i == m_PosY) {
					cid = id;
				}
				break;
			case DIR::LEFT:
				if (client->GetPosX() + i == m_PosX && client->GetPosY() == m_PosY) {
					cid = id;
				}
				break;
			case DIR::RIGHT:
				if (client->GetPosX() - i == m_PosX && client->GetPosY() == m_PosY) {
					cid = id;
				}
				break;
			}
		}
		if (-1 != cid) {
			if (true == m_powerup) {
				if (chrono::duration_cast<chrono::seconds>(chrono::system_clock::now() - m_poweruptime).count() >= POWERUP_TIME) {
					m_powerup = false;
					m_stat->SetPower(m_stat->GetPower() / 2);
				}
			}

			vector<int> see_monster;
			for (auto& obj : CNetworkMgr::GetInstance()->GetAllObject()) {
				if (CL_STATE::ST_INGAME != reinterpret_cast<CClient*>(obj)->GetState())
					continue;
				if (obj->GetID() >= MAX_USER)
					break;
				if (true == obj->CanSee(client->GetID()))
					see_monster.emplace_back(obj->GetID());
			}

			CObject* target = CNetworkMgr::GetInstance()->GetCObject(cid);

			target->GetStat()->SetCurHp(target->GetStat()->GetCurHp() - m_stat->GetPower() * 4);

			bool remove = false;
			int exp = 0;
			if (target->GetStat()->GetCurHp() <= 0) {
				remove = true;
				reinterpret_cast<CNpc*>(target)->SetDie(true);
				TIMER_EVENT ev = { cid, chrono::system_clock::now() + 30s, EV_MONSTER_RESPAWN, 0 };
				CNetworkMgr::GetInstance()->RegisterEvent(ev);
				exp = target->GetStat()->GetLevel() * target->GetStat()->GetLevel() * 2;
				if (reinterpret_cast<CNpc*>(target)->GetMonType() == MONSTER_TYPE::AGRO)
					exp *= 2;
				reinterpret_cast<CClientStat*>(m_stat.get())->GainExp(exp);
				Send_StatChange_Packet();
				CreateItem(target->GetPosX(), target->GetPosY());
			}

			for (const auto id : see_monster) {
				CClient* player = reinterpret_cast<CClient*>(CNetworkMgr::GetInstance()->GetCObject(id));
				player->Send_Damage_Packet(cid, 3);
				if (true == remove) {
					player->Send_RemoveObject_Packet(cid);
					char msg[CHAT_SIZE];
					if (id == m_ID) {
						sprintf_s(msg, CHAT_SIZE, "Obtained %dExp by killing %s", exp, target->GetName());
						player->Send_Chat_Packet(id, msg);
					}
					else {
						sprintf_s(msg, CHAT_SIZE, "%s Killed %s", player->GetName(), target->GetName());
						player->Send_Chat_Packet(id, msg);
					}
				}
				else {
					char msg[CHAT_SIZE];
					if (id == m_ID) {
						sprintf_s(msg, CHAT_SIZE, "Damaged %d at %s", player->GetStat()->GetPower() * 4, target->GetName());
						player->Send_Chat_Packet(id, msg);
					}
					else {
						sprintf_s(msg, CHAT_SIZE, "%s Damaged %d at %s", player->GetName(), player->GetStat()->GetPower() * 4, target->GetName());
						player->Send_Chat_Packet(id, msg);
					}
				}
			}
		}
	}
}

void CClient::Skill3()
{
	m_ViewLock.lock_shared();
	auto v_list = m_viewList;
	m_ViewLock.unlock_shared();

	for (const auto id : v_list) {
		CObject* client = CNetworkMgr::GetInstance()->GetCObject(id);
		if (id < MAX_USER)
			continue;
		int cid = -1;
		for (int i = -2; i < 3; ++i) {
			for (int j = -2; j < 3; ++j) {
				if (client->GetPosX() + i == m_PosX && client->GetPosY() + j == m_PosY) {
					cid = id;
					break;
				}
			}
			if (cid != -1)
				break;
		}
		if (-1 != cid) {
			if (true == m_powerup) {
				if (chrono::duration_cast<chrono::seconds>(chrono::system_clock::now() - m_poweruptime).count() >= POWERUP_TIME) {
					m_powerup = false;
					m_stat->SetPower(m_stat->GetPower() / 2);
				}
			}
			vector<int> see_monster;
			for (auto& obj : CNetworkMgr::GetInstance()->GetAllObject()) {
				if (CL_STATE::ST_INGAME != reinterpret_cast<CClient*>(obj)->GetState())
					continue;
				if (obj->GetID() >= MAX_USER)
					break;
				if (true == obj->CanSee(client->GetID()))
					see_monster.emplace_back(obj->GetID());
			}

			CNpc* target = reinterpret_cast<CNpc*>(CNetworkMgr::GetInstance()->GetCObject(cid));
			target->GetStat()->SetCurHp(target->GetStat()->GetCurHp() - m_stat->GetPower() * 3);

			bool remove = false;
			int exp = 0;
			if (target->GetStat()->GetCurHp() <= 0) {
				remove = true;
				target->SetDie(true);
				TIMER_EVENT ev = { cid, chrono::system_clock::now() + 30s, EV_MONSTER_RESPAWN, 0 };
				CNetworkMgr::GetInstance()->RegisterEvent(ev);
				exp = target->GetStat()->GetLevel() * target->GetStat()->GetLevel() * 2;
				if (target->GetMonType() == MONSTER_TYPE::AGRO)
					exp *= 2;
				reinterpret_cast<CClientStat*>(m_stat.get())->GainExp(exp);
				Send_StatChange_Packet();
				CreateItem(target->GetPosX(), target->GetPosY());
			}

			for (const auto id : see_monster) {
				CClient* player = reinterpret_cast<CClient*>(CNetworkMgr::GetInstance()->GetCObject(id));
				player->Send_Damage_Packet(cid, 3);
				if (true == remove) {
					player->Send_RemoveObject_Packet(cid);
					char msg[CHAT_SIZE];
					if (id == m_ID) {
						sprintf_s(msg, CHAT_SIZE, "Obtained %dExp by killing %s", exp, target->GetName());
						player->Send_Chat_Packet(id, msg);
					}
					else {
						sprintf_s(msg, CHAT_SIZE, "%s Killed %s", player->GetName(), target->GetName());
						player->Send_Chat_Packet(id, msg);
					}
				}
				else {
					char msg[CHAT_SIZE];
					if (id == m_ID) {
						sprintf_s(msg, CHAT_SIZE, "Damaged %d at %s", player->GetStat()->GetPower() * 3, target->GetName());
						player->Send_Chat_Packet(id, msg);
					}
					else {
						sprintf_s(msg, CHAT_SIZE, "%s Damaged %d at %s", player->GetName(), player->GetStat()->GetPower() * 3, target->GetName());
						player->Send_Chat_Packet(id, msg);
					}
				}
			}
		}
	}
}

random_device rd;
default_random_engine dre{ rd() };
void CClient::CreateItem(short x, short y)
{
	uniform_int_distribution<> uid;
	int item = uid(dre) % 100;	//Nothing:20%, Money:50%, Potion:12%/12%, Weapon:2%/2%/2%

	ITEM_TYPE item_type = ITEM_TYPE::NONE;

	if (item <= 49) { // Money
		item_type = ITEM_TYPE::MONEY;
	}
	else if (item >= 50 && item <= 67) { // Nothing

	}
	else if (item >= 68 && item <= 69) {
		item_type = ITEM_TYPE::HAT;
	}
	else if (item >= 70 && item <= 81) { // HP Potion
		item_type = ITEM_TYPE::HP_POTION;
	}
	else if (item >= 82 && item <= 93) { // MP Potion
		item_type = ITEM_TYPE::MP_POTION;
	}
	else if (item >= 94 && item <= 95) { // Wand
		item_type = ITEM_TYPE::WAND;
	}
	else if (item >= 96 && item <= 97) { // Cloth
		item_type = ITEM_TYPE::CLOTH;
	}
	else {	// ring
		item_type = ITEM_TYPE::RING;
	}

	if (ITEM_TYPE::NONE != item_type) {
		SC_ITEM_ADD_PACKET p;
		p.type = SC_ITEM_ADD;
		p.size = sizeof(p);
		p.x = x;
		p.y = y;
		p.item_type = static_cast<int>(item_type);
		GameUtil::SetItemTile(y, x, item_type);
		SendPacket(&p);
	}
}

void CClient::UseItem(int inven)
{
	if (true == m_items[inven]->GetEnable()) {
		bool use = false;
		switch (m_items[inven]->GetItemType()) {
		case ITEM_TYPE::HP_POTION:
			m_stat->HealHp(50);
			use = true;
			Send_Chat_Packet(m_ID, "Used HP Potion");
			break;
		case ITEM_TYPE::MP_POTION:
			reinterpret_cast<CClientStat*>(m_stat.get())->HealMp(30);
			use = true;
			Send_Chat_Packet(m_ID, "Used MP Potion");
			break;
		default:
			break;
		}

		if (true == use) {
			m_itemLock.lock();
			m_items[inven]->SetEnable(false);
			m_items[inven]->SetItemType(ITEM_TYPE::NONE);
			m_itemLock.unlock();
			Send_StatChange_Packet();
			Send_ItemUsed_Packet(inven);
		}
	}
}

const ITEM_TYPE CClient::GetItemType(int index) const
{
	return m_items[index]->GetItemType();
}

void CClient::SetItem(int index, ITEM_TYPE type, int num, bool enable)
{
	m_items[index]->SetEnable(enable);
	m_items[index]->SetNum(num);
	m_items[index]->SetItemType(type);
}

bool CClient::CanUse(char skill, chrono::system_clock::time_point time)
{
	int curMp = reinterpret_cast<CClientStat*>(m_stat.get())->GetCurMp();
	switch (skill) {
	case 0:
		if (chrono::duration_cast<chrono::seconds>(time - m_usedTime[0]).count() >= ATTACK_COOL) {
			m_usedTime[0] = time;
			return true;
		}
		break;
	case 1:
		if (chrono::duration_cast<chrono::seconds>(time - m_usedTime[1]).count() >= SKILL1_COOL &&
			curMp >= 15) {
			m_usedTime[1] = time;
			return true;
		}
		break;
	case 2:
		if (chrono::duration_cast<chrono::seconds>(time - m_usedTime[2]).count() >= SKILL2_COOL &&
			curMp >= 5) {
			m_usedTime[2] = time;
			return true;
		}
		break;
	case 3:
		if (chrono::duration_cast<chrono::seconds>(time - m_usedTime[3]).count() >= SKILL3_COOL &&
			curMp >= 10) {
			m_usedTime[3] = time;
			return true;
		}
		break;
	}
	return false;

}

void CClient::Heal()
{
	CClientStat* stat = reinterpret_cast<CClientStat*>(m_stat.get());

	m_stat->HealHp(static_cast<int>(m_stat->GetMaxHp() * 0.1f));
	stat->HealMp(static_cast<int>(stat->GetMaxMp() * 0.1f));

	Send_StatChange_Packet();
		
	TIMER_EVENT ev{ m_ID, chrono::system_clock::now() + 5s, EV_PLAYER_HEAL, 0 };
	CNetworkMgr::GetInstance()->RegisterEvent(ev);
}

unordered_set<int> CClient::CheckSection()
{
	unordered_set<int> viewList;
	//Center
	for (int id : GameUtil::GetSectionObjects(m_sectionY, m_sectionX)) {
		if (id == m_ID)
			continue;
		if (true == CanSee(id))
			viewList.insert(id);
	}

	//Right
	if (m_PosX % SECTION_SIZE >= SECTION_SIZE / 2 && m_sectionX != SECTION_NUM - 1) {
		for (int id : GameUtil::GetSectionObjects(m_sectionY, m_sectionX + 1)) {
			if (id == m_ID)
				continue;
			if (true == CanSee(id))
				viewList.insert(id);
		}

		//RightDown
		if (m_PosY % SECTION_SIZE >= SECTION_SIZE / 2 && m_sectionY != SECTION_NUM - 1) {
			for (int id : GameUtil::GetSectionObjects(m_sectionY + 1, m_sectionX + 1)) {
				if (id == m_ID)
					continue;
				if (true == CanSee(id))
					viewList.insert(id);
			}
		}
		//RightUp
		else if (m_PosY % SECTION_SIZE < SECTION_SIZE / 2 && m_sectionY != 0) {
			for (int id : GameUtil::GetSectionObjects(m_sectionY - 1, m_sectionX + 1)) {
				if (id == m_ID)
					continue;
				if (true == CanSee(id))
					viewList.insert(id);
			}
		}

	}
	//Left
	else if (m_PosX % SECTION_SIZE < SECTION_SIZE / 2 && m_sectionX != 0) {
		for (int id : GameUtil::GetSectionObjects(m_sectionY, m_sectionX - 1)) {
			if (id == m_ID)
				continue;
			if (true == CanSee(id))
				viewList.insert(id);
		}
		//LeftDown
		if (m_PosY % SECTION_SIZE >= SECTION_SIZE / 2 && m_sectionY != SECTION_NUM - 1) {
			for (int id : GameUtil::GetSectionObjects(m_sectionY + 1, m_sectionX - 1)) {
				if (id == m_ID)
					continue;
				if (true == CanSee(id))
					viewList.insert(id);
			}
		}
		//LeftUp
		else if (m_PosY % SECTION_SIZE < SECTION_SIZE / 2 && m_sectionY != 0) {
			for (int id : GameUtil::GetSectionObjects(m_sectionY - 1, m_sectionX - 1)) {
				if (id == m_ID)
					continue;
				if (true == CanSee(id))
					viewList.insert(id);
			}
		}
	}

	//Down
	if (m_PosY % SECTION_SIZE >= SECTION_SIZE / 2 && m_sectionY != SECTION_NUM - 1) {
		for (int id : GameUtil::GetSectionObjects(m_sectionY + 1, m_sectionX)) {
			if (id == m_ID)
				continue;
			if (true == CanSee(id))
				viewList.insert(id);
		}
	}
	//Up
	else if (m_PosY % SECTION_SIZE < SECTION_SIZE / 2 && m_sectionY != 0) {
		for (int id : GameUtil::GetSectionObjects(m_sectionY - 1, m_sectionX)) {
			if (id == m_ID)
				continue;
			if (true == CanSee(id))
				viewList.insert(id);
		}
	}

	return viewList;
}

void CClient::Damaged(int power)
{
	if (reinterpret_cast<CClientStat*>(m_stat.get())->Damaged(power)) {	
		SetPos(48, 47);
		Send_Move_Packet(m_ID);
		Send_StatChange_Packet();
	}
	else {
		Send_StatChange_Packet();
	}
}

void CClient::Send_LoginInfo_Packet()
{
	SC_LOGIN_INFO_PACKET p;
	p.id = m_ID;
	p.size = sizeof(SC_LOGIN_INFO_PACKET);
	p.type = SC_LOGIN_INFO;
	p.x = m_PosX;
	p.y = m_PosY;

	CClientStat* stat = reinterpret_cast<CClientStat*>(m_stat.get());
	p.max_hp = m_stat->GetMaxHp();
	p.hp = m_stat->GetCurHp();
	p.max_mp = stat->GetMaxMp();
	p.mp = stat->GetCurMp();
	p.exp = stat->GetExp();
	p.level = stat->GetLevel();
	strcpy_s(p.name, m_name);
	SendPacket(&p);
}

void CClient::Send_StatChange_Packet()
{
	SC_STAT_CHANGE_PACKET packet;

	packet.size = sizeof(packet);
	packet.type = SC_STAT_CHANGE;

	CClientStat* stat = reinterpret_cast<CClientStat*>(m_stat.get());
	packet.max_hp = m_stat->GetMaxHp();
	packet.hp = m_stat->GetCurHp();
	packet.max_mp = stat->GetMaxMp();
	packet.mp = stat->GetCurMp();
	packet.exp = stat->GetExp();
	packet.level = stat->GetLevel();

	SendPacket(&packet);
}

void CClient::Send_Damage_Packet(int cid, int powerlv)
{
	SC_DAMAGE_PACKET p;

	p.size = sizeof(p);
	p.type = SC_DAMAGE;
	p.id = cid;
	p.hp = CNetworkMgr::GetInstance()->GetCObject(cid)->GetStat()->GetCurHp();

	SendPacket(&p);
}

void CClient::Send_ItemUsed_Packet(int inven)
{
	SC_ITEM_USED_PACKET p;

	p.size = sizeof(p);
	p.type = SC_ITEM_USED;
	p.inven_num = inven;

	SendPacket(&p);
}

void CClient::RecvPacket()
{
	DWORD recv_flag = 0;
	memset(&m_recvOver.m_over, 0, sizeof(m_recvOver.m_over));
	m_recvOver.m_Wsabuf.len = BUF_SIZE - m_RemainBuf_Size;
	m_recvOver.m_Wsabuf.buf = m_recvOver.m_send_buf + m_RemainBuf_Size;
	WSARecv(m_Socket, &m_recvOver.m_Wsabuf, 1, 0, &recv_flag, &m_recvOver.m_over, 0);
}

void CClient::SendPacket(void* packet)
{
	COverlapEx* sdata = new COverlapEx{ reinterpret_cast<char*>(packet) };
	WSASend(m_Socket, &sdata->m_Wsabuf, 1, 0, 0, &sdata->m_over, 0);
}

void CClient::Send_LoginFail_Packet()
{
	SC_LOGIN_FAIL_PACKET p;
	p.size = sizeof(SC_LOGIN_FAIL_PACKET);
	p.type = SC_LOGIN_FAIL;
	SendPacket(&p);
}

void CClient::Send_Move_Packet(int c_id)
{
	SC_MOVE_OBJECT_PACKET p;
	p.id = c_id;
	p.size = sizeof(SC_MOVE_OBJECT_PACKET);
	p.type = SC_MOVE_OBJECT;
	p.x = CNetworkMgr::GetInstance()->GetCObject(c_id)->GetPosX();
	p.y = CNetworkMgr::GetInstance()->GetCObject(c_id)->GetPosY();
	p.move_time = last_move_time;
	SendPacket(&p);
}

void CClient::Send_AddObject_Packet(int c_id)
{
	SC_ADD_OBJECT_PACKET add_packet;
	add_packet.id = c_id;
	strcpy_s(add_packet.name, CNetworkMgr::GetInstance()->GetCObject(c_id)->GetName());
	add_packet.size = sizeof(add_packet);
	add_packet.type = SC_ADD_OBJECT;
	add_packet.x = CNetworkMgr::GetInstance()->GetCObject(c_id)->GetPosX();
	add_packet.y = CNetworkMgr::GetInstance()->GetCObject(c_id)->GetPosY();

	if (c_id >= MAX_USER) {
		CClientStat* stat = reinterpret_cast<CClientStat*>(CNetworkMgr::GetInstance()->GetCObject(c_id)->GetStat());
		add_packet.level = stat->GetLevel();
		add_packet.hp = stat->GetCurHp();
		add_packet.max_hp = stat->GetMaxHp();
		if (add_packet.hp <= 0) {
			return;
		}
	}
	else {
		add_packet.level = 0;
		add_packet.hp = 0;
		add_packet.max_hp = 0;
	}

	m_ViewLock.lock();
	m_viewList.insert(c_id);
	m_ViewLock.unlock();
	SendPacket(&add_packet);
}

void CClient::Send_Chat_Packet(int c_id, const char* mess)
{
	SC_CHAT_PACKET packet;
	packet.id = c_id;
	packet.size = sizeof(packet);
	packet.type = SC_CHAT;
	strcpy_s(packet.mess, mess);
	SendPacket(&packet);
}

void CClient::Send_RemoveObject_Packet(int c_id)
{
	m_ViewLock.lock();
	if (m_viewList.count(c_id))
		m_viewList.erase(c_id);
	else {
		m_ViewLock.unlock();
		return;
	}
	m_ViewLock.unlock();
	SC_REMOVE_OBJECT_PACKET p;
	p.id = c_id;
	p.size = sizeof(p);
	p.type = SC_REMOVE_OBJECT;
	SendPacket(&p);
}