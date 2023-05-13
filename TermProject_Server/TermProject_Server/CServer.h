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
	void WorkerFunc();
	void TimerFunc();

private:
	std::vector<std::thread> m_workers;
};

