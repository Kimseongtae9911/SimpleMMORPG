#include "pch.h"
#include "CNpc.h"
#include "GameUtil.h"
#include "ChatUtil.h"
#include "LuaFunc.h"
#include "CNetworkMgr.h"
#include "CClient.h"
#include "CItemGenerator.h"

CNpc::CNpc(int id)
{
	m_stat = make_unique<CStat>();
	m_ID = id;
	sprintf_s(m_name, "MONSTER%d", id);

	auto L = m_L = luaL_newstate();
	luaL_openlibs(L);

	luaL_loadfile(L, "npc.lua");
	lua_pcall(L, 0, 0, 0);

	lua_register(L, "API_Initialize", API_Initialize);
	lua_register(L, "API_SendMessage", API_SendMessage);
	lua_register(L, "API_get_x", API_get_x);
	lua_register(L, "API_get_y", API_get_y);
}

CNpc::~CNpc()
{
}

void CNpc::Attack()
{
	if (m_active == false)
		return;

	CNetworkMgr::GetInstance()->GetCObject(m_target)->Damaged(m_stat->GetPower(), m_ID);
	
	ChatUtil::SendDamageMsg(m_target, m_stat->GetPower(), m_name);
}

void CNpc::Chase()
{
	unordered_set<int> old_vl;
	for (auto& obj : CNetworkMgr::GetInstance()->GetAllObject()) {
		if (CL_STATE::ST_INGAME != reinterpret_cast<CClient*>(obj)->GetState())
			continue;
		if (obj->GetID() >= MAX_USER)
			break;
		if (true == CanSee(obj->GetID()))
			old_vl.insert(obj->GetID());
	}

	int x = 0;
	int y = 0;
	if (AStar(m_PosX, m_PosY, CNetworkMgr::GetInstance()->GetCObject(m_target)->GetPosX(), CNetworkMgr::GetInstance()->GetCObject(m_target)->GetPosY(), &x, &y))
	{
		m_PosX = x;
		m_PosY = y;
	}
	else
		return;

	// 움직인 이후에 해당 NPC가 시야에 있는 플레이어들
	unordered_set<int> new_vl;
	for (auto& obj : CNetworkMgr::GetInstance()->GetAllObject()) {
		if (CL_STATE::ST_INGAME != reinterpret_cast<CClient*>(obj)->GetState())
			continue;
		if (obj->GetID() >= MAX_USER)
			continue;
		if (true == CanSee(obj->GetID()))
			new_vl.insert(obj->GetID());
	}

	for (auto pl : new_vl) {
		// new_vl에는 있는데 old_vl에는 없을 때 플레이어의 시야에 등장
		if (0 == old_vl.count(pl)) {
			reinterpret_cast<CClient*>(CNetworkMgr::GetInstance()->GetCObject(pl))->AddObjectToView(m_ID);
		}
		else {
			// 플레이어가 계속 보고 있음.
			CClient* client = reinterpret_cast<CClient*>(CNetworkMgr::GetInstance()->GetCObject(pl));
			client->GetSession()->SendMovePacket(m_ID, x, y, client->lastMoveTime);
		}
	}

	for (auto pl : old_vl) {
		if (0 == new_vl.count(pl)) {
			CNetworkMgr::GetInstance()->GetCObject(pl)->m_ViewLock.lock_shared();
			if (0 != CNetworkMgr::GetInstance()->GetCObject(pl)->GetViewList().count(m_ID)) {
				CNetworkMgr::GetInstance()->GetCObject(pl)->m_ViewLock.unlock_shared();
				reinterpret_cast<CClient*>(CNetworkMgr::GetInstance()->GetCObject(pl))->RemoveObjectFromView(m_ID);
			}
			else {
				CNetworkMgr::GetInstance()->GetCObject(pl)->m_ViewLock.unlock_shared();
			}
		}
	}
}

