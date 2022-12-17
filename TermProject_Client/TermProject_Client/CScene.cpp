#include "pch.h"
#include "CScene.h"
#include "CObject.h"
#include "CEffect.h"

extern sf::Font g_font;

CUserInterface::CUserInterface()
{
	//set levelname
	m_levelText.setFont(g_font);
	m_levelText.setFillColor(sf::Color(255, 255, 255));
	m_levelText.setPosition({ WINDOW_WIDTH + 45.f, 20.f });
	m_levelText.setCharacterSize(20);
	m_levelText.setStyle(sf::Text::Bold);

	//set hp interface
	m_barBG.setSize({ 100.f, 15.f });
	m_barBG.setFillColor(sf::Color(50, 50, 50, 200));
	m_barBG.setPosition({WINDOW_WIDTH + 30.f, 70.f});

	m_hpBar.setSize({ 100.f, 15.f });
	m_hpBar.setFillColor(sf::Color(250, 50, 50, 200));
	m_hpBar.setPosition({ WINDOW_WIDTH + 30.f, 70.f });

	m_hpText.setFont(g_font);
	m_hpText.setFillColor(sf::Color(255, 255, 255));
	m_hpText.setPosition({ WINDOW_WIDTH + 30.f, 65.f });
	m_hpText.setCharacterSize(16);
	m_hpText.setStyle(sf::Text::Bold);

	//set mp interface
	m_mpBar.setSize({ 100.f, 15.f });
	m_mpBar.setFillColor(sf::Color(50, 50, 250, 200));
	m_mpBar.setPosition({ WINDOW_WIDTH + 30.f, 95.f });

	m_mpText.setFont(g_font);
	m_mpText.setFillColor(sf::Color(255, 255, 255));
	m_mpText.setPosition({ WINDOW_WIDTH + 30.f, 90.f });
	m_mpText.setCharacterSize(16);
	m_mpText.setStyle(sf::Text::Bold);

	//set exp interface
	m_expBar.setSize({ 100.f, 15.f });
	m_expBar.setFillColor(sf::Color(50, 250, 50, 200));
	m_expBar.setPosition({ WINDOW_WIDTH + 30.f, 120.f });

	m_expText.setFont(g_font);
	m_expText.setFillColor(sf::Color(255, 255, 255));
	m_expText.setPosition({ WINDOW_WIDTH + 50.f, 132.f });
	m_expText.setCharacterSize(13);
	m_expText.setStyle(sf::Text::Bold);
	m_expText.setString("50/100");
	defaultpos = m_expText.getGlobalBounds().width;

	m_expText2.setFont(g_font);
	m_expText2.setFillColor(sf::Color(255, 255, 255));
	m_expText2.setPosition({ WINDOW_WIDTH + 60.f, 115.f });
	m_expText2.setCharacterSize(16);
	m_expText2.setStyle(sf::Text::Bold);
	m_expText2.setString("EXP");

	//set skill
	m_skillTex.loadFromFile("Resource/Skill.png");
	m_skill.setSize({ 96, 32 });
	m_skill.setTexture(&m_skillTex);
	m_skill.setTextureRect(sf::Rect(32, 0, TILE_WIDTH * 3, TILE_WIDTH));
	m_skill.setPosition(WINDOW_WIDTH + 25.f, 190.f);

	for (int i = 0; i < m_usedskill.size(); ++i) {
		m_usedskill[i].setFillColor(sf::Color(0, 0, 0, 0)); //used --> alpha 180
		m_usedskill[i].setSize({ 32, 32 });
		m_usedskill[i].setPosition(WINDOW_WIDTH + 25.f + i * TILE_WIDTH, 190.f);
	}

	m_skillText.setFont(g_font);
	m_skillText.setFillColor(sf::Color(255, 255, 255));
	m_skillText.setPosition({ WINDOW_WIDTH + 45.f, 170.f });
	m_skillText.setCharacterSize(20);
	m_skillText.setStyle(sf::Text::Bold);
	m_skillText.setString("SKILL");

	//set Item
	m_itemConTex.loadFromFile("Resource/ItemContainer.png");
	m_itemCon.setSize({ 108, 73 });
	m_itemCon.setTexture(&m_itemConTex);
	m_itemCon.setTextureRect(sf::Rect(0, 0, 108, 73));
	m_itemCon.setPosition(WINDOW_WIDTH + 25.f, 270.f);

	m_itemText.setFont(g_font);
	m_itemText.setFillColor(sf::Color(255, 255, 255));
	m_itemText.setPosition({ WINDOW_WIDTH + 52.f, 250.f });
	m_itemText.setCharacterSize(20);
	m_itemText.setStyle(sf::Text::Bold);
	m_itemText.setString("ITEM");

	//set attack
	m_attack.setSize({ 32, 32 });
	m_attack.setTexture(&m_skillTex);
	m_attack.setTextureRect(sf::Rect(0, 0, TILE_WIDTH, TILE_WIDTH));
	m_attack.setPosition(WINDOW_WIDTH + 59.f, 380.f);

	m_attackcool.setFillColor(sf::Color(0, 0, 0, 0)); //used --> alpha 180
	m_attackcool.setSize({ 32, 32 });
	m_attackcool.setPosition(WINDOW_WIDTH + 55.f, 380.f);

	m_attackText.setFont(g_font);
	m_attackText.setFillColor(sf::Color(255, 255, 255));
	m_attackText.setPosition({ WINDOW_WIDTH + 43.f, 360.f });
	m_attackText.setCharacterSize(20);
	m_attackText.setStyle(sf::Text::Bold);
	m_attackText.setString("ATTACK");
}

