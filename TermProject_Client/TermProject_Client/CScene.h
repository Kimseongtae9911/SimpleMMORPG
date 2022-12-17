#pragma once

#define ATTACK_COOL 1.f
#define SKILL1_COOL 10.f
#define SKILL2_COOL 7.f
#define SKILL3_COOL 5.f

class CObject;
class CPlayer;
class CMoveObject;

class CUserInterface
{
public:
	CUserInterface();
	~CUserInterface();

	void Render(sf::RenderWindow& RW);

	void UpdateHp(int maxhp, int curhp);
	void UpdateMp(int maxmp, int curmp);
	void UpdateExp(int maxexp, int curexp);
	void UpdateLevel(int level);

	void SetSkillOnOff(int skill, bool type);

private:
	sf::Texture m_skillTex;
	sf::RectangleShape m_skill;
	array<sf::RectangleShape,3 > m_usedskill;

	sf::Texture m_itemConTex;
	sf::RectangleShape m_itemCon;

	sf::RectangleShape m_attack;
	sf::RectangleShape m_attackcool;

	sf::RectangleShape m_barBG;
	sf::RectangleShape m_hpBar;
	sf::RectangleShape m_mpBar;
	sf::RectangleShape m_expBar;
	
	sf::Text m_expText;
	sf::Text m_expText2;
	sf::Text m_hpText;
	sf::Text m_mpText;
	sf::Text m_levelText;
	sf::Text m_skillText;
	sf::Text m_itemText;
	sf::Text m_attackText;

	int defaultpos = 0;
};

class CScene
{
public:
	CScene();
	~CScene();

	void Update();
	void Render(sf::RenderWindow& RW);
	
	void ProcessLoginInfoPacket(char* ptr);
	void ProcessAddObjectPacket(char* ptr);
	void ProcessMoveObjectPacket(char* ptr);
	void ProcessRemoveObjectPacket(char* ptr);
	void ProcessChatPacket(char* ptr);
	void ProcessDamagePacket(char* ptr);

	void ChangeAvartarTex(int x, int y, int x2, int y2);
	void SetSkillOnOff(int skill, bool type);

	const chrono::system_clock::time_point GetCoolTime(int index) const { return m_cooltime[index]; }

	void SetCoolTime(int index, chrono::system_clock::time_point time) { m_cooltime[index] = time; }
	

private:
	int m_left;
	int m_top;
	int m_id;
	array<chrono::system_clock::time_point, 4> m_cooltime;

	CPlayer* m_avatar;
	CMoveObject** m_objects;

	unique_ptr<CUserInterface> m_interface;	

	sf::Texture* m_mapTile;
	sf::Texture* m_character;
	array<sf::Texture*, 3> m_enemy;
	sf::Texture* m_items;
	sf::Texture* m_skills;

	CObject* m_water_tile;
	CObject* m_grass_tile;
	CObject* m_rock_tile;
	CObject* m_tree_tile;
	CObject* m_dirt_tile;
	CObject* m_fire_tile;
	CObject* m_lake_tile;

	char m_tilemap[W_HEIGHT][W_WIDTH];
};