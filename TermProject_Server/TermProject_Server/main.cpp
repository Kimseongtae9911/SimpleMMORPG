#include "pch.h"
#include "CObject.h"
#include "CDatabase.h"
#include "CNetworkMgr.h"

concurrency::concurrent_priority_queue<TIMER_EVENT> timer_queue;

CDatabase* g_DB;
HANDLE h_iocp;
array<CObject*, MAX_USER + MAX_NPC> clients;
char g_tilemap[W_HEIGHT][W_WIDTH];
bool g_heal = false;

SOCKET g_s_socket, g_c_socket;
OVER_EXP g_a_over;

void InitializeData();
int Get_NewID();
void disconnect(int c_id);
bool CheckLoginFail(char* name);

bool Can_See(int from, int to);
bool Can_Move(short x, short y, char dir);
bool Can_Use(int id, char skill, chrono::system_clock::time_point time);
bool Agro(int from, int to);

void WakeUpNPC(int npc_id, int waker);
void do_npc_random_move(int npc_id);
void do_npc_chase(int npc_id);
void do_npc_attack(int npc_id);
bool AStar(int startX, int startY, int destX, int destY, int* x, int* y);

void process_packet(int c_id, char* packet);
void worker_thread(HANDLE h_iocp);
void timer_func();

int main()
{
	WSADATA WSAData;
	WSAStartup(MAKEWORD(2, 2), &WSAData);
	g_s_socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	SOCKADDR_IN server_addr;
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(PORT_NUM);
	server_addr.sin_addr.S_un.S_addr = INADDR_ANY;
	bind(g_s_socket, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr));
	listen(g_s_socket, SOMAXCONN);
	SOCKADDR_IN cl_addr;
	int addr_size = sizeof(cl_addr);
	
	InitializeData();

	h_iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, 0);
	CreateIoCompletionPort(reinterpret_cast<HANDLE>(g_s_socket), h_iocp, 9999, 0);
	g_c_socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);	
	g_a_over.m_comp_type = OP_TYPE::OP_ACCEPT;
	AcceptEx(g_s_socket, g_c_socket, g_a_over.m_send_buf, 0, addr_size + 16, addr_size + 16, 0, &g_a_over.m_over);

	vector <thread> worker_threads;
	int num_threads = std::thread::hardware_concurrency();
	for (int i = 0; i < num_threads - 1; ++i)
		worker_threads.emplace_back(worker_thread, h_iocp);
	thread timer_thread{ timer_func };
	timer_thread.join();
	for (auto& th : worker_threads)
		th.join();
	closesocket(g_s_socket);
	WSACleanup();
}

