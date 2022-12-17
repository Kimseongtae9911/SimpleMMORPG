#pragma once
class CEffect
{
public:
	CEffect() {}
	CEffect(string filename, float tba, int fa);
	~CEffect();

	void Update(float ElapsdTime);
	virtual void Render(sf::RenderWindow& RW, int x, int y, int left, int top, DIR dir) {}

	const bool GetEnable() const { return m_enable; }

	void SetEnable(const bool enable) { m_enable = enable; }

protected:
	sf::Texture m_texture;
	sf::Sprite m_sprite;

	float m_time_by_action;
	float m_action_time;
	int m_frame_action;

	float m_spriteLeft;

	bool m_enable;
};

class CAttackEffect : public CEffect
{
public:
	CAttackEffect() {}
	CAttackEffect(string filename, float tba, int fa);
	~CAttackEffect() {}

	virtual void Render(sf::RenderWindow& RW, int x, int y, int left, int top, DIR dir);
};

class CSkillEffect1 : public CEffect
{
public:
	CSkillEffect1() {}
	CSkillEffect1(string filename, float tba, int fa);
	~CSkillEffect1() {}

	virtual void Render(sf::RenderWindow& RW, int x, int y, int left, int top, DIR dir);
};

class CSkillEffect2 : public CEffect
{
public:
	CSkillEffect2() {}
	CSkillEffect2(string filename, float tba, int fa);
	~CSkillEffect2() {}

	virtual void Render(sf::RenderWindow& RW, int x, int y, int left, int top, DIR dir);
};

class CSkillEffect3 : public CEffect
{
public:
	CSkillEffect3() {}
	CSkillEffect3(string filename, float tba, int fa);
	~CSkillEffect3() {}

	virtual void Render(sf::RenderWindow& RW, int x, int y, int left, int top, DIR dir);
};