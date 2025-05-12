#pragma once
#include "CObject.h"

class CNpc : public CObject
{
public:
	CNpc(int id);
	virtual ~CNpc();

	void SetMonType(MONSTER_TYPE type) { m_monType = type; }
	void SetRespawnPos(short x, short y) { m_respawnX = x; m_respawnY = y; }
	void SetTarget(int target) { m_target = target; }
	void SetDie(bool die) { m_die = die; }

	lua_State* GetLua() { return m_L; }
	MONSTER_TYPE GetMonType() const { return m_monType; }
	
	const bool GetDie() const { return m_die; }
	const short GetRespawnX() const { return m_respawnX; }
	const short GetRespawnY() const { return m_respawnY; }

	void Attack();
	void Chase();
	void RandomMove();

	bool Agro(int to);
	void WakeUp(int waker);

	void CheckSection(std::unordered_set<int>& viewList) override;

	bool Damaged(int power, int attackID) override;
	void Update() override;

private:
	bool AStar(int startX, int startY, int destX, int destY, int* resultx, int* resulty);
	bool IsDest(int startX, int startY, int destX, int destY);
	bool IsValid(int x, int y);
	bool IsUnBlocked(int x, int y);
	double CalH(int x, int y, int destX, int destY);
	bool FindNextStep(Node node[MONSTER_VIEW * 2 + 1][MONSTER_VIEW * 2 + 1], int x, int y, int* resultx, int* resulty, int offsetX, int offsetY);
	void ViewListUpdate(const unordered_set<int>& viewList);

public:
	atomic_bool	m_active;
	mutex LuaLock;

private:
	lua_State* m_L;
	MONSTER_TYPE m_monType;
	short m_respawnX, m_respawnY;

	bool m_die = false;
	int m_target;
	NPC_STATE m_state;
};