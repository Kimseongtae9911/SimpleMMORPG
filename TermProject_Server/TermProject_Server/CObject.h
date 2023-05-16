#pragma once

#define ATTACK_COOL 1.f
#define SKILL1_COOL 20.f
#define SKILL2_COOL 5.f
#define SKILL3_COOL 7.f
#define POWERUP_TIME 10.f

class CItem;

class OVER_EXP {
public:
	WSAOVERLAPPED m_over;
	WSABUF m_Wsabuf;
	char m_send_buf[BUF_SIZE];
	OP_TYPE m_comp_type;
	int m_ai_target_obj;

	OVER_EXP()
	{
		m_Wsabuf.len = BUF_SIZE;
		m_Wsabuf.buf = m_send_buf;
		m_comp_type = OP_TYPE::OP_RECV;
		ZeroMemory(&m_over, sizeof(m_over));
	}
	OVER_EXP(char* packet)
	{
		m_Wsabuf.len = packet[0];
		m_Wsabuf.buf = m_send_buf;
		ZeroMemory(&m_over, sizeof(m_over));
		m_comp_type = OP_TYPE::OP_SEND;
		memcpy(m_send_buf, packet, packet[0]);
	}
};

class CObject
{
public:
	CObject();
	virtual ~CObject();

	void RecvPacket();
	void SendPacket(void* packet);
	void Send_LoginFail_Packet();
	void Send_Move_Packet(int c_id);
	void Send_AddObject_Packet(int c_id);
	void Send_Chat_Packet(int c_id, const char* mess);
	void Send_RemoveObject_Packet(int c_id);

	void SetState(CL_STATE state) { m_State = state; }
	void SetRemainBuf(const int size) { m_RemainBuf_Size = size; }
	void SetName(char* name) { strcpy_s(m_Name, name); }
	void SetPos(const short x, const short y) { m_PosX = x; m_PosY = y; }
	void SetLevel(int level) { m_level = level; }
	void SetMaxHp(int maxhp) { m_maxHp = maxhp; }
	void SetCurHp(int curhp) { if (curhp > m_maxHp) { m_curHp = m_maxHp; } else { m_curHp = curhp; } }
	void SetPower(int power) { m_power = power; }

	const CL_STATE GetState() const { return m_State; }
	const int GetRemainBuf() const { return m_RemainBuf_Size; }
	const short GetPosX() const { return m_PosX; }
	const short GetPosY() const { return m_PosY; }
	const int GetLevel() const { return m_level; }
	const int GetMaxHp() const { return m_maxHp; }
	const int GetCurHp() const { return m_curHp; }
	const int GetID() const { return m_ID; }
	const char* GetName() const { return m_Name; }
	const int GetPower() const { return m_power; }

	const unordered_set<int>& GetViewList() const {return m_view_list; }
	const SOCKET& GetSocket() const { return m_Socket; }

	const bool CanSee(int to) const;

public:
	mutex m_StateLock;
	shared_mutex m_ViewLock;
	int	last_move_time;

protected:
	SOCKET m_Socket;
	int	m_RemainBuf_Size;

	unordered_set<int> m_view_list;
	
	CL_STATE m_State;
	int m_ID;

	short m_PosX, m_PosY;
	char m_Name[NAME_SIZE];

	int m_level;
	int m_maxHp;
	int m_curHp;
	int m_power;

private:
	OVER_EXP m_RecvOver;
};

class CNpc : public CObject
{
public:
	CNpc(int id);
	virtual ~CNpc();

	void SetMonType(MONSTER_TYPE type) { m_monType = type; }
	void SetRespawnPos(short x, short y) { m_respawnX = x; m_respawnY = y; }
	void SetChase(bool chase) { m_chase = chase; }
	void SetTarget(int target) { m_target = target; }
	void SetDie(bool die) { m_die = die; }

	lua_State* GetLua() { return m_L; }
	const MONSTER_TYPE GetMonType() const { return m_monType; }
	const bool GetChase() const { return m_chase; }
	const int GetTarget() const { return m_target; }
	const bool GetDie() const { return m_die; }
	const short GetRespawnX() const { return m_respawnX; }
	const short GetRespawnY() const { return m_respawnY; }

	void Attack();
	void Chase();
	void RandomMove();

	bool Agro(int to);
	void WakeUp(int waker);

private:
	bool AStar(int startX, int startY, int destX, int destY, int* resultx, int* resulty);
	bool IsDest(int startX, int startY, int destX, int destY);
	bool IsValid(int x, int y);
	bool IsUnBlocked(int x, int y);
	double CalH(int x, int y, int destX, int destY);
	void FindPath(Node* node[], int destX, int destY, int* x, int* y);

public:
	atomic_bool	m_active;
	mutex LuaLock;

private:	
	lua_State* m_L;	
	MONSTER_TYPE m_monType;
	short m_respawnX, m_respawnY;

	bool m_die = false;
	bool m_chase;
	int m_target;
};

class CPlayer : public CObject
{
public:
	CPlayer();
	virtual ~CPlayer();

	void SetExp(const int exp) { m_exp = exp; }
	void SetMaxExp(const int exp) { m_maxExp = exp; }
	void SetMaxMp(const int mp) { m_maxMp = mp; }
	void SetMp(const int mp) { if (mp > m_maxMp) { m_curMp = m_maxMp; } else { m_curMp = mp; } }
	void SetUsedTime(const int index, const chrono::system_clock::time_point time) { m_usedTime[index] = time; }
	void SetDir(DIR dir) { m_dir = dir; }
	void SetPowerUp(bool power) { m_powerup = power; }

	const int GetExp() const { return m_exp; }
	const int GetMp() const { return m_curMp; }
	const int GetMaxMp() const { return m_maxMp; }
	const bool GetPowerUp() const { return m_powerup; }
	const chrono::system_clock::time_point GetUsedTime(const int index) const { return m_usedTime[index]; }

	void PlayerAccept(int id, SOCKET sock);
	void CheckItem();
	void Send_LoginInfo_Packet();
	void Send_StatChange_Packet();
	void Send_Damage_Packet(int cid, int powerlv);
	void Send_ItemUsed_Packet(int inven);

	void Attack();
	void Skill1();
	void Skill2();
	void Skill3();
	void GainExp(int exp);
	void CreateItem(short x, short y);
	void UseItem(int inven);

	const ITEM_TYPE GetItemType(int index) const;
	void SetItem(int index, ITEM_TYPE type, int num, bool enable);

	bool GetHeal() const { return m_heal; }
	void SetHeal(bool heal) { m_heal = heal; }
	bool CanUse(char skill, chrono::system_clock::time_point time);

	void Heal();

public:
	mutex m_itemLock;

private:
	chrono::system_clock::time_point m_poweruptime;
	bool m_powerup;

	int m_maxExp;
	int m_exp;
	int m_maxMp;
	int m_curMp;
	DIR m_dir;
	array<chrono::system_clock::time_point, 4> m_usedTime;
	array<CItem*, 6> m_items;

	bool m_heal;
};