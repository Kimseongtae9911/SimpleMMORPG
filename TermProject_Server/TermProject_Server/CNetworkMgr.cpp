#include "pch.h"
#include "CNetworkMgr.h"
#include "CPacketMgr.h"
#include "GameUtil.h"
#include "CClient.h"
#include "CNpc.h"
#include "CDatabase.h"
#include "COverlapEx.h"

std::unique_ptr<CNetworkMgr> CNetworkMgr::m_instance;

bool CNetworkMgr::Initialize()
{
	try {
		WSADATA WSAData;
		WSAStartup(MAKEWORD(2, 2), &WSAData);

		m_iocpfunc.insert({ OP_TYPE::OP_ACCEPT, [this](int id, int bytes, COverlapEx* over_ex) {Accept(id, bytes, over_ex); } });
		m_iocpfunc.insert({ OP_TYPE::OP_MONSTER_RESPAWN, [this](int id, int bytes, COverlapEx* over_ex) {MonsterRespawn(id, bytes, over_ex); } });
		m_iocpfunc.insert({ OP_TYPE::OP_NPC_MOVE, [this](int id, int bytes, COverlapEx* over_ex) {NpcMove(id, bytes, over_ex); } });
		m_iocpfunc.insert({ OP_TYPE::OP_PLAYER_HEAL, [this](int id, int bytes, COverlapEx* over_ex) {PlayerHeal(id, bytes, over_ex); } });
		m_iocpfunc.insert({ OP_TYPE::OP_RECV, [this](int id, int bytes, COverlapEx* over_ex) {Recv(id, bytes, over_ex); } });
		m_iocpfunc.insert({ OP_TYPE::OP_SEND, [this](int id, int bytes, COverlapEx* over_ex) {Send(id, bytes, over_ex); } });

		m_over = new COverlapEx;
#ifdef DATABASE
		m_database = new CDatabase();
		m_database->Initialize();
#else
		cout << "Execute without database" << endl;
#endif
		CPacketMgr::GetInstance()->Initialize();
		GameUtil::InitailzeData();

		for (int i = 0; i < MAX_USER; ++i) {
			m_objects[i] = new CClient();
		}
		cout << "Initialize NPC Start" << endl;
		for (int i = MAX_USER; i < MAX_USER + MAX_NPC; ++i) {
			m_objects[i] = new CNpc(i);
			CNpc* npc = reinterpret_cast<CNpc*>(m_objects[i]);
			lua_getglobal(npc->GetLua(), "monster_initialize");
			lua_pushnumber(npc->GetLua(), i);
			lua_pcall(npc->GetLua(), 1, 0, 0);
		}
		cout << "Initialize NPC Complete" << endl;


		for (int i = 0; i < MAX_USER; ++i) {
			SOCKET s = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
			if (s == INVALID_SOCKET) {
				cout << i << ", Failed to Create Socket" << endl;
			}
			m_socketpool.push(s);
			m_idMap.insert({s, i});
		}

		m_listenSock = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);

		m_iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, 0);
		if (m_iocp == NULL) {
			cout << "Failed to make HANDLE" << endl;
		}
		CreateIoCompletionPort(reinterpret_cast<HANDLE>(m_listenSock), m_iocp, 9999, 0);

		SOCKADDR_IN server_addr;
		memset(&server_addr, 0, sizeof(server_addr));
		server_addr.sin_family = AF_INET;
		server_addr.sin_port = htons(PORT_NUM);
		server_addr.sin_addr.S_un.S_addr = INADDR_ANY;
		int ret = ::bind(m_listenSock, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr));
		if (ret != 0) {
			cout << "Bind Error" << endl;
		}
		ret = listen(m_listenSock, SOMAXCONN);
		if (ret != 0) {
			cout << "Listen Error" << endl;
		}

		m_clientSock = m_socketpool.front();
		m_socketpool.pop();

		SOCKADDR_IN cl_addr;
		int addr_size = sizeof(cl_addr);

		m_over->m_comp_type = OP_TYPE::OP_ACCEPT;
		AcceptEx(m_listenSock, m_clientSock, m_over->m_send_buf, 0, addr_size + 16, addr_size + 16, 0, &m_over->m_over);
		
		return true;
	}
	catch (exception ex) {
		cout << ex.what() << endl;
		return false;
	}
    
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
		COverlapEx* ex_over = reinterpret_cast<COverlapEx*>(over);
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
				this_thread::yield();
				continue;
			}
			switch (ev.event_id) {
			case EV_RANDOM_MOVE:
			case EV_CHASE_PLAYER:
			case EV_ATTACK_PLAYER: {
				COverlapEx* ov = new COverlapEx;
				ov->m_comp_type = OP_TYPE::OP_NPC_MOVE;
				PostQueuedCompletionStatus(m_iocp, 1, ev.obj_id, &ov->m_over);
				break;
			}
			case EV_PLAYER_HEAL: {
				COverlapEx* ov = new COverlapEx;
				ov->m_comp_type = OP_TYPE::OP_PLAYER_HEAL;
				PostQueuedCompletionStatus(m_iocp, 1, ev.obj_id, &ov->m_over);
				break;
			}
			case EV_MONSTER_RESPAWN: {
				COverlapEx* ov = new COverlapEx;
				ov->m_comp_type = OP_TYPE::OP_MONSTER_RESPAWN;
				PostQueuedCompletionStatus(m_iocp, 1, ev.obj_id, &ov->m_over);
				break;
			}
			case EV_POWERUP_ROLLBACK: {
				COverlapEx* ov = new COverlapEx;
				ov->m_comp_type = OP_TYPE::OP_POWERUP_ROLLBACK;
				ov->m_ai_target_obj = ev.target_id;
				PostQueuedCompletionStatus(m_iocp, 1, ev.obj_id, &ov->m_over);
				break;
			}
			}
			continue;
		}
		this_thread::yield();
	}
}

