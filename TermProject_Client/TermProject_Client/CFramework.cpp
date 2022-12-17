#include "pch.h"
#include "CFramework.h"
#include "CPacketMgr.h"
#include "CScene.h"

CFramework::CFramework(const char* ip) : m_window(sf::VideoMode(WINDOW_WIDTH + 150, WINDOW_HEIGHT), "Simple MMORPG")
{
	m_scene = make_shared<CScene>();
	m_packetMgr = make_unique<CPacketMgr>(ip, m_scene);
	
	string name;
	cout << "Name을 입력하세요 : ";
	cin >> name;

	CS_LOGIN_PACKET p;
	p.size = sizeof(p);
	p.type = CS_LOGIN;
	strcpy_s(p.name, name.c_str());
	m_packetMgr->SendPacket(&p);

	m_window.setFramerateLimit(60);
}

CFramework::~CFramework()
{
}

void CFramework::Process()
{
	while (m_window.isOpen())
	{
		auto time = chrono::system_clock::now();
		if (chrono::duration_cast<chrono::seconds>(time - m_scene->GetCoolTime(0)).count() >= ATTACK_COOL) {
			m_scene->SetSkillOnOff(0, false);
		}
		if (chrono::duration_cast<chrono::seconds>(time - m_scene->GetCoolTime(1)).count() >= SKILL1_COOL) {
			m_scene->SetSkillOnOff(1, false);
		}
		if (chrono::duration_cast<chrono::seconds>(time - m_scene->GetCoolTime(2)).count() >= SKILL2_COOL) {
			m_scene->SetSkillOnOff(2, false);
		}
		if (chrono::duration_cast<chrono::seconds>(time - m_scene->GetCoolTime(3)).count() >= SKILL3_COOL) {
			m_scene->SetSkillOnOff(3, false);
		}
		if (true == m_scene->GetEffectEnable(1)) {
			if (chrono::duration_cast<chrono::seconds>(time - m_scene->GetCoolTime(1)).count() >= POWERUP_TIME) {
				m_scene->SetEffectEnable(1, false);
			}
		}
		sf::Event event;		
		while (m_window.pollEvent(event))
		{
			if (event.type == sf::Event::Closed)
				m_window.close();
			if (event.type == sf::Event::KeyPressed) {
				int direction = -1;
				char skill = -1;
				switch (event.key.code) {
				case sf::Keyboard::Left:
					direction = 2;
					m_scene->ChangeAvartarTex(0, TILE_WIDTH * 3, TILE_WIDTH, TILE_WIDTH);
					m_scene->SetDir(DIR::LEFT);
					break;
				case sf::Keyboard::Right:
					direction = 3;
					m_scene->ChangeAvartarTex(0, TILE_WIDTH * 1, TILE_WIDTH, TILE_WIDTH);
					m_scene->SetDir(DIR::RIGHT);
					break;
				case sf::Keyboard::Up:
					direction = 0;
					m_scene->ChangeAvartarTex(0, TILE_WIDTH * 0, TILE_WIDTH, TILE_WIDTH);
					m_scene->SetDir(DIR::UP);
					break;
				case sf::Keyboard::Down:
					m_scene->ChangeAvartarTex(0, TILE_WIDTH * 2, TILE_WIDTH, TILE_WIDTH);
					direction = 1;
					m_scene->SetDir(DIR::DOWN);
					break;
				case sf::Keyboard::Escape:
					m_window.close();
					break;
				case sf::Keyboard::A:
					if (chrono::duration_cast<chrono::seconds>(time - m_scene->GetCoolTime(0)).count() >= ATTACK_COOL) {
						skill = 0;
						m_scene->SetCoolTime(0, time);
						m_scene->SetSkillOnOff(0, true);
						m_scene->SetEffectEnable(0, true);
					}
					break;
				case sf::Keyboard::S:
					if (chrono::duration_cast<chrono::seconds>(time - m_scene->GetCoolTime(1)).count() >= SKILL1_COOL) {
						skill = 1;
						m_scene->SetCoolTime(1, time);
						m_scene->SetSkillOnOff(1, true);
						m_scene->SetEffectEnable(1, true);
					}
					break;
				case sf::Keyboard::D:
					if (chrono::duration_cast<chrono::seconds>(time - m_scene->GetCoolTime(2)).count() >= SKILL2_COOL) {
						skill = 2;
						m_scene->SetCoolTime(2, time);
						m_scene->SetSkillOnOff(2, true);
						m_scene->SetEffectEnable(2, true);
					}
					break;
				case sf::Keyboard::F:
					if (chrono::duration_cast<chrono::seconds>(time - m_scene->GetCoolTime(3)).count() >= SKILL3_COOL) {
						skill = 3;
						m_scene->SetCoolTime(3, time);
						m_scene->SetSkillOnOff(3, true);
						m_scene->SetEffectEnable(3, true);
					}
					break;
				}
				if (-1 != direction) {
					CS_MOVE_PACKET p;
					p.size = sizeof(p);
					p.type = CS_MOVE;
					p.direction = direction;
					m_packetMgr->SendPacket(&p);
				}
				if (-1 != skill) {
					CS_ATTACK_PACKET p;
					p.size = sizeof(p);
					p.type = CS_ATTACK;
					p.skill = skill;
					p.time = chrono::system_clock::now();
					m_packetMgr->SendPacket(&p);
				}
			}
		}
		m_window.clear();
		Update();
		Render();
		m_window.display();
	}
}

void CFramework::Update()
{
	float ElapsedTime = m_sfClock.getElapsedTime().asSeconds();
	m_packetMgr->RecvPacket();
	m_scene->Update(ElapsedTime);
	m_sfClock.restart();
}

void CFramework::Render()
{
	m_scene->Render(m_window);
}
