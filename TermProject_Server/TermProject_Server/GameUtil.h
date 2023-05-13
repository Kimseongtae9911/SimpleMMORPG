#pragma once
class GameUtil
{
public:
	static bool InitailzeData();

	static bool CanMove(short x, short y, char dir);
	
	static char GetTile(int x, int y) { return tilemap[x][y]; }

private:
	static char tilemap[W_HEIGHT][W_WIDTH];
};

