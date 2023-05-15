#include "pch.h"
#include "CObject.h"
#include "LuaFunc.h"
#include "CItem.h"
#include "CNetworkMgr.h"
#include "GameUtil.h"

CObject::CObject()
{
	m_ID = -1;
	m_Socket = 0;
	m_PosX = m_PosY = 0;
	m_Name[0] = 0;
	m_State = CL_STATE::ST_FREE;
	m_RemainBuf_Size = 0;
}

CObject::~CObject()
{
}

void CObject::RecvPacket()
{
	DWORD recv_flag = 0;
	memset(&m_RecvOver.m_over, 0, sizeof(m_RecvOver.m_over));
	m_RecvOver.m_Wsabuf.len = BUF_SIZE - m_RemainBuf_Size;
	m_RecvOver.m_Wsabuf.buf = m_RecvOver.m_send_buf + m_RemainBuf_Size;
	WSARecv(m_Socket, &m_RecvOver.m_Wsabuf, 1, 0, &recv_flag, &m_RecvOver.m_over, 0);
}

void CObject::SendPacket(void* packet)
{
	OVER_EXP* sdata = new OVER_EXP{ reinterpret_cast<char*>(packet) };
	WSASend(m_Socket, &sdata->m_Wsabuf, 1, 0, 0, &sdata->m_over, 0);
}

void CObject::Send_LoginFail_Packet()
{
	SC_LOGIN_FAIL_PACKET p;
	p.size = sizeof(SC_LOGIN_FAIL_PACKET);
	p.type = SC_LOGIN_FAIL;
	SendPacket(&p);
}

void CObject::Send_Move_Packet(int c_id)
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

