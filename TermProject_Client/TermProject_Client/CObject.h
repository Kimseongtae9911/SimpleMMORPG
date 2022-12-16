#pragma once
class CObject
{
protected:
	bool m_showing;
	sf::Sprite m_sprite;

	sf::Font m_font = {};
	sf::Text m_name = {};
	sf::Text m_chat = {};
	chrono::system_clock::time_point m_mess_end_time = {};

public:
	int m_x, m_y;
	int m_id;
	CObject(sf::Texture& t, int x, int y, int x2, int y2);

	CObject() { m_showing = false; }

	virtual ~CObject() {}

	void show() { m_showing = true; }
	void hide() { m_showing = false; }

	void a_move(int x, int y) { m_sprite.setPosition((float)x, (float)y); }

	void a_draw(sf::RenderWindow& RW) { RW.draw(m_sprite); }

	void ChangeTex(int x, int y, int x2, int y2) { m_sprite.setTextureRect({ x, y, x2, y2 }); }
};

class CMoveObject : public CObject
{
public:
	CMoveObject() {}
	virtual ~CMoveObject() {}

	void move(int x, int y) { m_x = x; m_y = y; }
	void draw(sf::RenderWindow& RW, int left, int top);

	void set_name(const char str[]);
	void set_chat(const char str[]);
	void SetMaxHp(int maxhp) { m_maxHP = maxhp; }
	void SetCurHp(int curhp) { m_curHP = curhp; }
	void SetLevel(int level) { m_level = level; }

protected:
	int m_maxHP;
	int m_curHP;
	int m_level;
};

class CPlayer : public CMoveObject
{
public:
	CPlayer(sf::Texture& t, int x, int y, int x2, int y2);
	virtual ~CPlayer() {}

	void SetExp(int exp) { m_exp = exp; }

private:
	int m_exp;
};

class CNpc : public CMoveObject
{
public:
	CNpc(sf::Texture& t, int x, int y, int x2, int y2);
	virtual ~CNpc() {}
};

