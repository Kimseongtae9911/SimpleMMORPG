#include "pch.h"
#include "CFramework.h"
#include "CPacketMgr.h"
#include "CScene.h"

CFramework::CFramework(const char* ip) : m_window(sf::VideoMode(WINDOW_WIDTH + 150, WINDOW_HEIGHT), "Simple MMORPG")
{
	m_scene = make_shared<CScene>();
	m_packetMgr = make_unique<CPacketMgr>(ip, m_scene);

	string name;
	cout << "Name�� �Է��ϼ��� : ";
	cin >> name;

	CS_LOGIN_PACKET p;
	p.size = sizeof(p);
	p.type = CS_LOGIN;
	strcpy_s(p.name, name.c_str());
	m_packetMgr->SendPacket(&p);
}

CFramework::~CFramework()
{
}

void CFramework::Process()
{
	while (m_window.isOpen())
	{
		auto time = chrono::system_clock::now();
		if (chrono::duration_cast<chrono::seconds>(time - m_scene->GetCoolTime(0)).count() >= 1.f) {
			m_scene->SetSkillOnOff(0, false);
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
					break;
				case sf::Keyboard::Right:
					direction = 3;
					m_scene->ChangeAvartarTex(0, TILE_WIDTH * 1, TILE_WIDTH, TILE_WIDTH);
					break;
				case sf::Keyboard::Up:
					direction = 0;
					m_scene->ChangeAvartarTex(0, TILE_WIDTH * 0, TILE_WIDTH, TILE_WIDTH);
					break;
				case sf::Keyboard::Down:
					m_scene->ChangeAvartarTex(0, TILE_WIDTH * 2, TILE_WIDTH, TILE_WIDTH);
					direction = 1;
					break;
				case sf::Keyboard::Escape:
					m_window.close();
					break;
				case sf::Keyboard::A:
					if (chrono::duration_cast<chrono::seconds>(time - m_scene->GetCoolTime(0)).count() >= 1.f) {
						skill = 0;
						m_scene->SetCoolTime(0, time);
						m_scene->SetSkillOnOff(0, true);
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
	m_packetMgr->RecvPacket();
}

void CFramework::Render()
{
	m_scene->Render(m_window);
}