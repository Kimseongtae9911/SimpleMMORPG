#pragma once
class CItemGenerator
{
	SINGLETON(CItemGenerator);

public:
	bool Initialize();
	bool Release();

	ITEM_TYPE CreateItem(short x, short y);
};