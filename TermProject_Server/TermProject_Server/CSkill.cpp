#include "pch.h"
#include "CSkill.h"
#include "CClient.h"
#include "CNetworkMgr.h"

void CAttack::Use(int id)
{
	CClient* myClient = reinterpret_cast<CClient*>(CNetworkMgr::GetInstance()->GetCObject(id));
	
	myClient->m_ViewLock.lock_shared();
	auto viewList = myClient->GetViewList();
	myClient->m_ViewLock.unlock_shared();

	int posX = myClient->GetPosX();
	int posY = myClient->GetPosY();

	array<pair<int, int>, 4> attackDir;
	attackDir[0] = { 0, 1 };
	attackDir[1] = { 0, -1 };
	attackDir[2] = { 1, 0 };
	attackDir[3] = { -1, 0 };

	for (const auto viewID : viewList) {
		CObject* object = CNetworkMgr::GetInstance()->GetCObject(viewID);
		if (viewID < MAX_USER)
			continue;

		for (auto& [dirX, dirY] : attackDir) {
			if (object->GetPosX() == posX + dirX && object->GetPosY() == posY + dirY) {
				object->Damaged(myClient->GetStat()->GetPower() * 2, id);
			}
		}
	}
}

void CPowerUp::Use(int id)
{
	CObject* myClient = CNetworkMgr::GetInstance()->GetCObject(id);
	int power = myClient->GetStat()->GetPower();
	myClient->GetStat()->SetPower(power * 2);

	CNetworkMgr::GetInstance()->RegisterEvent({ id, chrono::system_clock::now() + chrono::seconds(POWERUP_TIME), EVENT_TYPE::EV_POWERUP_ROLLBACK, power });
}

void CFire::Use(int id)
{
	CClient* myClient = reinterpret_cast<CClient*>(CNetworkMgr::GetInstance()->GetCObject(id));

	myClient->m_ViewLock.lock_shared();
	auto viewList = myClient->GetViewList();
	myClient->m_ViewLock.unlock_shared();

	int posX = myClient->GetPosX();
	int posY = myClient->GetPosY();

	//Collision
	for (const auto viewID : viewList) {
		if (viewID < MAX_USER)
			continue;

		CObject* target = CNetworkMgr::GetInstance()->GetCObject(viewID);
		int targetID = -1;
		for (int i = 1; i < 5; ++i) {
			switch (myClient->GetDir()) {
			case DIR::DOWN:
				if (target->GetPosX() == posX && target->GetPosY() - i == posY) {
					targetID = viewID;
				}
				break;
			case DIR::UP:
				if (target->GetPosX() == posX && target->GetPosY() + i == posY) {
					targetID = viewID;
				}
				break;
			case DIR::LEFT:
				if (target->GetPosX() + i == posX && target->GetPosY() == posY) {
					targetID = viewID;
				}
				break;
			case DIR::RIGHT:
				if (target->GetPosX() - i == posX && target->GetPosY() == posY) {
					targetID = viewID;
				}
				break;
			}

			if (-1 != targetID) {
				target->Damaged(myClient->GetStat()->GetPower() * 4, id);
			}
		}
	}
}

void CExplosion::Use(int id)
{
	CClient* myClient = reinterpret_cast<CClient*>(CNetworkMgr::GetInstance()->GetCObject(id));

	myClient->m_ViewLock.lock_shared();
	auto viewList = myClient->GetViewList();
	myClient->m_ViewLock.unlock_shared();

	int posX = myClient->GetPosX();
	int posY = myClient->GetPosY();

	for (const auto viewID : viewList) {
		if (viewID < MAX_USER)
			continue;

		CObject* target = CNetworkMgr::GetInstance()->GetCObject(viewID);

		for (int i = -2; i < 3; ++i) {
			for (int j = -2; j < 3; ++j) {
				if (target->GetPosX() + i == posX && target->GetPosY() + j == posY) {
					target->Damaged(myClient->GetStat()->GetPower() * 3, id);
				}
			}
		}
	}
}

bool CSkill::CanUse(int mp)
{
	if (chrono::duration_cast<chrono::seconds>(chrono::system_clock::now() - m_usedTime).count() >= m_coolTime &&
		mp >= m_mpConsumption) {
		m_usedTime = chrono::system_clock::now();
		return true;
	}
	return false;
}
