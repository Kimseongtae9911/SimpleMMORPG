#include "pch.h"
#include "CNetworkMgr.h"
#include "CPacketMgr.h"
#include "CObject.h"
#include "CDatabase.h"

std::unique_ptr<CNetworkMgr> CNetworkMgr::m_instance;

bool CNetworkMgr::Initialize()
{
	m_iocpfunc.insert({ OP_TYPE::OP_ACCEPT, [this](int id, int bytes, OVER_EXP* over_ex) {Accept(id, bytes, over_ex); } });
	m_iocpfunc.insert({ OP_TYPE::OP_MONSTER_RESPAWN, [this](int id, int bytes, OVER_EXP* over_ex) {MonsterRespawn(id, bytes, over_ex); } });
	m_iocpfunc.insert({ OP_TYPE::OP_NPC_MOVE, [this](int id, int bytes, OVER_EXP* over_ex) {NpcMove(id, bytes, over_ex); } });
	m_iocpfunc.insert({ OP_TYPE::OP_PLAYER_HEAL, [this](int id, int bytes, OVER_EXP* over_ex) {PlayerHeal(id, bytes, over_ex); } });
	m_iocpfunc.insert({ OP_TYPE::OP_RECV, [this](int id, int bytes, OVER_EXP* over_ex) {Recv(id, bytes, over_ex); } });
	m_iocpfunc.insert({ OP_TYPE::OP_SEND, [this](int id, int bytes, OVER_EXP* over_ex) {Send(id, bytes, over_ex); } });

	m_over = new OVER_EXP;
	m_database = new CDatabase();
	m_database->Initialize();
	CPacketMgr::GetInstance()->Initialize();

	for (int i = 0; i < MAX_USER; ++i) {
		m_objects[i] = new CPlayer();
	}
	cout << "Initialize NPC Start" << endl;
	for (int i = MAX_USER; i < MAX_USER + MAX_NPC; ++i) {
		m_objects[i] = new CNpc(i);
		CNpc* npc = reinterpret_cast<CNpc*>(m_objects[i]);
		lua_getglobal(npc->GetLua(), "monster_initialize");
		lua_pushnumber(npc->GetLua(), npc->GetID());
		lua_pcall(npc->GetLua(), 1, 0, 0);
	}
	cout << "Initialize NPC Complete" << endl;


	for (int i = 0; i < MAX_USER; ++i) {
		m_socketpool.push(WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED));
	}

	WSADATA WSAData;
	WSAStartup(MAKEWORD(2, 2), &WSAData);
	m_listenSock = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	SOCKADDR_IN server_addr;
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(PORT_NUM);
	server_addr.sin_addr.S_un.S_addr = INADDR_ANY;
	bind(m_listenSock, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr));
	listen(m_listenSock, SOMAXCONN);
	SOCKADDR_IN cl_addr;
	int addr_size = sizeof(cl_addr);

	m_iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, 0);
	CreateIoCompletionPort(reinterpret_cast<HANDLE>(m_listenSock), m_iocp, 9999, 0);

	m_clientSock = m_socketpool.front();
	m_socketpool.pop();

	m_over->m_comp_type = OP_TYPE::OP_ACCEPT;
	AcceptEx(m_listenSock, m_clientSock, m_over->m_send_buf, 0, addr_size + 16, addr_size + 16, 0, &m_over->m_over);

    return false;
}

bool CNetworkMgr::Release()
{
	WSACleanup();
    return false;
}

void CNetworkMgr::WorkerFunc()
{
	while (true) {
		DWORD num_bytes;
		ULONG_PTR key;
		WSAOVERLAPPED* over = nullptr;
		BOOL ret = GetQueuedCompletionStatus(m_iocp, &num_bytes, &key, &over, INFINITE);
		OVER_EXP* ex_over = reinterpret_cast<OVER_EXP*>(over);
		if (FALSE == ret) {
			if (ex_over->m_comp_type == OP_TYPE::OP_ACCEPT) cout << "Accept Error";
			else {
				cout << "GQCS Error on client[" << key << "]\n";
				Disconnect(static_cast<int>(key));
				if (ex_over->m_comp_type == OP_TYPE::OP_SEND) delete ex_over;
				continue;
			}
		}

		if ((0 == num_bytes) && ((ex_over->m_comp_type == OP_TYPE::OP_RECV) || (ex_over->m_comp_type == OP_TYPE::OP_SEND))) {
			Disconnect(static_cast<int>(key));
			if (ex_over->m_comp_type == OP_TYPE::OP_SEND) delete ex_over;
			continue;
		}

		auto iter = m_iocpfunc.find(ex_over->m_comp_type);
		if (iter != m_iocpfunc.end()) {
			iter->second(static_cast<int>(key), num_bytes, ex_over);
		}
		else
			cout << "Wrong IOCP Operation" << endl;
	}
}

