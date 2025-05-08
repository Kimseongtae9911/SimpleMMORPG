#include "pch.h"
#include "CServer.h"
#include "CNetworkMgr.h"
#include "Global.h"
#include "JobQueue.h"

CServer::CServer()
{
}

CServer::~CServer()
{
}

bool CServer::Initialize()
{
	if (false == CNetworkMgr::GetInstance()->Initialize()) {
		cout << "NetworkMgr Initialize Bug" << endl;
		return false;
	}
	
	cout << "Server Initialize Finish" << endl;
	return true;
}

bool CServer::Release()
{
    return true;
}

void CServer::Run()
{
	m_workers.reserve(3);
	m_networkThreads.reserve(3);
	for (unsigned int i = 0; i < 5; ++i) {
		m_networkThreads.emplace_back([this]() {CNetworkMgr::GetInstance()->IOCPFunc(); });
	}

	for (unsigned int i = 0; i < 5; ++i) {
		m_workers.emplace_back([]() {GPacketJobQueue->ProcessJob(); });
	}

	std::thread timer{ [this]() {CNetworkMgr::GetInstance()->TimerFunc(); } };
	timer.join();

	for (unsigned int i = 0; i < 3; ++i) {
		m_workers[i].join();
	}

	for (unsigned int i = 0; i < 3; ++i) {
		m_networkThreads[i].join();
	}
}