void CObject::Send_AddObject_Packet(int c_id)
{
	SC_ADD_OBJECT_PACKET add_packet;
	add_packet.id = c_id;
	strcpy_s(add_packet.name, CNetworkMgr::GetInstance()->GetCObject(c_id)->GetName());
	add_packet.size = sizeof(add_packet);
	add_packet.type = SC_ADD_OBJECT;
	add_packet.x = CNetworkMgr::GetInstance()->GetCObject(c_id)->GetPosX();
	add_packet.y = CNetworkMgr::GetInstance()->GetCObject(c_id)->GetPosY();

	if (c_id >= MAX_USER) {
		add_packet.level = CNetworkMgr::GetInstance()->GetCObject(c_id)->GetLevel();
		add_packet.hp = CNetworkMgr::GetInstance()->GetCObject(c_id)->GetCurHp();
		add_packet.max_hp = CNetworkMgr::GetInstance()->GetCObject(c_id)->GetMaxHp();
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
	m_view_list.insert(c_id);
	m_ViewLock.unlock();
	SendPacket(&add_packet);
}

void CObject::Send_Chat_Packet(int c_id, const char* mess)
{
	SC_CHAT_PACKET packet;
	packet.id = c_id;
	packet.size = sizeof(packet);
	packet.type = SC_CHAT;
	strcpy_s(packet.mess, mess);
	SendPacket(&packet);
}

void CObject::Send_RemoveObject_Packet(int c_id)
{
	m_ViewLock.lock();
	if (m_view_list.count(c_id))
		m_view_list.erase(c_id);
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

const bool CObject::CanSee(int to) const
{
	if (abs(GetPosX() - CNetworkMgr::GetInstance()->GetCObject(to)->GetPosX()) > VIEW_RANGE)
		return false;

	return abs(GetPosY() - CNetworkMgr::GetInstance()->GetCObject(to)->GetPosY()) <= VIEW_RANGE;
}

CNpc::CNpc(int id)
{
	m_ID = id;
	sprintf_s(m_Name, "MONSTER%d", id);
	m_State = CL_STATE::ST_INGAME;

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

	CPlayer* player = reinterpret_cast<CPlayer*>(CNetworkMgr::GetInstance()->GetCObject(m_target));

	player->SetCurHp(player->GetCurHp() - GetPower());
	if (player->GetCurHp() <= 0) {
		player->SetCurHp(player->GetMaxHp() / 2);
		player->SetMp(player->GetMaxMp() / 2);
		player->SetExp(player->GetExp() / 2);
		player->SetPos(48, 47);
		player->Send_Move_Packet(player->GetID());
		player->Send_StatChange_Packet();
	}
	else {
		player->Send_StatChange_Packet();
	}

	char msg[CHAT_SIZE];

	for (int i = 0; i < MAX_USER; ++i) {
		if (CNetworkMgr::GetInstance()->GetCObject(i)->GetState() != CL_STATE::ST_INGAME)
			continue;
		if (i == player->GetID()) {
			sprintf_s(msg, CHAT_SIZE, "%dDamage from %s", GetPower(), GetName());
		}
		else {
			sprintf_s(msg, CHAT_SIZE, "%s has been damaged %d by %s", player->GetName(), GetPower(), GetName());
		}
		CNetworkMgr::GetInstance()->GetCObject(i)->Send_Chat_Packet(i, msg);
	}

	if (player->GetHeal() == false) {
		TIMER_EVENT ev{ GetTarget(), chrono::system_clock::now() + 5s, EV_PLAYER_HEAL, 0 };
		CNetworkMgr::GetInstance()->RegisterEvent(ev);
		player->SetHeal(true);
	}
}

void CNpc::Chase()
{
	unordered_set<int> old_vl;
	for (auto& obj : CNetworkMgr::GetInstance()->GetAllObject()) {
		if (CL_STATE::ST_INGAME != obj->GetState())
			continue;
		if (obj->GetID() >= MAX_USER)
			continue;
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
		if (CL_STATE::ST_INGAME != obj->GetState())
			continue;
		if (obj->GetID() >= MAX_USER)
			continue;
		if (true == CanSee(obj->GetID()))
			new_vl.insert(obj->GetID());
	}

	for (auto pl : new_vl) {
		// new_vl에는 있는데 old_vl에는 없을 때 플레이어의 시야에 등장
		if (0 == old_vl.count(pl)) {
			CNetworkMgr::GetInstance()->GetCObject(pl)->Send_AddObject_Packet(m_ID);
		}
		else {
			// 플레이어가 계속 보고 있음.
			CNetworkMgr::GetInstance()->GetCObject(pl)->Send_Move_Packet(m_ID);
		}
	}

	for (auto pl : old_vl) {
		if (0 == new_vl.count(pl)) {
			CNetworkMgr::GetInstance()->GetCObject(pl)->m_ViewLock.lock_shared();
			if (0 != CNetworkMgr::GetInstance()->GetCObject(pl)->GetViewList().count(m_ID)) {
				CNetworkMgr::GetInstance()->GetCObject(pl)->m_ViewLock.unlock_shared();
				CNetworkMgr::GetInstance()->GetCObject(pl)->Send_RemoveObject_Packet(m_ID);
			}
			else {
				CNetworkMgr::GetInstance()->GetCObject(pl)->m_ViewLock.unlock_shared();
			}
		}
	}
}

void CNpc::RandomMove()
{	
	unordered_set<int> old_vl;
	for (auto& obj : CNetworkMgr::GetInstance()->GetAllObject()) {
		if (CL_STATE::ST_INGAME != obj->GetState())
			continue;
		if (obj->GetID() >= MAX_USER)
			continue;
		if (true == CanSee(obj->GetID()))
			old_vl.insert(obj->GetID());
	}

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
	}

	// 움직인 이후에 해당 NPC가 시야에 있는 플레이어들
	unordered_set<int> new_vl;
	for (auto& obj : CNetworkMgr::GetInstance()->GetAllObject()) {
		if (CL_STATE::ST_INGAME != obj->GetState())
			continue;
		if (obj->GetID() >= MAX_USER)
			continue;
		if (true == CanSee(obj->GetID()))
			new_vl.insert(obj->GetID());
	}

	for (auto pl : new_vl) {
		// new_vl에는 있는데 old_vl에는 없을 때 플레이어의 시야에 등장
		if (0 == old_vl.count(pl)) {
			CNetworkMgr::GetInstance()->GetCObject(pl)->Send_AddObject_Packet(m_ID);
		}
		else {
			// 플레이어가 계속 보고 있음.
			CNetworkMgr::GetInstance()->GetCObject(pl)->Send_Move_Packet(m_ID);
		}
	}

	for (auto pl : old_vl) {
		if (0 == new_vl.count(pl)) {
			CNetworkMgr::GetInstance()->GetCObject(pl)->m_ViewLock.lock_shared();
			if (0 != CNetworkMgr::GetInstance()->GetCObject(pl)->GetViewList().count(m_ID)) {
				CNetworkMgr::GetInstance()->GetCObject(pl)->m_ViewLock.unlock_shared();
				CNetworkMgr::GetInstance()->GetCObject(pl)->Send_RemoveObject_Packet(m_ID);
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
	//OVER_EXP* exover = new OVER_EXP;
	//exover->m_comp_type = OP_TYPE::OP_AI_HELLO;
	//exover->m_ai_target_obj = waker;
	//PostQueuedCompletionStatus(h_iocp, 1, npc_id, &exover->m_over);

	if (m_active)
		return;
	bool old_state = false;
	if (false == atomic_compare_exchange_strong(&m_active, &old_state, true))
		return;
	TIMER_EVENT ev{ m_ID, chrono::system_clock::now(), EV_RANDOM_MOVE, 0 };	
	CNetworkMgr::GetInstance()->RegisterEvent(ev);
}

bool CNpc::AStar(int startX, int startY, int destX, int destY, int* resultx, int* resulty)
{
	int dx1[4] = { 0, 0, 1, -1 };
	int dy1[4] = { -1, 1, 0, 0 };

	if (!IsValid(startX, startY) || !IsValid(destX, destY)) return false;
	if (!IsUnBlocked(startX, startY) || !IsUnBlocked(destX, destY)) return false;
	if (IsDest(startX, startY, destX, destY)) return false;

	bool** closedList = new bool* [W_HEIGHT];
	for (int i = 0; i < W_HEIGHT; ++i) {
		closedList[i] = new bool[W_WIDTH];

		for (int j = 0; j < W_WIDTH; ++j) {
			closedList[i][j] = false;
		}
	}

	Node** node = new Node * [W_HEIGHT];
	for (int i = 0; i < W_HEIGHT; ++i) {
		node[i] = new Node[W_WIDTH];
	}

	for (int i = 0; i < W_HEIGHT; ++i) {
		for (int j = 0; j < W_WIDTH; ++j) {
			node[i][j].f = node[i][j].g = node[i][j].h = INF;
			node[i][j].parentX = node[i][j].parentY = -1;
		}
	}

	node[startY][startX].f = node[startY][startX].g = node[startY][startX].h = 0.f;
	node[startY][startX].parentX = startX;
	node[startY][startX].parentY = startY;

	priority_queue<WeightPos> openList;
	//set<WeightPos> openList;
	openList.push({ 0.f, startX, startY });
	//openList.insert({ 0.f, startX, startY });

	while (!openList.empty()) {
		//WeightPos wp = *openList.begin();
		//openList.erase(openList.begin());
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
					for (int i = 0; i < W_HEIGHT; ++i) {
						delete[] closedList[i];
					}
					delete[] closedList;
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

						//openList.insert({ nf, nx, ny });
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
	return (x >= 0 && x < W_WIDTH&& y >= 0 && y < W_HEIGHT);
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

void CNpc::FindPath(Node* node[], int destX, int destY, int* x, int* y)
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

	for (int i = 0; i < W_HEIGHT; ++i)
		delete[] node[i];

	delete[] node;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

CPlayer::CPlayer()
{
	m_usedTime = { };
	m_power = 20;
	m_poweruptime = {};
	m_powerup = false;
	for (int i = 0; i < m_items.size(); ++i) {
		m_items[i] = new CItem();
	}
}

CPlayer::~CPlayer()
{
}

void CPlayer::PlayerAccept(int id, SOCKET sock)
{
	m_PosX = 0;
	m_PosY = 0;
	m_ID = id;
	m_Name[0] = 0;
	m_RemainBuf_Size = 0;
	m_Socket = sock;
	RecvPacket();
}

void CPlayer::CheckItem()
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

void CPlayer::Attack()
{
	m_ViewLock.lock_shared();
	auto v_list = m_view_list;
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
					m_power /= 2;
				}
			}
			CObject* client2 = CNetworkMgr::GetInstance()->GetCObject(cid);

			vector<int> see_monster;
			for (auto& obj : CNetworkMgr::GetInstance()->GetAllObject()) {
				if (CL_STATE::ST_INGAME != obj->GetState())
					continue;
				if (obj->GetID() >= MAX_USER)
					break;
				if (true == obj->CanSee(CNetworkMgr::GetInstance()->GetCObject(id)->GetID()))
					see_monster.emplace_back(obj->GetID());
			}

			client2->SetCurHp(client2->GetCurHp() - m_power * 2);

			bool remove = false;
			int exp = 0;
			if (client2->GetCurHp() <= 0.f) {
				remove = true;
				reinterpret_cast<CNpc*>(client2)->SetDie(true);
				TIMER_EVENT ev = { cid, chrono::system_clock::now() + 30s, EV_MONSTER_RESPAWN, 0 };
				CNetworkMgr::GetInstance()->RegisterEvent(ev);
				exp = client2->GetLevel() * client2->GetLevel() * 2;
				if (reinterpret_cast<CNpc*>(client2)->GetMonType() == MONSTER_TYPE::AGRO)
					exp *= 2;
				GainExp(exp);
				Send_StatChange_Packet();
				CreateItem(client2->GetPosX(), client2->GetPosY());
			}

			for (const auto id : see_monster) {
				CPlayer* client3 = reinterpret_cast<CPlayer*>(CNetworkMgr::GetInstance()->GetCObject(id));
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
						sprintf_s(msg, CHAT_SIZE, "Damaged %d at %s", client3->GetPower() * 2, client2->GetName());
						client3->Send_Chat_Packet(id, msg);
					}
					else {
						sprintf_s(msg, CHAT_SIZE, "%s Damaged %d at %s", client3->GetName(), client3->GetPower() * 2, client2->GetName());
						client3->Send_Chat_Packet(id, msg);
					}
				}
			}
		}
	}	
}