void CNpc::RandomMove()
{
	m_ViewLock.lock_shared();
	unordered_set<int> old_vl = m_viewList;
	m_ViewLock.unlock_shared();

	int x = m_PosX;
	int y = m_PosY;
	int dir = rand() % 4;
	if (GameUtil::CanMove(x, y, dir)) {
		switch (dir) {
		case 0:	y--; break;
		case 1:	y++; break;
		case 2:	x--; break;
		case 3:	x++; break;
		}
		SetPos(x, y);
		int sectionX = static_cast<int>(x / SECTION_SIZE);
		int sectionY = static_cast<int>(y / SECTION_SIZE);
		if (m_sectionX != sectionX || m_sectionY != sectionY) {
			GameUtil::RegisterToSection(sectionY, sectionX, m_ID);
			m_sectionX = sectionX;
			m_sectionY = sectionY;
		}
	}

	// 움직인 이후에 해당 NPC가 시야에 있는 플레이어들
	unordered_set<int> new_vl = CheckSection();

	for (auto pl : new_vl) {
		// new_vl에는 있는데 old_vl에는 없을 때 플레이어의 시야에 등장
		if (0 == old_vl.count(pl)) {
			reinterpret_cast<CClient*>(CNetworkMgr::GetInstance()->GetCObject(pl))->AddObjectToView(m_ID);
			m_ViewLock.lock();
			m_viewList.insert(pl);
			m_ViewLock.unlock();
		}
		else {
			// 플레이어가 계속 보고 있음.
			CClient* client = reinterpret_cast<CClient*>(CNetworkMgr::GetInstance()->GetCObject(pl));
			client->GetSession()->SendMovePacket(m_ID, x, y, client->lastMoveTime);
		}
	}

	for (auto pl : old_vl) {
		if (0 == new_vl.count(pl)) {
			CNetworkMgr::GetInstance()->GetCObject(pl)->m_ViewLock.lock_shared();
			if (0 != CNetworkMgr::GetInstance()->GetCObject(pl)->GetViewList().count(m_ID)) {
				CNetworkMgr::GetInstance()->GetCObject(pl)->m_ViewLock.unlock_shared();
				reinterpret_cast<CClient*>(CNetworkMgr::GetInstance()->GetCObject(pl))->RemoveObjectFromView(m_ID);
				m_ViewLock.lock();
				m_viewList.erase(pl);
				m_ViewLock.unlock();
			}
			else {
				CNetworkMgr::GetInstance()->GetCObject(pl)->m_ViewLock.unlock_shared();
			}
		}
	}
}

bool CNpc::Agro(int to)
{
	if (abs(m_PosX - CNetworkMgr::GetInstance()->GetCObject(to)->GetPosX()) > MONSTER_VIEW)
		return false;

	return abs(m_PosY - CNetworkMgr::GetInstance()->GetCObject(to)->GetPosY()) <= MONSTER_VIEW;
}

void CNpc::WakeUp(int waker)
{
	m_ViewLock.lock();
	m_viewList.insert(waker);
	m_ViewLock.unlock();

	if (m_active)
		return;

	bool old_state = false;
	if (false == atomic_compare_exchange_strong(&m_active, &old_state, true))
		return;
	TIMER_EVENT ev{ m_ID, chrono::system_clock::now(), EV_RANDOM_MOVE, 0 };
	CNetworkMgr::GetInstance()->RegisterEvent(ev);
}