void worker_thread(HANDLE h_iocp)
{
	while (true) {
		DWORD num_bytes;
		ULONG_PTR key;
		WSAOVERLAPPED* over = nullptr;
		BOOL ret = GetQueuedCompletionStatus(h_iocp, &num_bytes, &key, &over, INFINITE);
		OVER_EXP* ex_over = reinterpret_cast<OVER_EXP*>(over);
		if (FALSE == ret) {
			if (ex_over->m_comp_type == OP_TYPE::OP_ACCEPT) cout << "Accept Error";
			else {
				cout << "GQCS Error on client[" << key << "]\n";
				disconnect(static_cast<int>(key));
				if (ex_over->m_comp_type == OP_TYPE::OP_SEND) delete ex_over;
				continue;
			}
		}

		if ((0 == num_bytes) && ((ex_over->m_comp_type == OP_TYPE::OP_RECV) || (ex_over->m_comp_type == OP_TYPE::OP_SEND))) {
			disconnect(static_cast<int>(key));
			if (ex_over->m_comp_type == OP_TYPE::OP_SEND) delete ex_over;
			continue;
		}

		switch (ex_over->m_comp_type) {
		case OP_TYPE::OP_ACCEPT: {
			int client_id = Get_NewID();
			if (client_id != -1) {
				{
					lock_guard<mutex> ll(clients[client_id]->m_StateLock);
					clients[client_id]->SetState(CL_STATE::ST_ALLOC);
				}
				dynamic_cast<CPlayer*>(clients[client_id])->PlayerAccept(client_id, g_c_socket);

				g_c_socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
			}
			else {
				cout << "Max user exceeded.\n";
			}
			ZeroMemory(&g_a_over.m_over, sizeof(g_a_over.m_over));
			int addr_size = sizeof(SOCKADDR_IN);
			AcceptEx(g_s_socket, g_c_socket, g_a_over.m_send_buf, 0, addr_size + 16, addr_size + 16, 0, &g_a_over.m_over);
			break;
		}
		case OP_TYPE::OP_RECV: {
			int remain_data = num_bytes + clients[key]->GetRemainBuf();
			char* p = ex_over->m_send_buf;
			while (remain_data > 0) {
				int packet_size = p[0];
				if (packet_size <= remain_data) {
					process_packet(static_cast<int>(key), p);
					p = p + packet_size;
					remain_data = remain_data - packet_size;
				}
				else break;
			}
			clients[key]->SetRemainBuf(remain_data);
			if (remain_data > 0) {
				memcpy(ex_over->m_send_buf, p, remain_data);
			}
			clients[key]->RecvPacket();
			break;
		}
		case OP_TYPE::OP_SEND:
			delete ex_over;
			break;
		case OP_TYPE::OP_NPC_MOVE: {
			if (true == dynamic_cast<CNpc*>(clients[static_cast<int>(key)])->GetDie())
				continue;
			bool keep_alive = false;
			bool chase = false;
			bool attack = false;
			
			for (int j = 0; j < MAX_USER; ++j) {
				if (Can_See(static_cast<int>(key), j)) {
					keep_alive = true;
					if (MONSTER_TYPE::AGRO == dynamic_cast<CNpc*>(clients[static_cast<int>(key)])->GetMonType() && Agro(static_cast<int>(key), j)) {
						chase = true;
						if (abs(clients[j]->GetPosX() - clients[static_cast<int>(key)]->GetPosX()) + abs(clients[j]->GetPosY() - clients[static_cast<int>(key)]->GetPosY()) <= 1) {
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
				if (true == attack) {
					do_npc_attack(static_cast<int>(key));
					TIMER_EVENT ev{ static_cast<int>(key), chrono::system_clock::now() + 1s, EV_ATTACK_PLAYER, dynamic_cast<CNpc*>(clients[static_cast<int>(key)])->GetTarget() };
					timer_queue.push(ev);
				}
				else if (true == chase) {
					dynamic_cast<CNpc*>(clients[static_cast<int>(key)])->SetChase(true);
					do_npc_chase(static_cast<int>(key));
					TIMER_EVENT ev{ static_cast<int>(key), chrono::system_clock::now() + 1s, EV_CHASE_PLAYER, dynamic_cast<CNpc*>(clients[static_cast<int>(key)])->GetTarget() };
					timer_queue.push(ev);
				}
				else {
					dynamic_cast<CNpc*>(clients[static_cast<int>(key)])->SetChase(false);
					do_npc_random_move(static_cast<int>(key));
					TIMER_EVENT ev{ static_cast<int>(key), chrono::system_clock::now() + 1s, EV_RANDOM_MOVE, 0 };
					timer_queue.push(ev);
				}
			}
			else {
				dynamic_cast<CNpc*>(clients[static_cast<int>(key)])->m_active = false;
			}
			delete ex_over;
		}
			break;
		case OP_TYPE::OP_PLAYER_HEAL: {
			CPlayer* player = dynamic_cast<CPlayer*>(clients[static_cast<int>(key)]);
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

			if(true == heal)
				player->Send_StatChange_Packet();
			if (player->GetCurHp() < player->GetMaxHp()) {
				TIMER_EVENT ev{ static_cast<int>(key), chrono::system_clock::now() + 5s, EV_PLAYER_HEAL, 0};
				timer_queue.push(ev);
			}
			else {
				g_heal = false;
			}
			break;
		}
		case OP_TYPE::OP_MONSTER_RESPAWN: {
			CNpc* npc = dynamic_cast<CNpc*>(clients[static_cast<int>(static_cast<int>(key))]);
			npc->SetDie(false);
			npc->SetCurHp(npc->GetMaxHp());
			npc->SetPos(npc->GetRespawnX(), npc->GetRespawnY());
			for (int i = 0; i < MAX_USER; ++i) {
				if (clients[i]->GetState() != CL_STATE::ST_INGAME)
					continue;
				if(Can_See(i, npc->GetID()))
					clients[i]->Send_AddObject_Packet(npc->GetID());
			}
			TIMER_EVENT ev{ static_cast<int>(key), chrono::system_clock::now() + 1s, EV_RANDOM_MOVE, 0 };
			timer_queue.push(ev);
			break;
		}
		}
	}
}

void timer_func()
{
	while (true) {
		TIMER_EVENT ev;
		auto current_time = chrono::system_clock::now();
		if (true == timer_queue.try_pop(ev)) {
			if (ev.wakeup_time > current_time) {
				timer_queue.push(ev);
				this_thread::sleep_for(1ms);
				continue;
			}
			switch (ev.event_id) {
			case EV_RANDOM_MOVE:
			case EV_CHASE_PLAYER:
			case EV_ATTACK_PLAYER: {
				OVER_EXP* ov = new OVER_EXP;
				ov->m_comp_type = OP_TYPE::OP_NPC_MOVE;
				PostQueuedCompletionStatus(h_iocp, 1, ev.obj_id, &ov->m_over);
				break;
			}
			case EV_PLAYER_HEAL: {
				OVER_EXP* ov = new OVER_EXP;
				ov->m_comp_type = OP_TYPE::OP_PLAYER_HEAL;
				PostQueuedCompletionStatus(h_iocp, 1, ev.obj_id, &ov->m_over);
				break;
			}
			case EV_MONSTER_RESPAWN: {
				OVER_EXP* ov = new OVER_EXP;
				ov->m_comp_type = OP_TYPE::OP_MONSTER_RESPAWN;
				PostQueuedCompletionStatus(h_iocp, 1, ev.obj_id, &ov->m_over);
				break;
			}
			}
			continue;
		}
		this_thread::sleep_for(1ms);
	}
	/*while (true) {
		TIMER_EVENT ev;
		auto current_time = chrono::system_clock::now();
		auto temp_queue = timer_queue;
		if (true == temp_queue.try_pop(ev)) {
			if (ev.wakeup_time > current_time) {
				this_thread::sleep_for(1ms);
				continue;
			}
			switch (ev.event_id) {
			case EV_RANDOM_MOVE:
			case EV_CHASE_PLAYER:
			case EV_ATTACK_PLAYER: {
				OVER_EXP* ov = new OVER_EXP;
				ov->m_comp_type = OP_TYPE::OP_NPC_MOVE;
				PostQueuedCompletionStatus(h_iocp, 1, ev.obj_id, &ov->m_over);
				break;
			}
			case EV_PLAYER_HEAL: {
				OVER_EXP* ov = new OVER_EXP;
				ov->m_comp_type = OP_TYPE::OP_PLAYER_HEAL;
				PostQueuedCompletionStatus(h_iocp, 1, ev.obj_id, &ov->m_over);
				break;
			}
			}
			timer_queue.try_pop(ev);
			continue;
		}
		this_thread::sleep_for(1ms);
	}*/
}

void InitializeData()
{
	g_DB = new CDatabase();
	g_DB->Initialize();

	cout << "Uploading Map Data" << endl;
	fstream in("Data/Map.txt");
	for (int i = 0; i < 100; ++i) {
		for (int j = 0; j < 100; ++j) {
			char c;
			in >> c;
			g_tilemap[i][j] = c;
		}
	}
	for (int k = 0; k < 100; ++k) {
		for (int i = 1; i < 20; ++i) {
			for (int j = 0; j < 100; ++j) {
				g_tilemap[k][i * 100 + j] = g_tilemap[k][j];
			}
		}
	}
	for (int k = 100; k < W_HEIGHT; ++k) {
		for (int i = 0; i < 20; ++i) {
			for (int j = 0; j < 100; ++j) {
				g_tilemap[k][i * 100 + j] = g_tilemap[k - 100][j];
			}
		}
	}
	cout << "Map Upload Complete" << endl;

	for (int i = 0; i < MAX_USER; ++i) {
		clients[i] = new CPlayer();
	}
	cout << "Initialize NPC Start" << endl;
	for (int i = MAX_USER; i < MAX_USER + MAX_NPC; ++i) {
		clients[i] = new CNpc(i);
		lua_getglobal(dynamic_cast<CNpc*>(clients[i])->GetLua(), "monster_initialize");
		lua_pushnumber(dynamic_cast<CNpc*>(clients[i])->GetLua(), dynamic_cast<CNpc*>(clients[i])->GetID());
		lua_pcall(dynamic_cast<CNpc*>(clients[i])->GetLua(), 1, 0, 0);
	}
	cout << "Initialize NPC Complete" << endl;
}

int Get_NewID()
{
	for (int i = 0; i < MAX_USER; ++i) {
		lock_guard <mutex> ll{ clients[i]->m_StateLock };
		if (clients[i]->GetState() == CL_STATE::ST_FREE)
			return i;
	}
	return -1;
}

bool CheckLoginFail(char* name)
{
	if (!strcmp("Stress_Test", name))
		return false;
	for (int i = 0; i < MAX_USER; ++i) {
		clients[i]->m_StateLock.lock();
		if (clients[i]->GetState() == CL_STATE::ST_INGAME) {
			clients[i]->m_StateLock.unlock();
			if (!strcmp(clients[i]->GetName(), name)) {
				clients[i]->Send_LoginFail_Packet();
				return true;
			}
		}
		else {
			clients[i]->m_StateLock.unlock();
		}
	}

	return false;
}

void disconnect(int c_id)
{
	CPlayer* pc = dynamic_cast<CPlayer*>(clients[c_id]);
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
	g_DB->SavePlayerInfo(ui);

	clients[c_id]->m_ViewLock.lock();
	unordered_set <int> vl = clients[c_id]->GetViewList();
	clients[c_id]->m_ViewLock.unlock();
	for (auto& p_id : vl) {
		if (c_id >= MAX_USER)
			continue;
		auto& pl = clients[p_id];
		{
			lock_guard<mutex> ll(pl->m_StateLock);
			if (CL_STATE::ST_INGAME != pl->GetState())
				continue;
		}
		if (pl->GetID() == c_id)
			continue;
		pl->Send_RemoveObject_Packet(c_id);
	}
	closesocket(clients[c_id]->GetSocket());

	lock_guard<mutex> ll(clients[c_id]->m_StateLock);
	clients[c_id]->SetState(CL_STATE::ST_FREE);
}

bool Can_Move(short x, short y, char dir)
{
	bool canmove = true;
	switch (dir) {
		case 0: 
			if (y > 0) {
				y--;
				if (g_tilemap[y][x] != 'G' && g_tilemap[y][x] != 'D')
					canmove = false;
			}
			else {
				canmove = false;
			}
			break;
		case 1: 
			if (y < W_HEIGHT - 1) {
				y++;
				if (g_tilemap[y][x] != 'G' && g_tilemap[y][x] != 'D')
					canmove = false;
			}
			else {
				canmove = false;
			}
			break;
		case 2: 
			if (x > 0) {
				x--;
				if (g_tilemap[y][x] != 'G' && g_tilemap[y][x] != 'D')
					canmove = false;
			}
			else {
				canmove = false;
			}
			break;
		case 3: 
			if (x < W_WIDTH - 1) {
				x++;
				if (g_tilemap[y][x] != 'G' && g_tilemap[y][x] != 'D')
					canmove = false;
			}
			else {
				canmove = false;
			}
			break;
	}

	return canmove;
}

bool Can_See(int from, int to)
{
	if (abs(clients[from]->GetPosX() - clients[to]->GetPosX()) > VIEW_RANGE)
		return false;

	return abs(clients[from]->GetPosY() - clients[to]->GetPosY()) <= VIEW_RANGE;
}

bool Can_Use(int id, char skill, chrono::system_clock::time_point time)
{
	switch (skill) {
	case 0:
		if (chrono::duration_cast<chrono::seconds>(time - dynamic_cast<CPlayer*>(clients[id])->GetUsedTime(0)).count() >= ATTACK_COOL) {
			dynamic_cast<CPlayer*>(clients[id])->SetUsedTime(0, time);
			return true;
		}
		break;
	case 1:
		if (chrono::duration_cast<chrono::seconds>(time - dynamic_cast<CPlayer*>(clients[id])->GetUsedTime(1)).count() >= SKILL1_COOL &&
			dynamic_cast<CPlayer*>(clients[id])->GetMp() >= 15) {
			dynamic_cast<CPlayer*>(clients[id])->SetUsedTime(1, time);
			return true;
		}
		break;
	case 2:
		if (chrono::duration_cast<chrono::seconds>(time - dynamic_cast<CPlayer*>(clients[id])->GetUsedTime(2)).count() >= SKILL2_COOL &&
			dynamic_cast<CPlayer*>(clients[id])->GetMp() >= 5) {
			dynamic_cast<CPlayer*>(clients[id])->SetUsedTime(2, time);
			return true;
		}
		break;
	case 3:
		if (chrono::duration_cast<chrono::seconds>(time - dynamic_cast<CPlayer*>(clients[id])->GetUsedTime(3)).count() >= SKILL3_COOL &&
			dynamic_cast<CPlayer*>(clients[id])->GetMp() >= 10) {
			dynamic_cast<CPlayer*>(clients[id])->SetUsedTime(3, time);
			return true;
		}
		break;
	}
	return false;
}

bool Agro(int from, int to) {
	if (abs(clients[from]->GetPosX() - clients[to]->GetPosX()) > MONSTER_VIEW)
		return false;

	return abs(clients[from]->GetPosY() - clients[to]->GetPosY()) <= MONSTER_VIEW;
}

void WakeUpNPC(int npc_id, int waker)
{
	//OVER_EXP* exover = new OVER_EXP;
	//exover->m_comp_type = OP_TYPE::OP_AI_HELLO;
	//exover->m_ai_target_obj = waker;
	//PostQueuedCompletionStatus(h_iocp, 1, npc_id, &exover->m_over);

	if (dynamic_cast<CNpc*>(clients[npc_id])->m_active)
		return;
	bool old_state = false;
	if (false == atomic_compare_exchange_strong(&dynamic_cast<CNpc*>(clients[npc_id])->m_active, &old_state, true))
		return;
	TIMER_EVENT ev{ npc_id, chrono::system_clock::now(), EV_RANDOM_MOVE, 0 };
	timer_queue.push(ev);
}

void do_npc_random_move(int npc_id)
{
	CObject* npc = clients[npc_id];

	// 해당 NPC가 시야에 있는 플레이어들
	unordered_set<int> old_vl;
	for (auto& obj : clients) {
		if (CL_STATE::ST_INGAME != obj->GetState()) 
			continue;
		if (obj->GetID() >= MAX_USER)
			continue;
		if (true == Can_See(npc->GetID(), obj->GetID()))
			old_vl.insert(obj->GetID());
	}

	int x = npc->GetPosX();
	int y = npc->GetPosY();
	int dir = rand() % 4;
	if (Can_Move(x, y, dir)) {
		switch (dir) {
		case 0:	y--; break;
		case 1:	y++; break;
		case 2:	x--; break;
		case 3:	x++; break;
		}
		npc->SetPos(x, y);
	}

	// 움직인 이후에 해당 NPC가 시야에 있는 플레이어들
	unordered_set<int> new_vl;
	for (auto& obj : clients) {
		if (CL_STATE::ST_INGAME != obj->GetState()) 
			continue;
		if (obj->GetID() >= MAX_USER) 
			continue;
		if (true == Can_See(npc->GetID(), obj->GetID()))
			new_vl.insert(obj->GetID());
	}

	for (auto pl : new_vl) {
		// new_vl에는 있는데 old_vl에는 없을 때 플레이어의 시야에 등장
		if (0 == old_vl.count(pl)) {
			clients[pl]->Send_AddObject_Packet(npc->GetID());
		}
		else {
			// 플레이어가 계속 보고 있음.
			clients[pl]->Send_Move_Packet(npc->GetID());
		}
	}
	
	for (auto pl : old_vl) {
		if (0 == new_vl.count(pl)) {
			clients[pl]->m_ViewLock.lock();
			if (0 != clients[pl]->GetViewList().count(npc->GetID())) {
				clients[pl]->m_ViewLock.unlock();
				clients[pl]->Send_RemoveObject_Packet(npc->GetID());
			}
			else {
				clients[pl]->m_ViewLock.unlock();
			}
		}
	}
}

void do_npc_chase(int npc_id)
{
	CNpc* npc = dynamic_cast<CNpc*>(clients[npc_id]);

	// 해당 NPC가 시야에 있는 플레이어들
	unordered_set<int> old_vl;
	for (auto& obj : clients) {
		if (CL_STATE::ST_INGAME != obj->GetState())
			continue;
		if (obj->GetID() >= MAX_USER)
			continue;
		if (true == Can_See(npc->GetID(), obj->GetID()))
			old_vl.insert(obj->GetID());
	}
	
	int x = 0;
	int y = 0;
	if (AStar(npc->GetPosX(), npc->GetPosY(), clients[npc->GetTarget()]->GetPosX(), clients[npc->GetTarget()]->GetPosY(), &x, &y))
	{
		npc->SetPos(x, y);
	}
	else
		return;

	// 움직인 이후에 해당 NPC가 시야에 있는 플레이어들
	unordered_set<int> new_vl;
	for (auto& obj : clients) {
		if (CL_STATE::ST_INGAME != obj->GetState())
			continue;
		if (obj->GetID() >= MAX_USER)
			continue;
		if (true == Can_See(npc->GetID(), obj->GetID()))
			new_vl.insert(obj->GetID());
	}

	for (auto pl : new_vl) {
		// new_vl에는 있는데 old_vl에는 없을 때 플레이어의 시야에 등장
		if (0 == old_vl.count(pl)) {
			clients[pl]->Send_AddObject_Packet(npc->GetID());
		}
		else {
			// 플레이어가 계속 보고 있음.
			clients[pl]->Send_Move_Packet(npc->GetID());
		}
	}

	for (auto pl : old_vl) {
		if (0 == new_vl.count(pl)) {
			clients[pl]->m_ViewLock.lock();
			if (0 != clients[pl]->GetViewList().count(npc->GetID())) {
				clients[pl]->m_ViewLock.unlock();
				clients[pl]->Send_RemoveObject_Packet(npc->GetID());
			}
			else {
				clients[pl]->m_ViewLock.unlock();
			}
		}
	}
}

void do_npc_attack(int npc_id)
{	
	CNpc* npc = dynamic_cast<CNpc*>(clients[npc_id]);
	if (npc->m_active == false)
		return;

	CPlayer* player = dynamic_cast<CPlayer*>(clients[npc->GetTarget()]);

	player->SetCurHp(player->GetCurHp() - npc->GetPower());
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
		if (clients[i]->GetState() != CL_STATE::ST_INGAME)
			continue;
		if (i == player->GetID()) {
			sprintf_s(msg, CHAT_SIZE, "%dDamage from %s", npc->GetPower(), npc->GetName());
		}
		else {
			sprintf_s(msg, CHAT_SIZE, "%s has been damaged %d by %s", player->GetName(), npc->GetPower(), npc->GetName());
		}
		clients[i]->Send_Chat_Packet(i, msg);
	}

	if (g_heal == false) {
		TIMER_EVENT ev{ npc->GetTarget(), chrono::system_clock::now() + 5s, EV_PLAYER_HEAL, 0 };
		timer_queue.push(ev);
		g_heal = true;
	}
}

bool IsDest(int startX, int startY, int destX, int destY)
{
	if (startX == destX && startY == destY)
		return true;

	return false;
}

bool IsValid(int x, int y)
{
	return (x >= 0 && x < W_WIDTH && y >= 0 && y < W_HEIGHT);
}

bool IsUnBlocked(int x, int y)
{
	if (g_tilemap[y][x] == 'D' || g_tilemap[y][x] == 'G')
		return true;

	return false;
}

double CalH(int x, int y, int destX, int destY)
{
	return (abs(x - destX) + abs(y - destY));
}

void FindPath(Node* node[], int destX, int destY, int* x, int* y) 
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

bool AStar(int startX, int startY, int destX, int destY, int* resultx, int* resulty)
{
	int dx1[4] = { 0, 0, 1, -1 };
	int dy1[4] = { -1, 1, 0, 0 };

	if (!IsValid(startX, startY) || !IsValid(destX, destY)) return false;
	if (!IsUnBlocked(startX, startY) || !IsUnBlocked(destX, destY)) return false;
	if (IsDest(startX, startY, destX, destY)) return false;

	bool** closedList = new bool*[W_HEIGHT];
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

void process_packet(int c_id, char* packet)
{
	switch (packet[1]) {
	case CS_LOGIN: {
		CS_LOGIN_PACKET* p = reinterpret_cast<CS_LOGIN_PACKET*>(packet);

		if (strcmp(p->name, "Stress_Test")) {
			USER_INFO* user_info = reinterpret_cast<USER_INFO*>(g_DB->GetPlayerInfo(p->name));

			if (user_info->max_hp == -1) {
				g_DB->MakeNewInfo(p->name);
			}
			else {
				if (CheckLoginFail(p->name))
				{
					//disconnect(c_id);
				}
				else {
					clients[c_id]->SetName(p->name);
					dynamic_cast<CPlayer*>(clients[c_id])->SetExp(user_info->exp);
					{
						lock_guard<mutex> ll{ clients[c_id]->m_StateLock };
						clients[c_id]->SetPos(user_info->pos_x, user_info->pos_y);
						clients[c_id]->SetLevel(user_info->level);
						clients[c_id]->SetMaxHp(user_info->max_hp);
						clients[c_id]->SetCurHp(user_info->cur_hp);
						dynamic_cast<CPlayer*>(clients[c_id])->SetMaxExp(INIT_EXP + (user_info->level - 1) * EXP_UP);
						dynamic_cast<CPlayer*>(clients[c_id])->SetMaxMp(user_info->max_mp);
						dynamic_cast<CPlayer*>(clients[c_id])->SetMp(user_info->cur_mp);
						clients[c_id]->SetState(CL_STATE::ST_INGAME);
					}

					dynamic_cast<CPlayer*>(clients[c_id])->Send_LoginInfo_Packet();
					for (auto& pl : clients) {
						{
							lock_guard<mutex> ll(pl->m_StateLock);
							if (CL_STATE::ST_INGAME != pl->GetState())
								continue;
						}
						if (pl->GetID() == c_id)
							continue;
						if (false == Can_See(c_id, pl->GetID()))
							continue;
						if (pl->GetID() < MAX_USER)
							pl->Send_AddObject_Packet(c_id);
						else
							WakeUpNPC(pl->GetID(), c_id);
						clients[c_id]->Send_AddObject_Packet(pl->GetID());
					}
					CPlayer* player = dynamic_cast<CPlayer*>(clients[c_id]);
					if (user_info->item1 != -1) {
						player->SetItem(0, static_cast<ITEM_TYPE>(user_info->item1 + 1), user_info->moneycnt, true);
						SC_ITEM_GET_PACKET p;
						p.size = sizeof(p);
						p.type = SC_ITEM_GET;
						p.inven_num = 0;
						p.item_type = static_cast<ITEM_TYPE>(user_info->item1 + 1);
						p.x = player->GetPosX();
						p.y = player->GetPosY();
						player->SendPacket(&p);
					}
					if (user_info->item2 != -1) {
						player->SetItem(1, static_cast<ITEM_TYPE>(user_info->item2 + 1), 0, true);
						SC_ITEM_GET_PACKET p;
						p.size = sizeof(p);
						p.type = SC_ITEM_GET;
						p.inven_num = 1;
						p.item_type = static_cast<ITEM_TYPE>(user_info->item2 + 1);
						p.x = player->GetPosX();
						p.y = player->GetPosY();
						player->SendPacket(&p);
					}
					if (user_info->item3 != 3) {
						player->SetItem(2, static_cast<ITEM_TYPE>(user_info->item3 + 1), 0, true);
						SC_ITEM_GET_PACKET p;
						p.size = sizeof(p);
						p.type = SC_ITEM_GET;
						p.inven_num = 2;
						p.item_type = static_cast<ITEM_TYPE>(user_info->item3 + 1);
						p.x = player->GetPosX();
						p.y = player->GetPosY();
						player->SendPacket(&p);
					}
					if (user_info->item4 != -1) {
						player->SetItem(3, static_cast<ITEM_TYPE>(user_info->item4 + 1), 0, true);
						SC_ITEM_GET_PACKET p;
						p.size = sizeof(p);
						p.type = SC_ITEM_GET;
						p.inven_num = 3;
						p.item_type = static_cast<ITEM_TYPE>(user_info->item4 + 1);
						p.x = player->GetPosX();
						p.y = player->GetPosY();
						player->SendPacket(&p);
					}
					if (user_info->item5 != -1) {
						player->SetItem(4, static_cast<ITEM_TYPE>(user_info->item5 + 1), 0, true);
						SC_ITEM_GET_PACKET p;
						p.size = sizeof(p);
						p.type = SC_ITEM_GET;
						p.inven_num = 4;
						p.item_type = static_cast<ITEM_TYPE>(user_info->item5 + 1);
						p.x = player->GetPosX();
						p.y = player->GetPosY();
						player->SendPacket(&p);
					}
					if (user_info->item6 != -1) {
						player->SetItem(5, static_cast<ITEM_TYPE>(user_info->item6 + 1), 0, true);
						SC_ITEM_GET_PACKET p;
						p.size = sizeof(p);
						p.type = SC_ITEM_GET;
						p.inven_num = 5;
						p.item_type = static_cast<ITEM_TYPE>(user_info->item6 + 1);
						p.x = player->GetPosX();
						p.y = player->GetPosY();
						player->SendPacket(&p);
					}
				}
			}
		}
		else {
			clients[c_id]->SetName(p->name);
			clients[c_id]->SetPos(rand() % W_WIDTH, rand() % W_HEIGHT);
			clients[c_id]->SetMaxHp(1000000);
			clients[c_id]->SetCurHp(1000000);
			dynamic_cast<CPlayer*>(clients[c_id])->SetMaxExp(100);
			dynamic_cast<CPlayer*>(clients[c_id])->SetMaxMp(100);
			dynamic_cast<CPlayer*>(clients[c_id])->SetMp(100);
			{
				lock_guard<mutex> ll{ clients[c_id]->m_StateLock };
				clients[c_id]->SetState(CL_STATE::ST_INGAME);
			}
			dynamic_cast<CPlayer*>(clients[c_id])->Send_LoginInfo_Packet();
			for (auto& pl : clients) {
				{
					lock_guard<mutex> ll(pl->m_StateLock);
					if (CL_STATE::ST_INGAME != pl->GetState())
						continue;
				}
				if (pl->GetID() == c_id)
					continue;
				if (false == Can_See(c_id, pl->GetID()))
					continue;
				if (pl->GetID() < MAX_USER)
					pl->Send_AddObject_Packet(c_id);
				else
					WakeUpNPC(pl->GetID(), c_id);
				clients[c_id]->Send_AddObject_Packet(pl->GetID());
			}
		}
		break;
	}
	case CS_MOVE: {
		CS_MOVE_PACKET* p = reinterpret_cast<CS_MOVE_PACKET*>(packet);
		clients[c_id]->last_move_time = p->move_time;
		short x = clients[c_id]->GetPosX();
		short y = clients[c_id]->GetPosY();
		if (Can_Move(x, y, p->direction)) {
			switch (p->direction) {
			case 0: {
				dynamic_cast<CPlayer*>(clients[c_id])->SetDir(DIR::UP);
				y--; break;
			}
			case 1: {
				dynamic_cast<CPlayer*>(clients[c_id])->SetDir(DIR::DOWN);
				y++; break;
			}
			case 2: {
				dynamic_cast<CPlayer*>(clients[c_id])->SetDir(DIR::LEFT);
				x--; break;
			}
			case 3: {
				dynamic_cast<CPlayer*>(clients[c_id])->SetDir(DIR::RIGHT);
				x++; break;
			}
			}
			clients[c_id]->SetPos(x, y);
			dynamic_cast<CPlayer*>(clients[c_id])->CheckItem();

			unordered_set<int> near_list;
			clients[c_id]->m_ViewLock.lock();
			unordered_set<int> old_vlist = clients[c_id]->GetViewList();
			clients[c_id]->m_ViewLock.unlock();
			for (auto& cl : clients) {
				if (cl->GetState() != CL_STATE::ST_INGAME)
					continue;
				if (cl->GetID() == c_id)
					continue;
				if (Can_See(c_id, cl->GetID()))
					near_list.insert(cl->GetID());
			}

			clients[c_id]->Send_Move_Packet(c_id);

			for (auto& pl : near_list) {
				auto& cpl = clients[pl];
				if (pl < MAX_USER) {
					cpl->m_ViewLock.lock();
					if (clients[pl]->GetViewList().count(c_id)) {
						cpl->m_ViewLock.unlock();
						clients[pl]->Send_Move_Packet(c_id);
					}
					else {
						cpl->m_ViewLock.unlock();
						clients[pl]->Send_AddObject_Packet(c_id);
					}
				}
				else {
					WakeUpNPC(pl, c_id);
				}

				if (old_vlist.count(pl) == 0)
					clients[c_id]->Send_AddObject_Packet(pl);
			}

			for (auto& pl : old_vlist) {
				if (0 == near_list.count(pl)) {
					clients[c_id]->Send_RemoveObject_Packet(pl);
					if (pl < MAX_USER)
						clients[pl]->Send_RemoveObject_Packet(c_id);
				}
			}
			break;
		}
	}
	case CS_ATTACK: {
		CS_ATTACK_PACKET* p = reinterpret_cast<CS_ATTACK_PACKET*>(packet);
		if (Can_Use(c_id, p->skill, p->time)) {
			switch (p->skill) {
			case 0:
				dynamic_cast<CPlayer*>(clients[c_id])->Attack();
				break;
			case 1:
				if (false == dynamic_cast<CPlayer*>(clients[c_id])->GetPowerUp()) {
					dynamic_cast<CPlayer*>(clients[c_id])->Skill1();
					dynamic_cast<CPlayer*>(clients[c_id])->SetMp(dynamic_cast<CPlayer*>(clients[c_id])->GetMp() - 15);
					dynamic_cast<CPlayer*>(clients[c_id])->SetPower(dynamic_cast<CPlayer*>(clients[c_id])->GetPower() * 2);
					dynamic_cast<CPlayer*>(clients[c_id])->Send_StatChange_Packet();
				}
				break;
			case 2:
				dynamic_cast<CPlayer*>(clients[c_id])->Skill2();
				dynamic_cast<CPlayer*>(clients[c_id])->SetMp(dynamic_cast<CPlayer*>(clients[c_id])->GetMp() - 5);
				dynamic_cast<CPlayer*>(clients[c_id])->Send_StatChange_Packet();
				break;
			case 3:
				dynamic_cast<CPlayer*>(clients[c_id])->Skill3();
				dynamic_cast<CPlayer*>(clients[c_id])->SetMp(dynamic_cast<CPlayer*>(clients[c_id])->GetMp() - 10);
				dynamic_cast<CPlayer*>(clients[c_id])->Send_StatChange_Packet();
				break;
			}
		}
		break;
	}
	case CS_USE_ITEM:
	{
		CS_USE_ITEM_PACKET* p = reinterpret_cast<CS_USE_ITEM_PACKET*>(packet);
		dynamic_cast<CPlayer*>(clients[c_id])->UseItem(p->inven);
		break;
	}
	}
}