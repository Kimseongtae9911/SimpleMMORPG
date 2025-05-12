#pragma once

constexpr int SECTION_SIZE = VIEW_RANGE * 2;
constexpr int SECTION_NUM = static_cast<int>((W_WIDTH / SECTION_SIZE + 1));

struct Section {
	shared_mutex sectionLock;
	unordered_set<int> objects;
};


class GameUtil
{
public:
	static bool InitailzeData();

	static bool CanMove(short x, short y);
	
	static char GetTile(int x, int y) { return tilemap[x][y]; }
	static ITEM_TYPE GetItemTile(int x, int y) { return itemmap[x][y]; }
	static void SetItemTile(int x, int y, ITEM_TYPE item) { itemmap[x][y] = item; }

	static std::vector<int> GetSectionObjects(int y, int x);
	static void RegisterToSection(int beforeY, int berforeX, int y, int x, int id);

private:
	static char tilemap[W_HEIGHT][W_WIDTH];
	static ITEM_TYPE itemmap[W_HEIGHT][W_WIDTH];
	static array<array<Section, SECTION_NUM>, SECTION_NUM> sections;
};