unordered_set<int> CNpc::CheckSection()
{
	unordered_set<int> viewList;
	//Center
	for (int id : GameUtil::GetSectionObjects(m_sectionY, m_sectionX)) {
		if (id >= MAX_USER || id == m_ID)
			continue;
		if (true == CanSee(id))
			viewList.insert(id);
	}

	//Right
	if (m_PosX % SECTION_SIZE >= SECTION_SIZE / 2 && m_sectionX != SECTION_NUM - 1) {
		for (int id : GameUtil::GetSectionObjects(m_sectionY, m_sectionX + 1)) {
			if (id >= MAX_USER || id == m_ID)
				continue;
			if (true == CanSee(id))
				viewList.insert(id);
		}

		//RightDown
		if (m_PosY % SECTION_SIZE >= SECTION_SIZE / 2 && m_sectionY != SECTION_NUM - 1) {
			for (int id : GameUtil::GetSectionObjects(m_sectionY + 1, m_sectionX + 1)) {
				if (id >= MAX_USER || id == m_ID)
					continue;
				if (true == CanSee(id))
					viewList.insert(id);
			}
		}
		//RightUp
		else if (m_PosY % SECTION_SIZE < SECTION_SIZE / 2 && m_sectionY != 0) {
			for (int id : GameUtil::GetSectionObjects(m_sectionY - 1, m_sectionX + 1)) {
				if (id >= MAX_USER || id == m_ID)
					continue;
				if (true == CanSee(id))
					viewList.insert(id);
			}
		}

	}
	//Left
	else if (m_PosX % SECTION_SIZE < SECTION_SIZE / 2 && m_sectionX != 0) {
		for (int id : GameUtil::GetSectionObjects(m_sectionY, m_sectionX - 1)) {
			if (id >= MAX_USER || id == m_ID)
				continue;
			if (true == CanSee(id))
				viewList.insert(id);
		}
		//LeftDown
		if (m_PosY % SECTION_SIZE >= SECTION_SIZE / 2 && m_sectionY != SECTION_NUM - 1) {
			for (int id : GameUtil::GetSectionObjects(m_sectionY + 1, m_sectionX - 1)) {
				if (id >= MAX_USER || id == m_ID)
					continue;
				if (true == CanSee(id))
					viewList.insert(id);
			}
		}
		//LeftUp
		else if (m_PosY % SECTION_SIZE < SECTION_SIZE / 2 && m_sectionY != 0) {
			for (int id : GameUtil::GetSectionObjects(m_sectionY - 1, m_sectionX - 1)) {
				if (id >= MAX_USER || id == m_ID)
					continue;
				if (true == CanSee(id))
					viewList.insert(id);
			}
		}
	}

	//Down
	if (m_PosY % SECTION_SIZE >= SECTION_SIZE / 2 && m_sectionY != SECTION_NUM - 1) {
		for (int id : GameUtil::GetSectionObjects(m_sectionY + 1, m_sectionX)) {
			if (id >= MAX_USER || id == m_ID)
				continue;
			if (true == CanSee(id))
				viewList.insert(id);
		}
	}
	//Up
	else if (m_PosY % SECTION_SIZE < SECTION_SIZE / 2 && m_sectionY != 0) {
		for (int id : GameUtil::GetSectionObjects(m_sectionY - 1, m_sectionX)) {
			if (id >= MAX_USER || id == m_ID)
				continue;
			if (true == CanSee(id))
				viewList.insert(id);
		}
	}

	return viewList;
}

void CNpc::RemoveClient(int id)
{
	m_ViewLock.lock();
	m_viewList.erase(id);
	m_ViewLock.unlock();
}

bool CNpc::Damaged(int power, int attackID)
{
	if (!m_active || m_die)
		return false;

	if (m_stat->Damaged(power)) {
		CClient* client = reinterpret_cast<CClient*>(CNetworkMgr::GetInstance()->GetCObject(attackID));
		m_die = true;
		CNetworkMgr::GetInstance()->RegisterEvent({ m_ID, chrono::system_clock::now() + 30s, EV_MONSTER_RESPAWN, 0 });

		int exp = m_stat->GetLevel() * m_stat->GetLevel() * 2;
		if (m_monType == MONSTER_TYPE::AGRO)
			exp *= 2;
		reinterpret_cast<CClientStat*>(client->GetStat())->GainExp(exp);
		client->GetSession()->SendStatChangePacket(reinterpret_cast<CClientStat*>(client->GetStat()));

		ChatUtil::SendNpcKillMsg(m_ID, exp, attackID);

		ITEM_TYPE itemType = CItemGenerator::GetInstance()->CreateItem(client->GetPosX(), client->GetPosY());
		if (ITEM_TYPE::NONE != itemType) {
			SC_ITEM_ADD_PACKET p;
			p.type = SC_ITEM_ADD;
			p.size = sizeof(p);
			p.x = m_PosX;
			p.y = m_PosY;
			p.item_type = static_cast<int>(itemType);
			GameUtil::SetItemTile(m_PosY, m_PosX, itemType);
			client->GetSession()->SendPacket(&p);
		}

		return true;
	}	

	ChatUtil::SendNpcDamageMsg(m_ID, attackID);
	return false;
}

