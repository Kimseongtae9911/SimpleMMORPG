﻿#pragma once
#include <iostream>
#include <fstream>
#include <random>
#include <WS2tcpip.h>
#include <MSWSock.h>
#include <thread>
#include <mutex>
#include <shared_mutex>
#include <array>
#include <vector>
#include <unordered_set>
#include <set>
#include <queue>
#include <stack>
#include <string>
#include <concurrent_priority_queue.h>
#include <tbb/concurrent_hash_map.h>
#include <tbb/concurrent_queue.h>
#include <unordered_map>
#include <functional>
#include <memory>
#include "protocol.h"

extern "C" {
	#include "include\lua.h"
	#include "include\lauxlib.h"
	#include "include\lualib.h"
}

#pragma comment(lib, "WS2_32.lib")
#pragma comment(lib, "MSWSock.lib")
#pragma comment (lib, "lua54.lib")

//#define DATABASE
#define WITH_VIEW
#define WITH_SECTION
//#define CHECK_NPC_ACTIVE

using namespace std;

constexpr int VIEW_RANGE = 7;
constexpr int MONSTER_VIEW = 5;
constexpr int INIT_EXP = 10;
constexpr int EXP_UP = 40;
constexpr int INF = 2140000000;

#define SINGLETON(ClassName)									\
public:															\
	ClassName() {};												\
	~ClassName() {};											\
	ClassName(ClassName const&) = delete;						\
	ClassName& operator=(const ClassName&) = delete;			\
	static ClassName* GetInstance()	{							\
		if(!m_instance)											\
			m_instance.reset(new ClassName);					\
		return m_instance.get();								\
	}															\
																\
	void DestroyInstance()										\
	{															\
		if (m_instance)											\
			m_instance.reset(nullptr);							\
	}															\
private:														\
	static std::unique_ptr<ClassName> m_instance;

enum class DIR {LEFT, RIGHT, UP, DOWN};

enum class OP_TYPE { OP_ACCEPT, OP_RECV, OP_SEND, OP_NPC_MOVE, OP_PLAYER_HEAL, OP_MONSTER_RESPAWN, OP_POWERUP_ROLLBACK};

enum class CL_STATE {ST_FREE, ST_ALLOC, ST_INGAME};

enum class NPC_STATE {PATROL, CHASE, ATTACK};

enum ITEM_TYPE { NONE, MONEY, HP_POTION, MP_POTION, WAND, CLOTH, RING, HAT };

enum MONSTER_TYPE { AGRO, PEACE };

enum EVENT_TYPE { EV_RANDOM_MOVE, EV_CHASE_PLAYER, EV_ATTACK_PLAYER, EV_PLAYER_HEAL, EV_MONSTER_RESPAWN, EV_POWERUP_ROLLBACK };

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

struct WeightPos {
	double weight;
	int x;
	int y;

	constexpr bool operator < (const WeightPos& R) const
	{
		return (weight > R.weight);
	}
};

struct Node {
	int x, y;
	int parentX, parentY;
	double f = INF, g = INF, h = INF;
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
	short item1;
	short item2;
	short item3;
	short item4;
	short item5;
	short item6;
	int moneycnt;
};
#pragma pack (pop)