void CNetworkMgr::TimerFunc()
{
	while (true) {
		TIMER_EVENT ev;
		auto current_time = chrono::system_clock::now();
		if (true == m_timerQueue.try_pop(ev)) {
			if (ev.wakeup_time > current_time) {
				m_timerQueue.push(ev);
				this_thread::sleep_for(1ms);
				continue;
			}
			switch (ev.event_id) {
			case EV_RANDOM_MOVE:
			case EV_CHASE_PLAYER:
			case EV_ATTACK_PLAYER: {
				OVER_EXP* ov = new OVER_EXP;
				ov->m_comp_type = OP_TYPE::OP_NPC_MOVE;
				PostQueuedCompletionStatus(m_iocp, 1, ev.obj_id, &ov->m_over);
				break;
			}
			case EV_PLAYER_HEAL: {
				OVER_EXP* ov = new OVER_EXP;
				ov->m_comp_type = OP_TYPE::OP_PLAYER_HEAL;
				PostQueuedCompletionStatus(m_iocp, 1, ev.obj_id, &ov->m_over);
				break;
			}
			case EV_MONSTER_RESPAWN: {
				OVER_EXP* ov = new OVER_EXP;
				ov->m_comp_type = OP_TYPE::OP_MONSTER_RESPAWN;
				PostQueuedCompletionStatus(m_iocp, 1, ev.obj_id, &ov->m_over);
				break;
			}
			}
			continue;
		}
		this_thread::sleep_for(1ms);
	}
}

void CNetworkMgr::Accept(int id, int bytes, OVER_EXP* over_ex)
{
	int client_id = GetNewID();
	if (client_id != -1) {
		{
			lock_guard<mutex> ll(m_objects[client_id]->m_StateLock);
			m_objects[client_id]->SetState(CL_STATE::ST_ALLOC);
		}
		reinterpret_cast<CPlayer*>(m_objects[client_id])->PlayerAccept(client_id, m_clientSock);

		m_clientSock = m_socketpool.front();
		m_socketpool.pop();
	}
	else {
		cout << "Max user exceeded.\n";
	}
	ZeroMemory(&m_over->m_over, sizeof(m_over->m_over));
	int addr_size = sizeof(SOCKADDR_IN);
	AcceptEx(m_listenSock, m_clientSock, m_over->m_send_buf, 0, addr_size + 16, addr_size + 16, 0, &m_over->m_over);
}

void CNetworkMgr::MonsterRespawn(int id, int bytes, OVER_EXP* over_ex)
{
	CNpc* npc = reinterpret_cast<CNpc*>(m_objects[id]);
	npc->SetDie(false);
	npc->SetCurHp(npc->GetMaxHp());
	npc->SetPos(npc->GetRespawnX(), npc->GetRespawnY());
	for (int i = 0; i < MAX_USER; ++i) {
		if (m_objects[i]->GetState() != CL_STATE::ST_INGAME)
			continue;
		if (m_objects[i]->CanSee(npc->GetID()))
			m_objects[i]->Send_AddObject_Packet(npc->GetID());
	}
	TIMER_EVENT ev{ id, chrono::system_clock::now() + 1s, EV_RANDOM_MOVE, 0 };
	m_timerQueue.push(ev);
}

void CNetworkMgr::PlayerHeal(int id, int bytes, OVER_EXP* over_ex)
{
	CPlayer* player = reinterpret_cast<CPlayer*>(m_objects[id]);
	bool heal = false;
	if (player->GetCurHp() < player->GetMaxHp()) {
		player->SetCurHp(player->GetCurHp() + static_cast<int>(player->GetMaxHp() * 0.1f));
		if (player->GetCurHp() > player->GetMaxHp()) {
			player->SetCurHp(player->GetMaxHp());
		}
		heal = true;
	}
	if (player->GetMp() < player->GetMaxMp()) {
		player->SetMp(player->GetMp() + static_cast<int>(player->GetMaxMp() * 0.1f));
		if (player->GetMp() > player->GetMaxMp()) {
			player->SetMp(player->GetMaxMp());
		}
		heal = true;
	}

	if (true == heal)
		player->Send_StatChange_Packet();
	if (player->GetCurHp() < player->GetMaxHp()) {
		TIMER_EVENT ev{ id, chrono::system_clock::now() + 5s, EV_PLAYER_HEAL, 0 };
		m_timerQueue.push(ev);
	}
	else {
		player->SetHeal(false);
	}
}

