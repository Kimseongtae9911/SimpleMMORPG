#pragma once
#include "CObject.h"
#include "COverlapEx.h"

class CItem;

class CClient : public CObject
{
public:
	CClient();
	virtual ~CClient();

	const CL_STATE GetState() const { return m_State; }
	const int GetRemainBuf() const { return m_RemainBuf_Size; }
	const SOCKET& GetSocket() const { return m_Socket; }

	void SetState(CL_STATE state) { m_State = state; }
	void SetRemainBuf(const int size) { m_RemainBuf_Size = size; }

	void SetUsedTime(const int index, const chrono::system_clock::time_point time) { m_usedTime[index] = time; }
	void SetDir(DIR dir) { m_dir = dir; }
	void SetPowerUp(bool power) { m_powerup = power; }

	
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
	void CreateItem(short x, short y);
	void UseItem(int inven);

	const ITEM_TYPE GetItemType(int index) const;
	void SetItem(int index, ITEM_TYPE type, int num, bool enable);

	bool CanUse(char skill, chrono::system_clock::time_point time);

	void Heal();

	void RecvPacket();
	void SendPacket(void* packet);
	void Send_LoginFail_Packet();
	void Send_Move_Packet(int c_id);
	void Send_AddObject_Packet(int c_id);
	void Send_Chat_Packet(int c_id, const char* mess);
	void Send_RemoveObject_Packet(int c_id);

	virtual unordered_set<int> CheckSection();
	void Damaged(int power) override;

public:
	mutex m_itemLock;

private:
	chrono::system_clock::time_point m_poweruptime;
	bool m_powerup;

	DIR m_dir;
	array<chrono::system_clock::time_point, 4> m_usedTime;
	array<CItem*, 6> m_items;

	CL_STATE m_State;

	SOCKET m_Socket;
	int	m_RemainBuf_Size;
	COverlapEx m_recvOver;
};