void CPlayer::Skill1()
{
	m_poweruptime = chrono::system_clock::now();
	m_powerup = true;
}

void CPlayer::Skill2()
{
	m_ViewLock.lock_shared();
	auto v_list = m_view_list;
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
					m_power /= 2;
				}
			}
			
			vector<int> see_monster;
			for (auto& obj : CNetworkMgr::GetInstance()->GetAllObject()) {
				if (CL_STATE::ST_INGAME != obj->GetState())
					continue;
				if (obj->GetID() >= MAX_USER)
					break;
				if (true == obj->CanSee(client->GetID()))
					see_monster.emplace_back(obj->GetID());
			}

			CObject* target = CNetworkMgr::GetInstance()->GetCObject(cid);

			target->SetCurHp(target->GetCurHp() - m_power * 4);

			bool remove = false;
			int exp = 0;
			if (target->GetCurHp() <= 0) {
				remove = true;
				reinterpret_cast<CNpc*>(target)->SetDie(true);
				TIMER_EVENT ev = { cid, chrono::system_clock::now() + 30s, EV_MONSTER_RESPAWN, 0 };
				CNetworkMgr::GetInstance()->RegisterEvent(ev);
				exp = target->GetLevel() * target->GetLevel() * 2;
				if (reinterpret_cast<CNpc*>(target)->GetMonType() == MONSTER_TYPE::AGRO)
					exp *= 2;
				GainExp(exp);
				Send_StatChange_Packet();
				CreateItem(target->GetPosX(), target->GetPosY());
			}

			for (const auto id : see_monster) {
				CPlayer* player = reinterpret_cast<CPlayer*>(CNetworkMgr::GetInstance()->GetCObject(id));
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
						sprintf_s(msg, CHAT_SIZE, "Damaged %d at %s", player->GetPower() * 4, target->GetName());
						player->Send_Chat_Packet(id, msg);
					}
					else {
						sprintf_s(msg, CHAT_SIZE, "%s Damaged %d at %s", player->GetName(), player->GetPower() * 4, target->GetName());
						player->Send_Chat_Packet(id, msg);
					}
				}
			}
		}
	}
}