CUserInterface::~CUserInterface()
{
}

void CUserInterface::UpdateHp(int maxhp, int curhp)
{
	float percent = static_cast<float>(curhp) / static_cast<float>(maxhp) * 100;
	int p = static_cast<int>(percent);
	m_hpBar.setSize(sf::Vector2f(p, m_hpBar.getSize().y));

	string s = "HP:";
	s += to_string(curhp);
	m_hpText.setString(s);
}

void CUserInterface::UpdateMp(int maxmp, int curmp)
{
	float percent = static_cast<float>(curmp) / static_cast<float>(maxmp) * 100;
	int p = static_cast<int>(percent);
	m_mpBar.setSize(sf::Vector2f(p, m_mpBar.getSize().y));

	string s = "MP:";
	s += to_string(curmp);
	m_mpText.setString(s);
}

void CUserInterface::UpdateExp(int maxexp, int curexp)
{
	float percent = static_cast<float>(curexp) / static_cast<float>(maxexp) * 100;
	int p = static_cast<int>(percent);
	m_expBar.setSize(sf::Vector2f(p, m_expBar.getSize().y));

	string s = to_string(curexp);
	s += "/";
	s += to_string(maxexp);
	m_expText.setString(s);
	
	auto size = m_expText.getGlobalBounds();

	m_expText.setPosition({ WINDOW_WIDTH + 75.f - size.width / 2.f, m_expText.getPosition().y});
}

void CUserInterface::UpdateLevel(int level)
{
	string s = "LV:";
	s += to_string(level);
	m_levelText.setString(s);
}

void CUserInterface::SetSkillOnOff(int skill, bool type)
{
	switch (skill) {
	case 0:
		if (type) {
			m_attackcool.setFillColor(sf::Color(0, 0, 0, 180));
		}
		else {
			m_attackcool.setFillColor(sf::Color(0, 0, 0, 0));
		}
		break;
	case 1:
		if (type) {
			m_usedskill[0].setFillColor(sf::Color(0, 0, 0, 180));
		}
		else {
			m_usedskill[0].setFillColor(sf::Color(0, 0, 0, 0));
		}
		break;
	case 2:
		if (type) {
			m_usedskill[1].setFillColor(sf::Color(0, 0, 0, 180));
		}
		else {
			m_usedskill[1].setFillColor(sf::Color(0, 0, 0, 0));
		}
		break;
	case 3:
		if (type) {
			m_usedskill[2].setFillColor(sf::Color(0, 0, 0, 180));
		}
		else {
			m_usedskill[2].setFillColor(sf::Color(0, 0, 0, 0));
		}
		break;
	}
}

