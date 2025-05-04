#pragma once
#include "CStat.h"

class CObject
{
public:
	CObject();
	virtual ~CObject();
	
	void SetPos(const short x, const short y) { m_PosX = x; m_PosY = y; }
	void SetSection(int x, int y) { m_sectionX = x; m_sectionY = y; }
	void SetName(char* name) { strcpy_s(m_name, name); }

	CStat* GetStat() const { return m_stat.get(); }
	const char* GetName() const { return m_name; }
	short GetPosX() const { return m_PosX; }
	short GetPosY() const { return m_PosY; }
	int GetID() const { return m_ID; }
	
	int GetSectionX() const { return m_sectionX; }
	int GetSectionY() const { return m_sectionY; }

	const unordered_set<int>& GetViewList() const {return m_viewList; }	

	bool CanSee(int to) const;
	
	virtual void CheckSection(std::unordered_set<int>& viewList) { }
	virtual bool Damaged(int power, int attackID) = 0;
	virtual void Update() {}

public:
	mutex m_StateLock;
	shared_mutex m_ViewLock;
	unsigned int lastMoveTime;

protected:
	unordered_set<int> m_viewList;
	char m_name[NAME_SIZE];
	int m_ID;

	short m_PosX, m_PosY;

	unique_ptr<CStat> m_stat;

	int m_sectionX = 0;
	int m_sectionY = 0;
};