void CPlayer::Skill3()
{
	m_ViewLock.lock_shared();
	auto v_list = m_view_list;
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
					m_power /= 2;
				}
			}
			vector<int> see_monster;
			for (auto& obj : CNetworkMgr::GetInstance()->GetAllObject()) {
				if (CL_STATE::ST_INGAME != obj->GetState())
					continue;
				if (obj->GetID() >= MAX_USER)
					break;
				if (true == obj->CanSee(client->GetID()))
					see_monster.emplace_back(obj->GetID());
			}

			CNpc* target = reinterpret_cast<CNpc*>(CNetworkMgr::GetInstance()->GetCObject(cid));
			target->SetCurHp(target->GetCurHp() - m_power * 3);
			
			bool remove = false;
			int exp = 0;
			if (target->GetCurHp() <= 0) {
				remove = true;
				target->SetDie(true);
				TIMER_EVENT ev = { cid, chrono::system_clock::now() + 30s, EV_MONSTER_RESPAWN, 0 };
				CNetworkMgr::GetInstance()->RegisterEvent(ev);
				exp = target->GetLevel() * target->GetLevel() * 2;
				if (target->GetMonType() == MONSTER_TYPE::AGRO)
					exp *= 2;
				GainExp(exp);
				Send_StatChange_Packet();
				CreateItem(target->GetPosX(), target->GetPosY());
			}

			for (const auto id : see_monster) {
				CPlayer* player = reinterpret_cast<CPlayer*>(CNetworkMgr::GetInstance()->GetCObject(id));
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
						sprintf_s(msg, CHAT_SIZE, "Damaged %d at %s", player->GetPower() * 3, target->GetName());
						player->Send_Chat_Packet(id, msg);
					}
					else {
						sprintf_s(msg, CHAT_SIZE, "%s Damaged %d at %s", player->GetName(), player->GetPower() * 3, target->GetName());
						player->Send_Chat_Packet(id, msg);
					}
				}
			}
		}
	}
}

