#pragma once

class CObject;
class COverlapEx;
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

	void RegisterEvent(const TIMER_EVENT& ev) { m_timerQueue.push(ev); }
	vector<int> GetClientsCanSeeNpc(int npcID);
	
private:
	void Accept(int id, int bytes, COverlapEx* over_ex);
	void MonsterRespawn(int id, int bytes, COverlapEx* over_ex);
	void PlayerHeal(int id, int bytes, COverlapEx* over_ex);
	void NpcMove(int id, int bytes, COverlapEx* over_ex);
	void PowerUpRollBack(int id, int bytes, COverlapEx* over_ex);
	void Recv(int id, int bytes, COverlapEx* over_ex);
	void Send(int id, int bytes, COverlapEx* over_ex);

	int GetNewID();
	void Disconnect(int id);

private:
	HANDLE m_iocp;
	SOCKET m_listenSock;
	SOCKET m_clientSock;
	std::queue<SOCKET> m_socketpool;
	unordered_map<SOCKET, int> m_idMap;

	std::unordered_map<OP_TYPE, std::function<void(int, int, COverlapEx*)>> m_iocpfunc;


	concurrency::concurrent_priority_queue<TIMER_EVENT> m_timerQueue;

	array<CObject*, MAX_USER + MAX_NPC> m_objects;

	COverlapEx* m_over;

	CDatabase* m_database;
};

