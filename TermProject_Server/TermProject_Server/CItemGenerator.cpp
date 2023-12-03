#include "pch.h"
#include "CItemGenerator.h"

std::unique_ptr<CItemGenerator> CItemGenerator::m_instance;

bool CItemGenerator::Initialize()
{
	return false;
}

bool CItemGenerator::Release()
{
	return false;
}

random_device rd;
default_random_engine dre{ rd() };
ITEM_TYPE CItemGenerator::CreateItem(short x, short y)
{
	uniform_int_distribution<> uid;
	int item = uid(dre) % 100;	//Nothing:20%, Money:50%, Potion:12%/12%, Weapon:2%/2%/2%

	ITEM_TYPE itemType = ITEM_TYPE::NONE;

	if (item <= 49) { // Money
		itemType = ITEM_TYPE::MONEY;
	}
	else if (item >= 50 && item <= 67) { // Nothing

	}
	else if (item >= 68 && item <= 69) {
		itemType = ITEM_TYPE::HAT;
	}
	else if (item >= 70 && item <= 81) { // HP Potion
		itemType = ITEM_TYPE::HP_POTION;
	}
	else if (item >= 82 && item <= 93) { // MP Potion
		itemType = ITEM_TYPE::MP_POTION;
	}
	else if (item >= 94 && item <= 95) { // Wand
		itemType = ITEM_TYPE::WAND;
	}
	else if (item >= 96 && item <= 97) { // Cloth
		itemType = ITEM_TYPE::CLOTH;
	}
	else {	// ring
		itemType = ITEM_TYPE::RING;
	}

	return itemType;
}