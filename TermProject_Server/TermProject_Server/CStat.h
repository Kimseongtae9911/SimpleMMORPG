#pragma once
class CStat
{
public:
	void SetLevel(int level) { m_level = level; }
	void SetMaxHp(int maxhp) { m_maxHp = maxhp; }
	void SetCurHp(int curhp) { if (curhp > m_maxHp) { m_curHp = m_maxHp; } else { m_curHp = curhp; } }
	void SetPower(int power) { m_power = power; }

	int GetLevel() const { return m_level; }
	int GetMaxHp() const { return m_maxHp; }
	int GetCurHp() const { return m_curHp; }
	int GetPower() const { return m_power; }

	virtual bool Damaged(int power);

protected:
	mutex m_hpLock;

	int m_level;
	int m_maxHp;
	int m_curHp;
	int m_power;
};

class CClientStat : public CStat
{
public:
	CClientStat() {}
	CClientStat(int power) { m_power = power; }

	int GetExp() const { return m_exp; }
	int GetMp() const { return m_curMp; }
	int GetMaxMp() const { return m_maxMp; }
	int GetCurMp() const { return m_curMp; }

	void SetExp(const int exp) { m_exp = exp; }
	void SetMaxExp(const int exp) { m_maxExp = exp; }
	void SetMaxMp(const int mp) { m_maxMp = mp; }
	void SetMp(const int mp) { if (mp > m_maxMp) { m_curMp = m_maxMp; } else { m_curMp = mp; } }

	void GainExp(int exp);
	void HealHp(int heal);
	void HealMp(int heal);
	bool Damaged(int power) override;

	bool IsFullCondition() const { return (m_curHp == m_maxHp) && (m_curMp == m_maxMp); }
	bool IsMaxMp() const { return m_curMp == m_maxMp; }
	bool IsMaxHp() const { return m_curHp == m_maxHp; }

private:
	int m_maxExp;
	int m_exp;
	int m_maxMp;
	int m_curMp;
};

