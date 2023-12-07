#pragma once

class COverlapEx;
class OverlapPool
{
public:
	static COverlapEx* GetOverlapFromPool();
	static void RegisterToPool(COverlapEx* over) { m_overlapPool.push(over); }

private:
	static concurrency::concurrent_priority_queue<COverlapEx*> m_overlapPool;
};