vector<int> CNetworkMgr::GetClientsCanSeeNpc(int npcID)
{
	vector<int> chatClients;
	for (auto& obj : CNetworkMgr::GetInstance()->GetAllObject()) {
		CClient* clObj = reinterpret_cast<CClient*>(obj);
		clObj->m_StateLock.lock();
		if (CL_STATE::ST_INGAME != clObj->GetState()) {
			clObj->m_StateLock.unlock();
			continue;
		}
		clObj->m_StateLock.unlock();
		if (obj->GetID() >= MAX_USER)
			break;
		if (obj->CanSee(npcID))
			chatClients.emplace_back(obj->GetID());
	}

	return chatClients;
}

void CNetworkMgr::Accept(int id, int bytes, COverlapEx* over_ex)
{
	if (m_clientSock == -1) {
		return;
	}
	int client_id = m_idMap.find(m_clientSock)->second;
	{
		lock_guard<mutex> ll(m_objects[client_id]->m_StateLock);
		reinterpret_cast<CClient*>(m_objects[client_id])->SetState(CL_STATE::ST_ALLOC);
	}
	CreateIoCompletionPort(reinterpret_cast<HANDLE>(m_clientSock), m_iocp, client_id, 0);
	reinterpret_cast<CClient*>(m_objects[client_id])->PlayerAccept(client_id, m_clientSock);

	if (!m_socketpool.empty()) {
		m_clientSock = m_socketpool.front();
		m_socketpool.pop();
	}
	else {
		cout << "Max user exceeded.\n";
		m_clientSock = -1;
	}

	ZeroMemory(&m_over->m_over, sizeof(m_over->m_over));
	int addr_size = sizeof(SOCKADDR_IN);
	AcceptEx(m_listenSock, m_clientSock, m_over->m_send_buf, 0, addr_size + 16, addr_size + 16, 0, &m_over->m_over);
}

void CNetworkMgr::MonsterRespawn(int id, int bytes, COverlapEx* over_ex)
{
	CNpc* npc = reinterpret_cast<CNpc*>(m_objects[id]);
	npc->SetDie(false);
	npc->GetStat()->SetCurHp(npc->GetStat()->GetMaxHp());
	npc->SetPos(npc->GetRespawnX(), npc->GetRespawnY());
	for (int i = 0; i < MAX_USER; ++i) {
		if (reinterpret_cast<CClient*>(m_objects[i])->GetState() != CL_STATE::ST_INGAME)
			continue;
		if (m_objects[i]->CanSee(npc->GetID()))
			reinterpret_cast<CClient*>(m_objects[i])->AddObjectToView(npc->GetID());
	}
	TIMER_EVENT ev{ id, chrono::system_clock::now() + 1s, EV_RANDOM_MOVE, 0 };
	m_timerQueue.push(ev);
}

