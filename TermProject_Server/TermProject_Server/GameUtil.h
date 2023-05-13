#pragma once
class GameUtil
{
public:
	static bool InitailzeData();

	static bool CanMove(short x, short y, char dir);
	
	static char GetTile(int x, int y) { return tilemap[x][y]; }
	static ITEM_TYPE GetItemTile(int x, int y) { return itemmap[x][y]; }
	static void SetItemTile(int x, int y, ITEM_TYPE item) { itemmap[x][y] = item; }

private:
	static char tilemap[W_HEIGHT][W_WIDTH];
	static ITEM_TYPE itemmap[W_HEIGHT][W_WIDTH];
};