void CUserInterface::Render(sf::RenderWindow& RW)
{
	//Render level
	RW.draw(m_levelText);

	//Render hp
	m_barBG.setPosition({ WINDOW_WIDTH + 30.f, 70.f });
	RW.draw(m_barBG);
	RW.draw(m_hpBar);
	RW.draw(m_hpText);

	//Render mp
	m_barBG.setPosition({ WINDOW_WIDTH + 30.f, 95.f });
	RW.draw(m_barBG);
	RW.draw(m_mpBar);
	RW.draw(m_mpText);

	//Render exp
	m_barBG.setPosition({ WINDOW_WIDTH + 30.f, 120.f });
	RW.draw(m_barBG);
	RW.draw(m_expBar);
	RW.draw(m_expText2);
	RW.draw(m_expText);

	//Render skill
	RW.draw(m_skillText);
	RW.draw(m_skill);
	for (int i = 0; i < m_usedskill.size(); ++i) {
		RW.draw(m_usedskill[i]);
	}

	//Render item
	RW.draw(m_itemText);
	RW.draw(m_itemCon);

	//Render Attack
	RW.draw(m_attackText);
	RW.draw(m_attack);
	RW.draw(m_attackcool);
}

//////////////////////////////////////////////////////////////////////////////////////////

CScene::CScene()
{
	m_interface = make_unique<CUserInterface>();
	m_mapTile = new sf::Texture;
	m_character = new sf::Texture;
	m_items = new sf::Texture;
	m_skills = new sf::Texture;
	for (int i = 0; i < m_enemy.size(); ++i) {
		m_enemy[i] = new sf::Texture;
	}

	m_mapTile->loadFromFile("Resource/MapTiles.png");
	m_character->loadFromFile("Resource/Character.png");
	m_items->loadFromFile("Resource/Item.png");
	m_skills->loadFromFile("Resource/Skill.png");
	m_enemy[0]->loadFromFile("Resource/Enemy.png");

	m_avatar = new CPlayer{ *m_character, 0, 64, 32, 32};
	m_avatar->set_name("Test");

	m_objects = new CMoveObject*[MAX_USER + MAX_NPC];

	for (int i = 0; i < MAX_USER + MAX_NPC; ++i) {
		if (i < MAX_USER) {
			m_objects[i] = new CPlayer{ *m_character, 0, 64, 32, 32};
			m_objects[i]->hide();
		}
		else {
			m_objects[i] = new CNpc{ *m_enemy[0], 0, 64, 32, 32};
			m_objects[i]->hide();
		}
	}

	m_water_tile = new CObject{ *m_mapTile, 0, 0, 32, 32};
	m_grass_tile = new CObject{ *m_mapTile, 0, 32, 32, 32};
	m_rock_tile = new CObject{ *m_mapTile, 128, 0, 32, 32};
	m_tree_tile = new CObject{ *m_mapTile, 128, 32, 32, 32};
	m_dirt_tile = new CObject{ *m_mapTile, 32, 0, 32, 32};
	m_lake_tile = new CObject{ *m_mapTile, 128, 64, 32, 32};
	m_fire_tile = new CObject{ *m_mapTile, 0, 64, 32, 32};

	fstream in("Resource/Map.txt");
	for (int i = 0; i < 100; ++i) {
		for (int j = 0; j < 100; ++j) {
			char c;
			in >> c;
			m_tilemap[i][j] = c;
		}
	}

	for (int k = 0; k < 100; ++k) {
		for (int i = 1; i < 20; ++i) {
			for (int j = 0; j < 100; ++j) {
				m_tilemap[k][i * 100 + j] = m_tilemap[k][j];
			}
		}
	}

	for (int k = 100; k < W_HEIGHT; ++k) {
		for (int i = 0; i < 20; ++i) {
			for (int j = 0; j < 100; ++j) {
				m_tilemap[k][i * 100 + j] = m_tilemap[k - 100][j];
			}
		}
	}

	m_cooltime = {};
	m_effects[0] = new CAttackEffect("Resource/AttackEffect.png", 0.25f, 3);
	m_effects[1] = new CSkillEffect1("Resource/SkillEffect1.png", 0.25f, 7);
	m_effects[2] = new CSkillEffect2("Resource/SkillEffect2.png", 0.1f, 12);
	m_effects[3] = new CSkillEffect3("Resource/SkillEffect3.png", 0.1f, 10);	
}

CScene::~CScene()
{
	delete m_mapTile;
	delete m_character;
	delete m_items;
	delete m_skills;
	for (int i = 0; i < m_enemy.size(); ++i)
		delete m_enemy[i];
}

void CScene::Update(float ElapsedTime)
{
	for (int i = 0; i < m_effects.size(); ++i) {
		if (m_effects[i]) {
			if(m_effects[i]->GetEnable())
				m_effects[i]->Update(ElapsedTime);
		}
	}
}

