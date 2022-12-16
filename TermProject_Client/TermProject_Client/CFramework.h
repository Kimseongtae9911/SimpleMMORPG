#pragma once

class CPacketMgr;
class CScene;

class CFramework
{
public: 
	CFramework(const char* ip);
	~CFramework();

	void Process();

private:
	void Update();
	void Render();

private:
	sf::RenderWindow m_window = {};

	unique_ptr<CPacketMgr> m_packetMgr;
	shared_ptr<CScene> m_scene;
};

