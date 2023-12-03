#include "pch.h"
#include "CObject.h"
#include "CNetworkMgr.h"

CObject::CObject()
{
	m_ID = -1;
	m_PosX = m_PosY = 0;
	memset(m_name, 0, sizeof(m_name));
}

CObject::~CObject()
{
}

bool CObject::CanSee(int to) const
{
	if (abs(GetPosX() - CNetworkMgr::GetInstance()->GetCObject(to)->GetPosX()) > VIEW_RANGE)
		return false;

	return abs(GetPosY() - CNetworkMgr::GetInstance()->GetCObject(to)->GetPosY()) <= VIEW_RANGE;
}