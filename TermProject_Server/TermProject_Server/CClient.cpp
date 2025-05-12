#include "pch.h"
#include "CClient.h"
#include "CNpc.h"
#include "CItem.h"
#include "GameUtil.h"
#include "CNetworkMgr.h"
#include "Global.h"
#include "CDatabase.h"

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

	if (!m_isHealEventRegistered && !stat->IsMaxMp())
	{
		TIMER_EVENT ev{ m_ID, chrono::system_clock::now() + 5s, EV_PLAYER_HEAL, 0 };
		CNetworkMgr::GetInstance()->RegisterEvent(ev);
	}
}

void CClient::UseItem(int inven)
{
	if (true == m_items[inven]->GetEnable()) {
		bool use = false;
		switch (m_items[inven]->GetItemType()) {
		case ITEM_TYPE::HP_POTION:
			static_cast<CClientStat*>(m_stat.get())->HealHp(50);
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
	CClientStat* stat = static_cast<CClientStat*>(m_stat.get());

	stat->HealHp(static_cast<int>(m_stat->GetMaxHp() * 0.1f));
	stat->HealMp(static_cast<int>(stat->GetMaxMp() * 0.1f));

	m_session->SendStatChangePacket(static_cast<CClientStat*>(m_stat.get()));
	
	if (!stat->IsFullCondition())
	{
		TIMER_EVENT ev{ m_ID, chrono::system_clock::now() + 5s, EV_PLAYER_HEAL, 0 };
		CNetworkMgr::GetInstance()->RegisterEvent(ev);
	}
	else
		m_isHealEventRegistered = false;
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
		GameUtil::RegisterToSection(m_sectionY, m_sectionX, sectionY, sectionX, m_ID);
		m_sectionX = sectionX;
		m_sectionY = sectionY;
	}

	unordered_set<int> oldView = m_viewList;
	std::unordered_set<int> newView;
	CheckSection(newView);

	m_session->SendMovePacket(m_ID, x, y, lastMoveTime);

	for (auto& pl : newView) {
		CObject* cpl = CNetworkMgr::GetInstance()->GetCObject(pl);
		if (pl < MAX_USER) {
			CClient* cl = reinterpret_cast<CClient*>(cpl);

			cl->GetJobQueue().PushJob([this, cl, x, y]() {
				// 시야에 있다면 패킷 전송, 없다면 시야에 추가
				const auto& viewList = cl->GetViewList();
				viewList.end() != viewList.find(m_ID) ? cl->GetSession()->SendMovePacket(m_ID, x, y, lastMoveTime) : cl->AddObjectToView(m_ID);
				});
			GPacketJobQueue->AddSessionQueue(cl);
		}
		else {
			reinterpret_cast<CNpc*>(cpl)->WakeUp(m_ID);
		}

		if (oldView.find(pl) == oldView.end())
			AddObjectToView(pl);
	}

	for (auto& pl : oldView) {
		if (newView.end() == newView.find(pl)) {
			RemoveObjectFromView(pl);
			if (pl < MAX_USER)
			{
				auto otherClient = reinterpret_cast<CClient*>(CNetworkMgr::GetInstance()->GetCObject(pl));
				otherClient->GetJobQueue().PushJob([this, otherClient]() {
					otherClient->RemoveObjectFromView(m_ID);
					});
				GPacketJobQueue->AddSessionQueue(otherClient);
			}
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

void CClient::CheckSection(std::unordered_set<int>& viewList)
{
	viewList.reserve(10);
	const auto InsertToViewList = [this, &viewList](int sectionX, int sectionY) {
		for (int id : GameUtil::GetSectionObjects(sectionY, sectionX)) {
			if (id == m_ID)
				continue;

			if (CanSee(id))
				viewList.insert(id);
		}
		};
	//Center
	InsertToViewList(m_sectionX, m_sectionY);

	//Right
	if (m_sectionX <= SECTION_NUM - 2) {
		InsertToViewList(m_sectionX + 1, m_sectionY);

		//RightDown
		if (m_sectionY <= SECTION_NUM - 2)
			InsertToViewList(m_sectionX + 1, m_sectionY + 1);

		//RightUp
		if (m_sectionY >= 1)
			InsertToViewList(m_sectionX + 1, m_sectionY - 1);	

	}
	//Left
	if (m_sectionX >= 1) {
		InsertToViewList(m_sectionX - 1, m_sectionY);

		//LeftDown
		if (m_sectionY <= SECTION_NUM - 2)
			InsertToViewList(m_sectionX - 1, m_sectionY + 1);

		//LeftUp
		if (m_sectionY >= 1)
			InsertToViewList(m_sectionX - 1, m_sectionY - 1);
	}

	//Down
	if (m_sectionY <= SECTION_NUM - 2)
		InsertToViewList(m_sectionX, m_sectionY + 1);
	
	//Up
	if (m_sectionY >= 1)
		InsertToViewList(m_sectionX, m_sectionY - 1);
}

bool CClient::Damaged(int power, int attackID)
{
	const auto RegisterHealEvent = [this]() {
		auto stat = static_cast<CClientStat*>(m_stat.get());
		if (!m_isHealEventRegistered && !stat->IsMaxHp())
		{
			TIMER_EVENT ev{ m_ID, chrono::system_clock::now() + 5s, EV_PLAYER_HEAL, 0 };
			CNetworkMgr::GetInstance()->RegisterEvent(ev);
		}
		};

	if (m_stat->Damaged(power)) {	
		SetPos(48, 47);

		int sectionX = static_cast<int>(m_PosX / SECTION_SIZE);
		int sectionY = static_cast<int>(m_PosY / SECTION_SIZE);

		if (m_sectionX != sectionX || m_sectionY != sectionY) {
			GameUtil::RegisterToSection(m_sectionY, m_sectionX, sectionY, sectionX, m_ID);
			m_sectionX = sectionX;
			m_sectionY = sectionY;
		}

		unordered_set<int> oldView = m_viewList;
		std::unordered_set<int> newView;
		CheckSection(newView);

		for (auto& pl : newView) {
			CObject* cpl = CNetworkMgr::GetInstance()->GetCObject(pl);
			if (pl < MAX_USER) {
				CClient* cl = reinterpret_cast<CClient*>(cpl);

				cl->GetJobQueue().PushJob([this, cl]() {
					// 시야에 있다면 패킷 전송, 없다면 시야에 추가
					const auto& viewList = cl->GetViewList();
					viewList.end() != viewList.find(m_ID) ? cl->GetSession()->SendMovePacket(m_ID, m_PosX, m_PosY, lastMoveTime) : cl->AddObjectToView(m_ID);
					});
				GPacketJobQueue->AddSessionQueue(cl);
			}
			else {
				reinterpret_cast<CNpc*>(cpl)->WakeUp(m_ID);
			}

			if (oldView.find(pl) == oldView.end())
				AddObjectToView(pl);
		}

		for (auto& pl : oldView) {
			if (newView.end() == newView.find(pl)) {
				RemoveObjectFromView(pl);
				if (pl < MAX_USER)
				{
					auto otherClient = reinterpret_cast<CClient*>(CNetworkMgr::GetInstance()->GetCObject(pl));
					otherClient->GetJobQueue().PushJob([this, otherClient]() {
						otherClient->RemoveObjectFromView(m_ID);
						});
					GPacketJobQueue->AddSessionQueue(otherClient);
				}
			}
		}

		m_session->SendMovePacket(m_ID, 48, 47, lastMoveTime);
		m_session->SendStatChangePacket(reinterpret_cast<CClientStat*>(m_stat.get()));

		RegisterHealEvent();
		return true;
	}
	else {
		m_session->SendStatChangePacket(reinterpret_cast<CClientStat*>(m_stat.get()));
		RegisterHealEvent();
		return false;
	}
}

void CClient::Logout()
{
	m_isDisconnected = true;

	auto stat = static_cast<CClientStat*>(m_stat.get());

	USER_INFO ui;
	memcpy(ui.name, m_name, NAME_SIZE);
	ui.pos_x = m_PosX;
	ui.pos_y = m_PosY;
	ui.cur_hp = stat->GetCurHp();
	ui.max_hp = stat->GetMaxHp();
	ui.exp = stat->GetExp();
	ui.cur_mp = stat->GetMp();
	ui.max_mp = stat->GetMaxMp();
	ui.level = stat->GetLevel();
	ui.item1 = GetItemType(0) - 1;
	ui.item2 = GetItemType(1) - 1;
	ui.item3 = GetItemType(2) - 1;
	ui.item4 = GetItemType(3) - 1;
	ui.item5 = GetItemType(4) - 1;
	ui.item6 = GetItemType(5) - 1;
	ui.moneycnt = 78;

#ifdef DATABASE
	CNetworkMgr::GetInstance()->GetDatabase()->SavePlayerInfo(ui);
#endif

	for (auto& p_id : m_viewList) {
		if (p_id >= MAX_USER)
			continue;

		auto pl = reinterpret_cast<CClient*>(CNetworkMgr::GetInstance()->GetCObject(p_id));
		pl->GetJobQueue().PushJob([this, pl]() {
			pl->RemoveObjectFromView(m_ID);
			});
		GPacketJobQueue->AddSessionQueue(pl);
	}
	closesocket(m_session->GetSocket());

	m_State = CL_STATE::ST_FREE;
}

void CClient::AddObjectToView(int c_id)
{
	m_viewList.insert(c_id);

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
	if (c_id == m_ID)
		return;

	const auto it = m_viewList.find(c_id);
	if (it == m_viewList.end())
		return;

	m_viewList.erase(c_id);

	SC_REMOVE_OBJECT_PACKET p;
	p.id = c_id;
	p.size = sizeof(p);
	p.type = SC_REMOVE_OBJECT;
	m_session->SendPacket(&p);
}