void CScene::Render(sf::RenderWindow& RW)
{
	for (int i = 0; i < SCREEN_HEIGHT; ++i) {
		for (int j = 0; j < SCREEN_WIDTH; ++j) {
			int tile_x = j + m_left;
			int tile_y = i + m_top;
			if ((tile_x < 0) || (tile_y < 0))
				continue;
			switch (m_tilemap[tile_y][tile_x]) {
			case 'G':
				m_grass_tile->a_move(TILE_WIDTH * j + 7, TILE_WIDTH * i + 7);
				m_grass_tile->a_draw(RW);
				break;
			case 'D':
				m_dirt_tile->a_move(TILE_WIDTH * j + 7, TILE_WIDTH * i + 7);
				m_dirt_tile->a_draw(RW);
				break;
			case 'R':
				m_dirt_tile->a_move(TILE_WIDTH * j + 7, TILE_WIDTH * i + 7);
				m_dirt_tile->a_draw(RW);
				m_rock_tile->a_move(TILE_WIDTH * j + 7, TILE_WIDTH * i + 7);
				m_rock_tile->a_draw(RW);
				break;
			case 'W':
				m_water_tile->a_move(TILE_WIDTH * j + 7, TILE_WIDTH * i + 7);
				m_water_tile->a_draw(RW);
				break;
			case 'L':
				m_lake_tile->a_move(TILE_WIDTH * j + 7, TILE_WIDTH * i + 7);
				m_lake_tile->a_draw(RW);
				break;
			case 'T':
				m_grass_tile->a_move(TILE_WIDTH * j + 7, TILE_WIDTH * i + 7);
				m_grass_tile->a_draw(RW);
				m_tree_tile->a_move(TILE_WIDTH * j + 7, TILE_WIDTH * i + 7);
				m_tree_tile->a_draw(RW);
				break;
			case 'F':
				m_grass_tile->a_move(TILE_WIDTH * j + 7, TILE_WIDTH * i + 7);
				m_grass_tile->a_draw(RW);
				m_fire_tile->a_move(TILE_WIDTH * j + 7, TILE_WIDTH * i + 7);
				m_fire_tile->a_draw(RW);
				break;
			}
		}
	}
	m_avatar->draw(RW, m_left, m_top);
	for (int i = 0; i < MAX_USER + MAX_NPC; ++i) {
		if(m_objects[i] != NULL)
			m_objects[i]->draw(RW, m_left, m_top);
	}

	sf::Text text;
	text.setFont(g_font);
	char buf[100];
	sprintf_s(buf, "(%d, %d)", m_avatar->m_x, m_avatar->m_y);
	text.setString(buf);
	RW.draw(text);

	m_interface->Render(RW);

	for (int i = 0; i < m_effects.size(); ++i) {
		if (m_effects[i]) {
			if (m_effects[i]->GetEnable())
				m_effects[i]->Render(RW, m_avatar->m_x, m_avatar->m_y, m_left, m_top, m_avatar->GetDir());
		}
	}
}

void CScene::ProcessLoginInfoPacket(char* ptr)
{
	SC_LOGIN_INFO_PACKET* packet = reinterpret_cast<SC_LOGIN_INFO_PACKET*>(ptr);	
	m_id = packet->id;
	m_avatar->m_id = m_id;
	m_avatar->move(packet->x, packet->y);
	m_left = packet->x - 8;
	m_top = packet->y - 8;
	m_avatar->set_name(packet->name);
	m_avatar->SetCurHp(packet->hp);
	m_avatar->SetMaxHp(packet->max_hp);
	m_avatar->SetExp(packet->exp);
	m_avatar->SetLevel(packet->level);
	m_avatar->show();

	m_interface->UpdateHp(packet->max_hp, packet->hp);
	m_interface->UpdateLevel(packet->level);
	m_interface->UpdateExp(INIT_EXP + (packet->level-1) * EXP_UP, packet->exp);
	m_interface->UpdateMp(packet->max_mp, packet->mp);
}

