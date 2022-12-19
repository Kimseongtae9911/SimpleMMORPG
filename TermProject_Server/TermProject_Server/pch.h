#pragma once
#include <iostream>
#include <fstream>
#include <random>
#include <WS2tcpip.h>
#include <MSWSock.h>
#include <thread>
#include <mutex>
#include <array>
#include <vector>
#include <unordered_set>
#include <concurrent_priority_queue.h>
#include "protocol.h"

extern "C" {
	#include "include\lua.h"
	#include "include\lauxlib.h"
	#include "include\lualib.h"
}

#pragma comment(lib, "WS2_32.lib")
#pragma comment(lib, "MSWSock.lib")
#pragma comment (lib, "lua54.lib")

using namespace std;

constexpr int VIEW_RANGE = 7;
constexpr int INIT_EXP = 10;
constexpr int EXP_UP = 40;

enum class DIR {LEFT, RIGHT, UP, DOWN};

enum class OP_TYPE { OP_ACCEPT, OP_RECV, OP_SEND, OP_NPC_MOVE, OP_AI_HELLO };

enum class CL_STATE {ST_FREE, ST_ALLOC, ST_INGAME};

enum ITEM_TYPE {NONE, MONEY, HP_POTION, MP_POTION, WAND, CLOTH, RING, HAT};

enum MONSTER_TYPE {};

enum EVENT_TYPE { EV_RANDOM_MOVE };

struct TIMER_EVENT {
	int obj_id;
	chrono::system_clock::time_point wakeup_time;
	EVENT_TYPE event_id;
	int target_id;
	constexpr bool operator < (const TIMER_EVENT& L) const
	{
		return (wakeup_time > L.wakeup_time);
	}
};

#pragma pack (push, 1)
struct USER_INFO {
	int max_hp;
	int cur_hp;
	int max_mp;
	int cur_mp;
	int level;
	int exp;
	short pos_x;
	short pos_y;
	char name[NAME_SIZE + 1];
};
#pragma pack (pop)