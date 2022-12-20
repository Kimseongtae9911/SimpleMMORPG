#pragma once
class CItem
{
public:
	CItem();
	~CItem();

	void SetItemType(ITEM_TYPE type) { m_itemType = type; }
	void SetEnable(bool in) { m_enable = in; }
	void SetCreateTime(chrono::system_clock::time_point time) { m_createTime = time; }
	void SetNum(int num) { m_num = num; }

	const ITEM_TYPE GetItemType() const { return m_itemType; }
	const bool GetEnable() const { return m_enable; }
	const chrono::system_clock::time_point GetCreateTime() const { return m_createTime; }
	const int GetNum() const { return m_num; }

private:
	ITEM_TYPE m_itemType;
	bool m_enable;
	int m_num = 0;
	chrono::system_clock::time_point m_createTime;
};

