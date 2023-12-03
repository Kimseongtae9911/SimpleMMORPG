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

	m_skills[0] = new CAttack();
	m_skills[1] = new CPowerUp();
	m_skills[2] = new CFire();
	m_skills[3] = new CExplosion();

	for (int i = 0; i < m_items.size(); ++i) {
		m_items[i] = new CItem();
	}
}

CClient::~CClient()
{
	for (auto& skill : m_skills)
		delete skill;
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

bool CClient::Damaged(int power, int attackID)
{
	if (m_stat->Damaged(power)) {	
		SetPos(48, 47);
		Send_Move_Packet(m_ID);
		Send_StatChange_Packet();
		return true;
	}
	else {
		Send_StatChange_Packet();
		return false;
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