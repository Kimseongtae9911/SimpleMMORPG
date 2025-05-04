#include "pch.h"
#include "CNetworkMgr.h"
#include "CPacketMgr.h"
#include "GameUtil.h"
#include "CClient.h"
#include "CNpc.h"
#include "CDatabase.h"
#include "COverlapEx.h"
#include "OverlapPool.h"
#include "Global.h"

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

		for (int i = 0; i < MAX_USER * 2; ++i) {
			OverlapPool::RegisterToPool(new COverlapEx());
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

void CNetworkMgr::IOCPFunc()
{
	while (true) {
		DWORD num_bytes;
		ULONG_PTR key;
		WSAOVERLAPPED* over = nullptr;
		BOOL ret = GetQueuedCompletionStatus(m_iocp, &num_bytes, &key, &over, INFINITE);
		COverlapEx* over_ex = reinterpret_cast<COverlapEx*>(over);
		if (FALSE == ret) {
			if (over_ex->m_comp_type == OP_TYPE::OP_ACCEPT) cout << "Accept Error";
			else {
				cout << "GQCS Error on client[" << key << "]\n";
				Disconnect(static_cast<int>(key));
				if (over_ex->m_comp_type == OP_TYPE::OP_SEND) OverlapPool::RegisterToPool(over_ex);
				continue;
			}
		}

		if ((0 == num_bytes) && ((over_ex->m_comp_type == OP_TYPE::OP_RECV) || (over_ex->m_comp_type == OP_TYPE::OP_SEND))) {
			Disconnect(static_cast<int>(key));
			if (over_ex->m_comp_type == OP_TYPE::OP_SEND) OverlapPool::RegisterToPool(over_ex);
			continue;
		}

		auto iter = m_iocpfunc.find(over_ex->m_comp_type);
		if (iter != m_iocpfunc.end()) {
			iter->second(static_cast<int>(key), num_bytes, over_ex);
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
				this_thread::sleep_for(1s);
				continue;
			}
			switch (ev.event_id) {
			case EV_RANDOM_MOVE:
			case EV_CHASE_PLAYER:
			case EV_ATTACK_PLAYER: {
				COverlapEx* ov = OverlapPool::GetOverlapFromPool();
				ov->m_comp_type = OP_TYPE::OP_NPC_MOVE;
				PostQueuedCompletionStatus(m_iocp, 1, ev.obj_id, &ov->m_over);
				break;
			}
			case EV_PLAYER_HEAL: {
				COverlapEx* ov = OverlapPool::GetOverlapFromPool();
				ov->m_comp_type = OP_TYPE::OP_PLAYER_HEAL;
				PostQueuedCompletionStatus(m_iocp, 1, ev.obj_id, &ov->m_over);
				break;
			}
			case EV_MONSTER_RESPAWN: {
				COverlapEx* ov = OverlapPool::GetOverlapFromPool();
				ov->m_comp_type = OP_TYPE::OP_MONSTER_RESPAWN;
				PostQueuedCompletionStatus(m_iocp, 1, ev.obj_id, &ov->m_over);
				break;
			}
			case EV_POWERUP_ROLLBACK: {
				COverlapEx* ov = OverlapPool::GetOverlapFromPool();
				ov->m_comp_type = OP_TYPE::OP_POWERUP_ROLLBACK;
				ov->m_ai_target_obj = ev.target_id;
				PostQueuedCompletionStatus(m_iocp, 1, ev.obj_id, &ov->m_over);
				break;
			}
			}
			continue;
		}
		this_thread::sleep_for(1s);
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
	CClient* client = static_cast<CClient*>(m_objects[id]);
	client->GetJobQueue().PushJob([client]() {
		client->Heal();
		});
}

void CNetworkMgr::NpcMove(int id, int bytes, COverlapEx* over_ex)
{
	m_objects[id]->Update();
	OverlapPool::RegisterToPool(over_ex);
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

	bool needProcess = false;
	auto& recvBuf = client->GetSession()->GetRecvPacketBuf();

	while (remain_data > 0) {
		BASE_PACKET* p = reinterpret_cast<BASE_PACKET*>(packet);

		if (p->size <= remain_data) {
			if (recvBuf.size() < p->size)
				recvBuf.resize(p->size);

			memcpy(recvBuf.data(), p, p->size);

			client->GetJobQueue().PushJob([client, p = std::move(recvBuf)]() {
				CPacketMgr::GetInstance()->PacketProcess(reinterpret_cast<BASE_PACKET*>(const_cast<char*>(p.data())), client);
				});

			packet += p->size;
			remain_data -= p->size;
			needProcess = true;
		}
		else break;
	}
	client->GetSession()->SetRemainBuf(remain_data);
	if (remain_data > 0) {
		memcpy(over_ex->m_send_buf, packet, remain_data);
	}
	client->GetSession()->RecvPacket();

	if (needProcess)
		GPacketJobQueue->AddSessionQueue(client);
}

void CNetworkMgr::Send(int id, int bytes, COverlapEx* over_ex)
{
	OverlapPool::RegisterToPool(over_ex);
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
	pc->GetJobQueue().PushJob([pc]() {
		pc->Logout();
		});
}