void CNetworkMgr::NpcMove(int id, int bytes, OVER_EXP* over_ex)
{
	if (true == reinterpret_cast<CNpc*>(m_objects[id])->GetDie()) {
		delete over_ex;
		return;
	}

	bool keep_alive = false;
	bool chase = false;
	bool attack = false;

	for (int j = 0; j < MAX_USER; ++j) {
		if (m_objects[id]->CanSee(j)) {
			keep_alive = true;
			if (MONSTER_TYPE::AGRO == reinterpret_cast<CNpc*>(m_objects[id])->GetMonType() && reinterpret_cast<CNpc*>(m_objects[id])->Agro(j)) {
				chase = true;
				if (abs(m_objects[j]->GetPosX() - m_objects[static_cast<int>(id)]->GetPosX()) + abs(m_objects[j]->GetPosY() - m_objects[static_cast<int>(id)]->GetPosY()) <= 1) {
					attack = true;
				}
			}
			else {
				// Peace 몬스터 처리
			}
			break;
		}
	}
	if (true == keep_alive) {
		CNpc* npc = reinterpret_cast<CNpc*>(m_objects[id]);
		if (true == attack) {
			npc->Attack();
			TIMER_EVENT ev{ id, chrono::system_clock::now() + 1s, EV_ATTACK_PLAYER, npc->GetTarget() };
			m_timerQueue.push(ev);
		}
		else if (true == chase) {
			npc->SetChase(true);
			npc->Chase();
			TIMER_EVENT ev{ id, chrono::system_clock::now() + 1s, EV_CHASE_PLAYER, npc->GetTarget() };
			m_timerQueue.push(ev);
		}
		else {
			npc->SetChase(false);
			npc->RandomMove();
			TIMER_EVENT ev{ id, chrono::system_clock::now() + 1s, EV_RANDOM_MOVE, 0 };
			m_timerQueue.push(ev);
		}
	}
	else {
		dynamic_cast<CNpc*>(m_objects[id])->m_active = false;
	}
	delete over_ex;
}

void CNetworkMgr::Recv(int id, int bytes, OVER_EXP* over_ex)
{
	int remain_data = bytes + m_objects[id]->GetRemainBuf();
	char* packet = over_ex->m_send_buf;

	while (remain_data > 0) {
		BASE_PACKET* p = reinterpret_cast<BASE_PACKET*>(packet);

		if (p->size <= remain_data) {
			CPacketMgr::GetInstance()->PacketProcess(p, reinterpret_cast<CPlayer*>(m_objects[id]));
			packet += p->size;
			remain_data -= p->size;
		}
		else break;
	}
	m_objects[id]->SetRemainBuf(remain_data);
	if (remain_data > 0) {
		memcpy(over_ex->m_send_buf, packet, remain_data);
	}
	m_objects[id]->RecvPacket();
}

void CNetworkMgr::Send(int id, int bytes, OVER_EXP* over_ex)
{
	delete over_ex;
}

int CNetworkMgr::GetNewID()
{
	for (int i = 0; i < MAX_USER; ++i) {
		lock_guard <mutex> ll{ m_objects[i]->m_StateLock };
		if (m_objects[i]->GetState() == CL_STATE::ST_FREE)
			return i;
	}
	return -1;
}

void CNetworkMgr::Disconnect(int id)
{
	CPlayer* pc = reinterpret_cast<CPlayer*>(m_objects[id]);
	USER_INFO ui;
	memcpy(ui.name, pc->GetName(), NAME_SIZE);
	ui.pos_x = pc->GetPosX();
	ui.pos_y = pc->GetPosY();
	ui.cur_hp = pc->GetCurHp();
	ui.max_hp = pc->GetMaxHp();
	ui.exp = pc->GetExp();
	ui.cur_mp = pc->GetMp();
	ui.max_mp = pc->GetMaxMp();
	ui.level = pc->GetLevel();
	ui.item1 = pc->GetItemType(0) - 1;
	ui.item2 = pc->GetItemType(1) - 1;
	ui.item3 = pc->GetItemType(2) - 1;
	ui.item4 = pc->GetItemType(3) - 1;
	ui.item5 = pc->GetItemType(4) - 1;
	ui.item6 = pc->GetItemType(5) - 1;
	ui.moneycnt = 78;
	m_database->SavePlayerInfo(ui);

	pc->m_ViewLock.lock();
	unordered_set <int> vl = pc->GetViewList();
	pc->m_ViewLock.unlock();
	for (auto& p_id : vl) {
		if (id >= MAX_USER)
			continue;
		auto& pl = m_objects[p_id];
		{
			lock_guard<mutex> ll(pl->m_StateLock);
			if (CL_STATE::ST_INGAME != pl->GetState())
				continue;
		}
		if (pl->GetID() == id)
			continue;
		pl->Send_RemoveObject_Packet(id);
	}
	closesocket(m_objects[id]->GetSocket());

	lock_guard<mutex> ll(m_objects[id]->m_StateLock);
	m_objects[id]->SetState(CL_STATE::ST_FREE);
}
