#include "pch.h"
#include "CServer.h"
#include "CNetworkMgr.h"

CServer::CServer()
{
}

CServer::~CServer()
{
}

bool CServer::Initialize()
{
	CNetworkMgr::GetInstance()->Initialize();

    return false;
}

bool CServer::Release()
{
    return false;
}

void CServer::Run()
{
	unsigned int threadNum = std::thread::hardware_concurrency() * 2l;

	m_workers.reserve(threadNum);
	for (unsigned int i = 0; i < threadNum - 3; ++i) {
		m_workers.emplace_back([this]() {WorkerFunc(); });
	}

	std::thread timer{ [this]() {TimerFunc(); } };
	timer.join();

	for (unsigned int i = 0; i < threadNum - 2; ++i) {
		m_workers[i].join();
	}
}

void CServer::WorkerFunc()
{
	CNetworkMgr::GetInstance()->WorkerFunc();
}

void CServer::TimerFunc()
{
	CNetworkMgr::GetInstance()->TimerFunc();
}
