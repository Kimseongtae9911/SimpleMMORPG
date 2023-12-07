#include "pch.h"
#include "CClient.h"
#include "CNpc.h"
#include "CItem.h"
#include "GameUtil.h"
#include "CNetworkMgr.h"

CClient::CClient()
{
	m_State = CL_STATE::ST_FREE;

	m_stat = make_unique<CClientStat>(20);

	m_skills[0] = new CAttack();
	m_skills[1] = new CPowerUp();
	m_skills[2] = new CFire();
	m_skills[3] = new CExplosion();

	for (int i = 0; i < m_items.size(); ++i) {
		m_items[i] = new CItem();
	}
	m_session = new CSession();
}

CClient::~CClient()
{
	for (auto& skill : m_skills)
		delete skill;

	delete m_session;
}

void CClient::PlayerAccept(int id, SOCKET sock)
{
	m_PosX = 0;
	m_PosY = 0;
	m_ID = id;
	m_name[0] = 0;
	m_session->Initialize(sock);
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
			m_session->SendPacket(&p);

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
			m_session->SendChatPacket(m_ID, msg);
		}
		GameUtil::SetItemTile(m_PosY, m_PosX, ITEM_TYPE::NONE);
	}
}

void CClient::UseSkill(int skill)
{
	CClientStat* stat = reinterpret_cast<CClientStat*>(m_stat.get());
	if (m_skills[skill]->CanUse(stat->GetCurMp())) {
		m_skills[skill]->Use(m_ID);
		stat->SetMp(stat->GetCurMp() - m_skills[skill]->GetMpConsumtion());
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
			m_session->SendChatPacket(m_ID, "Used HP Potion");
			break;
		case ITEM_TYPE::MP_POTION:
			reinterpret_cast<CClientStat*>(m_stat.get())->HealMp(30);
			use = true;
			m_session->SendChatPacket(m_ID, "Used MP Potion");
			break;
		default:
			break;
		}

		if (true == use) {
			m_itemLock.lock();
			m_items[inven]->SetEnable(false);
			m_items[inven]->SetItemType(ITEM_TYPE::NONE);
			m_itemLock.unlock();
			m_session->SendStatChangePacket(reinterpret_cast<CClientStat*>(m_stat.get()));
			m_session->SendItemUsePacket(inven);
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

void CClient::Heal()
{
	CClientStat* stat = reinterpret_cast<CClientStat*>(m_stat.get());

	m_stat->HealHp(static_cast<int>(m_stat->GetMaxHp() * 0.1f));
	stat->HealMp(static_cast<int>(stat->GetMaxMp() * 0.1f));

	m_session->SendStatChangePacket(reinterpret_cast<CClientStat*>(m_stat.get()));
		
	TIMER_EVENT ev{ m_ID, chrono::system_clock::now() + 5s, EV_PLAYER_HEAL, 0 };
	CNetworkMgr::GetInstance()->RegisterEvent(ev);
}

void CClient::Move(char dir)
{
	short x = m_PosX;
	short y = m_PosY;
	switch (dir) {
	case 0: {
		y--; break;
	}
	case 1: {
		y++; break;
	}
	case 2: {
		x--; break;
	}
	case 3: {
		x++; break;
	}
	}
	if (!GameUtil::CanMove(x, y)) {
		return;
	}

	SetPos(x, y);
	CheckItem();
#ifdef WITH_VIEW
#ifdef WITH_SECTION
	int sectionX = static_cast<int>(x / SECTION_SIZE);
	int sectionY = static_cast<int>(y / SECTION_SIZE);

	if (m_sectionX != sectionX || m_sectionY != sectionY) {
		GameUtil::RegisterToSection(m_sectionX, m_sectionY, sectionY, sectionX, m_ID);
		m_sectionX = sectionX;
		m_sectionY = sectionY;
	}

	m_ViewLock.lock_shared();
	unordered_set<int> old_vlist = m_viewList;
	m_ViewLock.unlock_shared();

	unordered_set<int> near_list = CheckSection();

	m_session->SendMovePacket(m_ID, x, y, lastMoveTime);

	for (auto& pl : near_list) {
		CObject* cpl = CNetworkMgr::GetInstance()->GetCObject(pl);
		if (pl < MAX_USER) {
			CClient* cl = reinterpret_cast<CClient*>(cpl);
			cpl->m_ViewLock.lock_shared();
			if (cpl->GetViewList().count(m_ID)) {
				cpl->m_ViewLock.unlock_shared();
				cl->GetSession()->SendMovePacket(m_ID, x, y, lastMoveTime);
			}
			else {
				cpl->m_ViewLock.unlock_shared();
				cl->AddObjectToView(m_ID);
			}
		}
		else {
			reinterpret_cast<CNpc*>(cpl)->WakeUp(m_ID);
		}

		if (old_vlist.count(pl) == 0)
			AddObjectToView(pl);
	}

	for (auto& pl : old_vlist) {
		if (0 == near_list.count(pl)) {
			RemoveObjectFromView(pl);
			if (pl < MAX_USER)
				reinterpret_cast<CClient*>(CNetworkMgr::GetInstance()->GetCObject(pl))->RemoveObjectFromView(m_ID);
		}
	}
#else
	m_ViewLock.lock_shared();
	unordered_set<int> oldView = m_viewList;
	m_ViewLock.unlock_shared();

	unordered_set<int> nearView;

	for (auto& obj : CNetworkMgr::GetInstance()->GetAllObject()) {
		if (obj->GetID() < MAX_USER && CL_STATE::ST_INGAME != reinterpret_cast<CClient*>(obj)->GetState())
			continue;
		if (obj->GetID() == m_ID)
			continue;
		if (true == CanSee(obj->GetID()))
			nearView.insert(obj->GetID());
	}

	m_session->SendMovePacket(m_ID, x, y, lastMoveTime);

	for (auto& pl : nearView) {
		CObject* cpl = CNetworkMgr::GetInstance()->GetCObject(pl);
		if (pl < MAX_USER) {
			CClient* cl = reinterpret_cast<CClient*>(cpl);
			cpl->m_ViewLock.lock_shared();
			if (cpl->GetViewList().count(m_ID)) {
				cpl->m_ViewLock.unlock_shared();
				cl->GetSession()->SendMovePacket(m_ID, x, y, lastMoveTime);
			}
			else {
				cpl->m_ViewLock.unlock_shared();
				cl->AddObjectToView(m_ID);
			}
		}
		else {
			reinterpret_cast<CNpc*>(cpl)->WakeUp(m_ID);
		}

		if (oldView.count(pl) == 0)
			AddObjectToView(pl);
	}

	for (auto& pl : oldView) {
		if (0 == nearView.count(pl)) {
			RemoveObjectFromView(pl);
			if (pl < MAX_USER)
				reinterpret_cast<CClient*>(CNetworkMgr::GetInstance()->GetCObject(pl))->RemoveObjectFromView(m_ID);
		}
	}
#endif
#else
	for (auto& obj : CNetworkMgr::GetInstance()->GetAllObject()) {
		if (obj->GetID() >= MAX_USER) {
			if (CanSee(obj->GetID()))
				reinterpret_cast<CNpc*>(obj)->WakeUp(m_ID);
		}
		else {
			CClient* cl = reinterpret_cast<CClient*>(obj);
			if (cl->GetState() != CL_STATE::ST_INGAME)
				continue;
			cl->GetSession()->SendMovePacket(m_ID, x, y, lastMoveTime);
		}
	}
#endif
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

bool CClient::Damaged(int power, int attackID)
{
	if (m_stat->Damaged(power)) {	
		SetPos(48, 47);
		m_session->SendMovePacket(m_ID, 48, 47, lastMoveTime);
		m_session->SendStatChangePacket(reinterpret_cast<CClientStat*>(m_stat.get()));
		return true;
	}
	else {
		m_session->SendStatChangePacket(reinterpret_cast<CClientStat*>(m_stat.get()));
		return false;
	}
}

void CClient::AddObjectToView(int c_id)
{
	m_ViewLock.lock();
	m_viewList.insert(c_id);
	m_ViewLock.unlock();

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

	m_session->SendPacket(&add_packet);
}

void CClient::RemoveObjectFromView(int c_id)
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
	m_session->SendPacket(&p);
}