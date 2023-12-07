#include "pch.h"
#include "GameUtil.h"

char GameUtil::tilemap[W_HEIGHT][W_WIDTH];
ITEM_TYPE GameUtil::itemmap[W_HEIGHT][W_WIDTH] = { NONE, };
array<array<Section, SECTION_NUM>, SECTION_NUM> GameUtil::sections;

bool GameUtil::InitailzeData()
{
	cout << "Uploading Map Data" << endl;
	fstream in("Data/Map.txt");
	for (int i = 0; i < 100; ++i) {
		for (int j = 0; j < 100; ++j) {
			char c;
			in >> c;
			tilemap[i][j] = c;
		}
	}
	for (int k = 0; k < 100; ++k) {
		for (int i = 1; i < 20; ++i) {
			for (int j = 0; j < 100; ++j) {
				tilemap[k][i * 100 + j] = tilemap[k][j];
			}
		}
	}
	for (int k = 100; k < W_HEIGHT; ++k) {
		for (int i = 0; i < 20; ++i) {
			for (int j = 0; j < 100; ++j) {
				tilemap[k][i * 100 + j] = tilemap[k - 100][j];
			}
		}
	}
	cout << "Map Upload Complete" << endl;

	return true;
}

bool GameUtil::CanMove(short x, short y)
{
	if (x < 0 || x >= W_WIDTH - 1 || y < 0 || y >= W_HEIGHT - 1)
		return false;

	if (tilemap[y][x] != 'G' && tilemap[y][x] != 'D')
		return false;

	return true;
}

unordered_set<int> GameUtil::GetSectionObjects(int x, int y)
{
	sections[x][y].sectionLock.lock_shared();
	unordered_set<int> objects(sections[x][y].objects);
	sections[x][y].sectionLock.unlock_shared();

	return objects;
}

void GameUtil::RegisterToSection(int beforeX, int berforeY, int x, int y, int id)
{
	if (beforeX != -1) {
		sections[x][y].sectionLock.lock();
		sections[x][y].objects.erase(id);
		sections[x][y].sectionLock.unlock();
	}
	sections[x][y].sectionLock.lock(); 
	sections[x][y].objects.insert(id); 
	sections[x][y].sectionLock.unlock();
}