bool CNpc::AStar(int startX, int startY, int destX, int destY, int* resultx, int* resulty)
{
	int dx1[4] = { 0, 0, 1, -1 };
	int dy1[4] = { -1, 1, 0, 0 };

	if (!IsValid(startX, startY) || !IsValid(destX, destY)) return false;
	if (!IsUnBlocked(startX, startY) || !IsUnBlocked(destX, destY)) return false;
	if (IsDest(startX, startY, destX, destY)) return false;

	bool closedList[MONSTER_VIEW * 2 + 1][MONSTER_VIEW * 2 + 1] = {};

	for (int i = 0; i < MONSTER_VIEW; ++i) {
		for (int j = 0; j < MONSTER_VIEW; ++j) {
			closedList[i][j] = false;
		}
	}

	Node node[MONSTER_VIEW * 2 + 1][MONSTER_VIEW * 2 + 1] = {};

	for (int i = 0; i < MONSTER_VIEW * 2 + 1; ++i) {
		for (int j = 0; j < MONSTER_VIEW * 2 + 1; ++j) {
			node[i][j].f = node[i][j].g = node[i][j].h = INF;
			node[i][j].parentX = node[i][j].parentY = -1;
		}
	}

	int offsetX = startX;
	int offsetY = startY;
	destX = MONSTER_VIEW + (destX - startX);
	destY = MONSTER_VIEW + (destY - startY);
	startX = MONSTER_VIEW;
	startY = MONSTER_VIEW;

	node[startY][startX].f = node[startY][startX].g = node[startY][startX].h = 0.f;
	node[startY][startX].parentX = startX;
	node[startY][startX].parentY = startY;

	priority_queue<WeightPos> openList;

	openList.push({ 0.f, startX, startY });

	while (!openList.empty()) {
		WeightPos wp = openList.top();
		openList.pop();

		int x = wp.x;
		int y = wp.y;

		closedList[y][x] = true;

		double nf, ng, nh;

		for (int i = 0; i < 4; ++i) {
			int ny = y + dy1[i];
			int nx = x + dx1[i];

			if (IsValid(nx, ny)) {
				if (IsDest(nx, ny, destX, destY)) {
					node[ny][nx].parentX = x;
					node[ny][nx].parentY = y;
					FindPath(node, destX, destY, resultx, resulty);
					*resultx += (offsetX - MONSTER_VIEW);
					*resulty += (offsetY - MONSTER_VIEW);
					return true;
				}
				else if (!closedList[ny][nx] && IsUnBlocked(nx, ny)) {
					ng = node[y][x].g + 1.0f;
					nh = CalH(nx, ny, destX, destY);
					nf = ng + nh;

					if (node[ny][nx].f == INF || node[ny][nx].f > nf) {
						node[ny][nx].f = nf;
						node[ny][nx].g = ng;
						node[ny][nx].h = nh;
						node[ny][nx].parentX = x;
						node[ny][nx].parentY = y;

						openList.push({ nf, nx, ny });
					}
				}
			}
		}
	}

	return false;
}

bool CNpc::IsDest(int startX, int startY, int destX, int destY)
{
	if (startX == destX && startY == destY)
		return true;

	return false;
}

bool CNpc::IsValid(int x, int y)
{
	return (x >= 0 && x < W_WIDTH && y >= 0 && y < W_HEIGHT);
}

bool CNpc::IsUnBlocked(int x, int y)
{
	if (GameUtil::GetTile(y, x) == 'D' || GameUtil::GetTile(y, x) == 'G')
		return true;

	return false;
}

double CNpc::CalH(int x, int y, int destX, int destY)
{
	return (abs(x - destX) + abs(y - destY));
}

void CNpc::FindPath(Node node[MONSTER_VIEW * 2 + 1][MONSTER_VIEW * 2 + 1], int destX, int destY, int* x, int* y)
{
	stack<Node> s;
	s.push({ destX, destY });
	while (!(node[destY][destX].parentX == destX && node[destY][destX].parentY == destY)) {
		int tempx = node[destY][destX].parentX;
		int tempy = node[destY][destX].parentY;
		destX = tempx;
		destY = tempy;
		s.push({ destX, destY });
	}

	s.pop();
	*x = s.top().parentX;
	*y = s.top().parentY;
}