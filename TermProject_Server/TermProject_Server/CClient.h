#pragma once
#include "CObject.h"
#include "CSkill.h"
#include "CSession.h"
#include "JobQueue.h"

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

	void CheckSection(std::unordered_set<int>& viewList) override;
	bool Damaged(int power, int attackID) override;

	JobQueue& GetJobQueue() { return m_jobQueue; }
	bool TryMarkInQueue()
	{
		bool expected = false;
		return m_isEnqueued.compare_exchange_strong(expected, true);
	}
	void UnmarkInQueue() { m_isEnqueued.store(false); }
	bool IsInQueue() const { return m_isEnqueued.load(); }
	bool IsDisconnected() const { return m_isDisconnected; }
	void SetDisconnected() { m_isDisconnected.store(true); }
	void Logout();

public:
	mutex m_itemLock;

private:	
	DIR m_dir;
	array<CSkill*, 4> m_skills;	
	array<CItem*, 6> m_items;

	CL_STATE m_State;
	bool m_isHealEventRegistered = false;

	CSession* m_session;

	std::atomic_bool m_isDisconnected = false;
	std::atomic_bool m_isEnqueued = false;
	JobQueue m_jobQueue;
};

