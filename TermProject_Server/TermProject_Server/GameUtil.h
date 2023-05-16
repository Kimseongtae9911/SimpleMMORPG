#pragma once

constexpr int SECTION_SIZE = VIEW_RANGE * 2;
constexpr int SECTION_NUM = static_cast<int>((W_WIDTH / SECTION_SIZE + 1));

struct Section {
	shared_mutex sectionLock;
	vector<int> objects;
};

class GameUtil
{
public:
	static bool InitailzeData();

	static bool CanMove(short x, short y, char dir);
	
	static char GetTile(int x, int y) { return tilemap[x][y]; }
	static ITEM_TYPE GetItemTile(int x, int y) { return itemmap[x][y]; }
	static void SetItemTile(int x, int y, ITEM_TYPE item) { itemmap[x][y] = item; }

	static vector<int>& GetSectionObjects(int x, int y);
	static void RegisterToSection(int x, int y, int id) { sections[x][y].sectionLock.lock_shared(); sections[x][y].objects.push_back(id); sections[x][y].sectionLock.unlock_shared();}

private:
	static char tilemap[W_HEIGHT][W_WIDTH];
	static ITEM_TYPE itemmap[W_HEIGHT][W_WIDTH];
	static array<array<Section, SECTION_NUM>, SECTION_NUM> sections;
};

