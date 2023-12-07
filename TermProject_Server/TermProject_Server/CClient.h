#pragma once
#include "CObject.h"
#include "CSkill.h"
#include "CSession.h"

class CItem;

class CClient : public CObject
{
public:
	CClient();
	virtual ~CClient();

	const CL_STATE GetState() const { return m_State; }
	
	DIR GetDir() const { return m_dir; }
	CSession* GetSession() const { return m_session; }

	void SetState(CL_STATE state) { m_State = state; }
	void SetDir(DIR dir) { m_dir = dir; }

	void PlayerAccept(int id, SOCKET sock);
	void CheckItem();	

	void UseSkill(int skill);
	void UseItem(int inven);

	const ITEM_TYPE GetItemType(int index) const;
	void SetItem(int index, ITEM_TYPE type, int num, bool enable);

	void Heal();
	void Move(char dir);
	
	void AddObjectToView(int c_id);
	void RemoveObjectFromView(int c_id);

	virtual unordered_set<int> CheckSection();
	bool Damaged(int power, int attackID) override;

public:
	mutex m_itemLock;

private:	
	DIR m_dir;
	array<CSkill*, 4> m_skills;	
	array<CItem*, 6> m_items;

	CL_STATE m_State;	

	CSession* m_session;
};