void CScene::ProcessAddObjectPacket(char* ptr)
{
	SC_ADD_OBJECT_PACKET* packet = reinterpret_cast<SC_ADD_OBJECT_PACKET*>(ptr);
	int id = packet->id;

	if (id == m_id) {
		m_avatar->move(packet->x, packet->y);
		m_left = packet->x - 8;
		m_top = packet->y - 8;
		m_avatar->show();
	}
	else if (id < MAX_USER) {
		//m_objects[id] = new CObject{ *m_character, 0, 64, 32, 32};
		m_objects[id]->m_id = id;
		m_objects[id]->move(packet->x, packet->y);
		m_objects[id]->set_name(packet->name);
		m_objects[id]->show();
	}
	else {
		//m_objects[id] = new CObject{ *m_enemy, 0, 64, 32, 32};
		m_objects[id]->m_id = id;
		m_objects[id]->move(packet->x, packet->y);
		char newname[NAME_SIZE] = { '\0' };
		strcpy_s(newname, packet->name);
		m_objects[id]->set_name(newname);

		m_objects[id]->SetCurHp(packet->hp);
		m_objects[id]->SetMaxHp(packet->max_hp);
		m_objects[id]->SetLevel(packet->level);

		m_objects[id]->show();
	}
}

void CScene::ProcessMoveObjectPacket(char* ptr)
{
	SC_MOVE_OBJECT_PACKET* packet = reinterpret_cast<SC_MOVE_OBJECT_PACKET*>(ptr);
	int other_id = packet->id;
	if (other_id == m_id) {
		m_avatar->move(packet->x, packet->y);
		m_left = packet->x - 8;
		m_top = packet->y - 8;
		/*g_left_x = packet->x - SCREEN_WIDTH / 2;
		g_top_y = packet->y - SCREEN_HEIGHT / 2;*/
	}
	else {
		if (m_objects[other_id]->m_x == packet->x) {
			if (m_objects[other_id]->m_y > packet->y) {
				m_objects[other_id]->ChangeTex(0, TILE_WIDTH * 0, TILE_WIDTH, TILE_WIDTH);
			}
			else {
				m_objects[other_id]->ChangeTex(0, TILE_WIDTH * 2, TILE_WIDTH, TILE_WIDTH);
			}
		}
		else {
			if (m_objects[other_id]->m_x > packet->x) {
				m_objects[other_id]->ChangeTex(0, TILE_WIDTH * 3, TILE_WIDTH, TILE_WIDTH);
			}
			else {
				m_objects[other_id]->ChangeTex(0, TILE_WIDTH * 1, TILE_WIDTH, TILE_WIDTH);
			}
		}
		m_objects[other_id]->move(packet->x, packet->y);
	}
}

void CScene::ProcessRemoveObjectPacket(char* ptr)
{
	SC_REMOVE_OBJECT_PACKET* packet = reinterpret_cast<SC_REMOVE_OBJECT_PACKET*>(ptr);
	int other_id = packet->id;
	if (other_id == m_id) {
		m_avatar->hide();
	}
	else {
		m_objects[other_id]->hide();
	}
}

void CScene::ProcessChatPacket(char* ptr)
{
	SC_CHAT_PACKET* packet = reinterpret_cast<SC_CHAT_PACKET*>(ptr);
	int other_id = packet->id;
	if (other_id == m_id) {
		m_avatar->set_chat(packet->mess);
	}
	else {
		m_objects[other_id]->set_chat(packet->mess);
	}
}

void CScene::ProcessDamagePacket(char* ptr)
{
	SC_DAMAGE_PACKET* packet = reinterpret_cast<SC_DAMAGE_PACKET*>(ptr);
	m_objects[packet->id]->SetCurHp(packet->hp);
	dynamic_cast<CNpc*>(m_objects[packet->id])->UpdateHPBar();
}

void CScene::ProcessStatChangePacket(char* ptr)
{
	SC_STAT_CHANGE_PACKET* p = reinterpret_cast<SC_STAT_CHANGE_PACKET*>(ptr);

	m_interface->UpdateLevel(p->level);
	m_interface->UpdateHp(p->max_hp, p->hp);
	m_interface->UpdateMp(p->max_mp, p->mp);
	m_interface->UpdateExp(INIT_EXP + (p->level-1) * EXP_UP, p->exp);
}

void CScene::ChangeAvartarTex(int x, int y, int x2, int y2)
{
	m_avatar->ChangeTex(x, y, x2, y2);
}

void CScene::SetSkillOnOff(int skill, bool type)
{
	switch (skill) {
	case 0:
	case 1:
	case 2:
	case 3:
		m_interface->SetSkillOnOff(skill, type);
		break;
	default:
		break;
	}
}

void CScene::SetEffectEnable(int index, bool type)
{
	m_effects[index]->SetEnable(type);
}

void CScene::SetDir(DIR dir)
{
	m_avatar->SetDir(dir);
}

const bool CScene::GetEffectEnable(int index) const
{
	return m_effects[index]->GetEnable();
}
