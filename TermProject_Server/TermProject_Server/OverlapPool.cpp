#include "pch.h"
#include "OverlapPool.h"
#include "COverlapEx.h"

concurrency::concurrent_priority_queue<COverlapEx*> OverlapPool::m_overlapPool;

COverlapEx* OverlapPool::GetOverlapFromPool()
{
	COverlapEx* overEx;
	if (!m_overlapPool.try_pop(overEx)) {
		overEx = new COverlapEx;
	}
	return overEx;
}
