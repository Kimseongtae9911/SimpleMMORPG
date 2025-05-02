#pragma once
class CServer
{
public:
	CServer();
	~CServer();

	bool Initialize();
	bool Release();
	void Run();

private:
	std::vector<std::thread> m_networkThreads;
	std::vector<std::thread> m_workers;
};

