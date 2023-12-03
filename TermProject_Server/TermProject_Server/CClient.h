#pragma once
#include "CObject.h"
#include "CSkill.h"
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
	DIR GetDir() const { return m_dir; }

	void SetState(CL_STATE state) { m_State = state; }
	void SetRemainBuf(const int size) { m_RemainBuf_Size = size; }
	void SetDir(DIR dir) { m_dir = dir; }

	void PlayerAccept(int id, SOCKET sock);
	void CheckItem();
	void Send_LoginInfo_Packet();
	void Send_StatChange_Packet();
	void Send_Damage_Packet(int cid, int powerlv);
	void Send_ItemUsed_Packet(int inven);

	void UseSkill(int skill);
	void UseItem(int inven);

	const ITEM_TYPE GetItemType(int index) const;
	void SetItem(int index, ITEM_TYPE type, int num, bool enable);

	void Heal();

	void RecvPacket();
	void SendPacket(void* packet);
	void Send_LoginFail_Packet();
	void Send_Move_Packet(int c_id);
	void Send_AddObject_Packet(int c_id);
	void Send_Chat_Packet(int c_id, const char* mess);
	void Send_RemoveObject_Packet(int c_id);

	virtual unordered_set<int> CheckSection();
	bool Damaged(int power, int attackID) override;

public:
	mutex m_itemLock;

private:	

	DIR m_dir;
	array<CSkill*, 4> m_skills;	
	array<CItem*, 6> m_items;

	CL_STATE m_State;

	SOCKET m_Socket;
	int	m_RemainBuf_Size;
	COverlapEx m_recvOver;
};

