#pragma once

constexpr int ATTACK_COOLTIME = 1;
constexpr int POWERUP_COOLTIME = 20;
constexpr int FIRE_COOLTIME = 5;
constexpr int EXPLOSION_COOLTIME = 7;
constexpr int POWERUP_TIME = 10;

constexpr int ATTACK_MP_CONSUMTION = 0;
constexpr int POWERUP_MP_CONSUMTION = 15;
constexpr int FIRE_MP_CONSUMTION = 5;
constexpr int EXPLOSION_MP_CONSUMTION = 10;

class CSkill
{
public:
	CSkill() {}
	~CSkill() {}

	void SetCoolTime(int time) { m_coolTime = time; }
	int GetMpConsumtion() const { return m_mpConsumption; }
	bool CanUse(int mp);

	virtual void Use(int id) = 0;

protected:
	chrono::system_clock::time_point m_usedTime;
	int m_coolTime;
	int m_mpConsumption;
};

class CAttack : public CSkill
{
public:
	CAttack() { m_coolTime = ATTACK_COOLTIME; m_mpConsumption = ATTACK_MP_CONSUMTION; }

	void Use(int id) override;
};

class CPowerUp : public CSkill
{
public:
	CPowerUp() { m_coolTime = POWERUP_COOLTIME; m_mpConsumption = POWERUP_MP_CONSUMTION; }

	void Use(int id) override;
};

class CFire : public CSkill
{
public:
	CFire() { m_coolTime = FIRE_COOLTIME; m_mpConsumption = FIRE_MP_CONSUMTION; }

	void Use(int id) override;
};

class CExplosion : public CSkill
{
public:
	CExplosion() { m_coolTime = EXPLOSION_COOLTIME; m_mpConsumption = EXPLOSION_MP_CONSUMTION; }

	void Use(int id) override;
};