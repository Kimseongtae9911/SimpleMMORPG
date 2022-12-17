#include "pch.h"
#include "CObject.h"
#include "CDatabase.h"

concurrency::concurrent_priority_queue<TIMER_EVENT> timer_queue;

CDatabase* g_DB;
HANDLE h_iocp;
array<CObject*, MAX_USER + MAX_NPC> clients;
char g_tilemap[W_HEIGHT][W_WIDTH];

SOCKET g_s_socket, g_c_socket;
OVER_EXP g_a_over;

void InitializeData();
int Get_NewID();

bool Can_See(int from, int to);
bool Can_Move(short x, short y, char dir);
bool Can_Use(int id, char skill, chrono::system_clock::time_point time);

void WakeUpNPC(int npc_id, int waker)
{
	OVER_EXP* exover = new OVER_EXP;
	exover->m_comp_type = OP_TYPE::OP_AI_HELLO;
	exover->m_ai_target_obj = waker;
	PostQueuedCompletionStatus(h_iocp, 1, npc_id, &exover->m_over);

	if (dynamic_cast<CNpc*>(clients[npc_id])->m_active) 
		return;
	bool old_state = false;
	if (false == atomic_compare_exchange_strong(&dynamic_cast<CNpc*>(clients[npc_id])->m_active, &old_state, true))
		return;
	TIMER_EVENT ev{ npc_id, chrono::system_clock::now(), EV_RANDOM_MOVE, 0 };
	timer_queue.push(ev);
}

void process_packet(int c_id, char* packet)
{
	switch (packet[1]) {
	case CS_LOGIN: {
		CS_LOGIN_PACKET* p = reinterpret_cast<CS_LOGIN_PACKET*>(packet);
		USER_INFO* user_info = reinterpret_cast<USER_INFO*>(g_DB->GetPlayerInfo(p->name));
		if (user_info->max_hp == -1) {
			clients[c_id]->Send_LoginFail_Packet();
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
			case 0: y--; break;
			case 1: y++; break;
			case 2: x--; break;
			case 3: x++; break;
			}
			clients[c_id]->SetPos(x, y);

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
				else WakeUpNPC(pl, c_id);

				if (old_vlist.count(pl) == 0)
					clients[c_id]->Send_AddObject_Packet(pl);
			}

			for (auto& pl : old_vlist) {
				if (0 == near_list.count(pl)) {
					clients[c_id]->Send_RemoveObject_Packet(pl);
					if (pl < MAX_USER)
						clients[c_id]->Send_RemoveObject_Packet(c_id);
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
				break;
			case 2:
				break;
			case 3:
				break;
			}
		}
		break;
	}
	}
}

void disconnect(int c_id)
{	
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

void do_npc_random_move(int npc_id)
{
	CObject* npc = clients[npc_id];
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
	unordered_set<int> new_vl;
	for (auto& obj : clients) {
		if (CL_STATE::ST_INGAME != obj->GetState()) 
			continue;
		if (obj->GetID() >= MAX_USER) 
			continue;
		if (true == Can_See(npc->GetID(), npc->GetID()))
			new_vl.insert(obj->GetID());
	}

	for (auto pl : new_vl) {
		if (0 == old_vl.count(pl)) {
			// 플레이어의 시야에 등장
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
			bool keep_alive = false;
			for (int j = 0; j < MAX_USER; ++j)
				if (Can_See(static_cast<int>(key), j)) {
					keep_alive = true;
					break;
				}
			if (true == keep_alive) {
				do_npc_random_move(static_cast<int>(key));
				TIMER_EVENT ev{ key, chrono::system_clock::now() + 1s, EV_RANDOM_MOVE, 0 };
				timer_queue.push(ev);
			}
			else {
				dynamic_cast<CNpc*>(clients[key])->m_active = false;
			}
			delete ex_over;
		}
						break;
		case OP_TYPE::OP_AI_HELLO:
			dynamic_cast<CNpc*>(clients[key])->LuaLock.lock();
			lua_getglobal(dynamic_cast<CNpc*>(clients[key])->GetLua(), "event_player_move");
			lua_pushnumber(dynamic_cast<CNpc*>(clients[key])->GetLua(), ex_over->m_ai_target_obj);
			lua_pcall(dynamic_cast<CNpc*>(clients[key])->GetLua(), 1, 0, 0);
			lua_pop(dynamic_cast<CNpc*>(clients[key])->GetLua(), 1);
			dynamic_cast<CNpc*>(clients[key])->LuaLock.unlock();
			delete ex_over;
			break;
		}
	}
}

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

void timer_func()
{
	while (true) {
		TIMER_EVENT ev;
		auto current_time = chrono::system_clock::now();
		auto temp_queue = timer_queue;
		if (true == temp_queue.try_pop(ev)) {
			if (ev.wakeup_time > current_time) {
				this_thread::sleep_for(1ms);
				continue;
			}
			if (clients[ev.obj_id]->GetCurHp() > 0.f) {
				switch (ev.event_id) {
				case EV_RANDOM_MOVE:
					OVER_EXP* ov = new OVER_EXP;
					ov->m_comp_type = OP_TYPE::OP_NPC_MOVE;
					PostQueuedCompletionStatus(h_iocp, 1, ev.obj_id, &ov->m_over);
					break;
				}
			}
			timer_queue.try_pop(ev);
			continue;
		}
		this_thread::sleep_for(1ms);
	}
}

void InitializeData()
{
	g_DB = new CDatabase();
	g_DB->Initialize();

	for (int i = 0; i < MAX_USER; ++i) {
		clients[i] = new CPlayer();
	}
	cout << "Initialize NPC Start" << endl;
	for (int i = MAX_USER; i < MAX_USER + MAX_NPC; ++i) {
		//clients[i] = new CPlayer();
		clients[i] = new CNpc(i);
	}
	cout << "Initialize NPC Complete" << endl;

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
		if (chrono::duration_cast<chrono::seconds>(time - dynamic_cast<CPlayer*>(clients[id])->GetUsedTime(1)).count() >= SKILL1_COOL) {
			dynamic_cast<CPlayer*>(clients[id])->SetUsedTime(1, time);
			return true;
		}
		break;
	case 2:
		if (chrono::duration_cast<chrono::seconds>(time - dynamic_cast<CPlayer*>(clients[id])->GetUsedTime(2)).count() >= SKILL2_COOL) {
			dynamic_cast<CPlayer*>(clients[id])->SetUsedTime(2, time);
			return true;
		}
		break;
	case 3:
		if (chrono::duration_cast<chrono::seconds>(time - dynamic_cast<CPlayer*>(clients[id])->GetUsedTime(3)).count() >= SKILL3_COOL) {
			dynamic_cast<CPlayer*>(clients[id])->SetUsedTime(3, time);
			return true;
		}
		break;
	}
	return false;
}