#include "pch.h"
#include "CStat.h"

void CStat::HealHp(int heal)
{
	m_hpLock.lock();
	m_curHp += heal;
	if (m_curHp > m_maxHp)
		m_curHp = m_maxHp;
	m_hpLock.unlock();
}

void CClientStat::GainExp(int exp)
{
	m_exp += exp;
	if (m_exp >= m_maxExp) {
		while (true) {
			m_exp -= m_maxExp;
			m_level++;
			m_maxExp = INIT_EXP + (m_level - 1) * EXP_UP;

			if (m_exp < m_maxExp)
				break;
		}
		m_maxHp = m_level * 100;
		m_curHp = m_maxHp;
		m_maxMp = m_level * 50;
		m_curMp = m_maxMp;
	}
}

void CClientStat::HealMp(int heal)
{
	m_curMp += heal;
	if (m_curMp > m_maxMp)
		m_curMp = m_maxMp;
}

bool CClientStat::Damaged(int power)
{
	m_hpLock.lock();
	m_curHp -= power;
	if (m_curHp <= 0) {
		m_curHp = m_maxHp / 2;
		m_hpLock.unlock();
		return true;
	}
	else {
		m_hpLock.unlock();
		return false;
	}
}