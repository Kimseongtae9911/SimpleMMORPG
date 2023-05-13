#pragma once

class CObject;
class OVER_EXP;
class CDatabase;

class CNetworkMgr
{
	SINGLETON(CNetworkMgr);

public:
	bool Initialize();
	bool Release();

	void WorkerFunc();
	void TimerFunc();

	CObject* GetCObject(int id) const { return m_objects[id]; }
	const array<CObject*, MAX_USER + MAX_NPC>& GetAllObject() const { return m_objects; }

	CDatabase* GetDatabase() const { return m_database; }

private:
	void Accept(int id, int bytes, OVER_EXP* over_ex);
	void MonsterRespawn(int id, int bytes, OVER_EXP* over_ex);
	void PlayerHeal(int id, int bytes, OVER_EXP* over_ex);
	void NpcMove(int id, int bytes, OVER_EXP* over_ex);
	void Recv(int id, int bytes, OVER_EXP* over_ex);
	void Send(int id, int bytes, OVER_EXP* over_ex);

	int GetNewID();
	void Disconnect(int id);

private:
	HANDLE m_iocp;
	SOCKET m_listenSock;
	SOCKET m_clientSock;
	std::queue<SOCKET> m_socketpool;

	std::unordered_map<OP_TYPE, std::function<void(int, int, OVER_EXP*)>> m_iocpfunc;
	concurrency::concurrent_priority_queue<TIMER_EVENT> m_timerQueue;

	array<CObject*, MAX_USER + MAX_NPC> m_objects;

	OVER_EXP* m_over;

	CDatabase* m_database;
};