void CNetworkMgr::PlayerHeal(int id, int bytes, COverlapEx* over_ex)
{
	reinterpret_cast<CClient*>(m_objects[id])->Heal();
}

void CNetworkMgr::NpcMove(int id, int bytes, COverlapEx* over_ex)
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

void CNetworkMgr::PowerUpRollBack(int id, int bytes, COverlapEx* over_ex)
{
	CClient* client = reinterpret_cast<CClient*>(m_objects[id]);
	m_objects[id]->GetStat()->SetPower(m_objects[id]->GetStat()->GetPower() - over_ex->m_ai_target_obj);
}

void CNetworkMgr::Recv(int id, int bytes, COverlapEx* over_ex)
{
	CClient* client = reinterpret_cast<CClient*>(m_objects[id]);

	int remain_data = bytes + client->GetSession()->GetRemainBuf();
	char* packet = over_ex->m_send_buf;

	while (remain_data > 0) {
		BASE_PACKET* p = reinterpret_cast<BASE_PACKET*>(packet);

		if (p->size <= remain_data) {
			CPacketMgr::GetInstance()->PacketProcess(p, client);
			packet += p->size;
			remain_data -= p->size;
		}
		else break;
	}
	client->GetSession()->SetRemainBuf(remain_data);
	if (remain_data > 0) {
		memcpy(over_ex->m_send_buf, packet, remain_data);
	}
	client->GetSession()->RecvPacket();
}

void CNetworkMgr::Send(int id, int bytes, COverlapEx* over_ex)
{
	delete over_ex;
}

int CNetworkMgr::GetNewID()
{
	for (int i = 0; i < MAX_USER; ++i) {
		lock_guard <mutex> ll{ m_objects[i]->m_StateLock };
		if (reinterpret_cast<CClient*>(m_objects[i])->GetState() == CL_STATE::ST_FREE)
			return i;
	}
	return -1;
}

void CNetworkMgr::Disconnect(int id)
{
	CClient* pc = reinterpret_cast<CClient*>(m_objects[id]);
	CClientStat* stat = reinterpret_cast<CClientStat*>(m_objects[id]->GetStat());
	USER_INFO ui;
	memcpy(ui.name, pc->GetName(), NAME_SIZE);
	ui.pos_x = pc->GetPosX();
	ui.pos_y = pc->GetPosY();
	ui.cur_hp = stat->GetCurHp();
	ui.max_hp = stat->GetMaxHp();
	ui.exp = stat->GetExp();
	ui.cur_mp = stat->GetMp();
	ui.max_mp = stat->GetMaxMp();
	ui.level = stat->GetLevel();
	ui.item1 = pc->GetItemType(0) - 1;
	ui.item2 = pc->GetItemType(1) - 1;
	ui.item3 = pc->GetItemType(2) - 1;
	ui.item4 = pc->GetItemType(3) - 1;
	ui.item5 = pc->GetItemType(4) - 1;
	ui.item6 = pc->GetItemType(5) - 1;
	ui.moneycnt = 78;

#ifdef DATABASE
	m_database->SavePlayerInfo(ui);
#endif

	pc->m_ViewLock.lock_shared();
	unordered_set <int> vl = pc->GetViewList();
	pc->m_ViewLock.unlock_shared();
	for (auto& p_id : vl) {
		if (id >= MAX_USER)
			continue;
		auto& pl = m_objects[p_id];
		{
			lock_guard<mutex> ll(pl->m_StateLock);
			if (CL_STATE::ST_INGAME != reinterpret_cast<CClient*>(pl)->GetState())
				continue;
		}
		if (pl->GetID() == id)
			continue;
		reinterpret_cast<CClient*>(pl)->RemoveObjectFromView(id);
	}
	closesocket(pc->GetSession()->GetSocket());

	lock_guard<mutex> ll(m_objects[id]->m_StateLock);
	pc->SetState(CL_STATE::ST_FREE);
}