void CPlayer::GainExp(int exp)
{
	m_exp += exp;
	if (m_exp >= m_maxExp) {
		while (true) {
			m_exp -= m_maxExp;
			m_level++;
			m_maxExp = INIT_EXP + (m_level - 1) * EXP_UP;

			if (m_exp < m_maxExp)
				break;
		}
		m_maxHp = m_level * 100;
		m_curHp = m_maxHp;
		m_maxMp = m_level * 50;
		m_curMp = m_maxMp;
	}
}

random_device rd;
default_random_engine dre{ rd() };
void CPlayer::CreateItem(short x, short y)
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

void CPlayer::UseItem(int inven)
{
	if (true == m_items[inven]->GetEnable()) {
		bool use = false;
		switch (m_items[inven]->GetItemType()) {
		case ITEM_TYPE::HP_POTION:
			m_curHp += 50;
			if (m_curHp > m_maxHp)
				m_curHp = m_maxHp;
			use = true;
			Send_Chat_Packet(m_ID, "Used HP Potion");
			break;
		case ITEM_TYPE::MP_POTION:
			m_curMp += 30;
			if (m_curMp > m_maxMp)
				m_curMp = m_maxMp;
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

const ITEM_TYPE CPlayer::GetItemType(int index) const
{
	return m_items[index]->GetItemType();
}

void CPlayer::SetItem(int index, ITEM_TYPE type, int num, bool enable)
{
	m_items[index]->SetEnable(enable);
	m_items[index]->SetNum(num);
	m_items[index]->SetItemType(type);
}

bool CPlayer::CanUse(char skill, chrono::system_clock::time_point time)
{
	switch (skill) {
	case 0:
		if (chrono::duration_cast<chrono::seconds>(time - m_usedTime[0]).count() >= ATTACK_COOL) {
			m_usedTime[0] = time;
			return true;
		}
		break;
	case 1:
		if (chrono::duration_cast<chrono::seconds>(time - m_usedTime[1]).count() >= SKILL1_COOL &&
			m_curMp >= 15) {
			m_usedTime[1] = time;
			return true;
		}
		break;
	case 2:
		if (chrono::duration_cast<chrono::seconds>(time - m_usedTime[2]).count() >= SKILL2_COOL &&
			m_curMp >= 5) {
			m_usedTime[2] = time;
			return true;
		}
		break;
	case 3:
		if (chrono::duration_cast<chrono::seconds>(time - m_usedTime[3]).count() >= SKILL3_COOL &&
			m_curMp >= 10) {
			m_usedTime[3] = time;
			return true;
		}
		break;
	}
	return false;

}

void CPlayer::Send_LoginInfo_Packet()
{
	SC_LOGIN_INFO_PACKET p;
	p.id = m_ID;
	p.size = sizeof(SC_LOGIN_INFO_PACKET);
	p.type = SC_LOGIN_INFO;
	p.x = m_PosX;
	p.y = m_PosY;

	p.max_hp = m_maxHp;
	p.hp = m_curHp;
	p.max_mp = m_maxMp;
	p.mp = m_curMp;
	p.exp = m_exp;
	p.level = m_level;
	strcpy_s(p.name, m_Name);
	SendPacket(&p);
}

void CPlayer::Send_StatChange_Packet()
{
	SC_STAT_CHANGE_PACKET packet;

	packet.size = sizeof(packet);
	packet.type = SC_STAT_CHANGE;
	packet.max_hp = m_maxHp;
	packet.hp = m_curHp;
	packet.max_mp = m_maxMp;
	packet.mp = m_curMp;
	packet.level = m_level;
	packet.exp = m_exp;

	SendPacket(&packet);
}

void CPlayer::Send_Damage_Packet(int cid, int powerlv)
{
	SC_DAMAGE_PACKET p;

	p.size = sizeof(p);
	p.type = SC_DAMAGE;
	p.id = cid;
	p.hp = CNetworkMgr::GetInstance()->GetCObject(cid)->GetCurHp();
	
	SendPacket(&p);
}

void CPlayer::Send_ItemUsed_Packet(int inven)
{
	SC_ITEM_USED_PACKET p;

	p.size = sizeof(p);
	p.type = SC_ITEM_USED;
	p.inven_num = inven;

	SendPacket(&p);
}