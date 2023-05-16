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

bool GameUtil::CanMove(short x, short y, char dir)
{
	bool canmove = true;
	switch (dir) {
	case 0:
		if (y > 0) {
			y--;
			if (tilemap[y][x] != 'G' && tilemap[y][x] != 'D')
				canmove = false;
		}
		else {
			canmove = false;
		}
		break;
	case 1:
		if (y < W_HEIGHT - 1) {
			y++;
			if (tilemap[y][x] != 'G' && tilemap[y][x] != 'D')
				canmove = false;
		}
		else {
			canmove = false;
		}
		break;
	case 2:
		if (x > 0) {
			x--;
			if (tilemap[y][x] != 'G' && tilemap[y][x] != 'D')
				canmove = false;
		}
		else {
			canmove = false;
		}
		break;
	case 3:
		if (x < W_WIDTH - 1) {
			x++;
			if (tilemap[y][x] != 'G' && tilemap[y][x] != 'D')
				canmove = false;
		}
		else {
			canmove = false;
		}
		break;
	}

	return canmove;

}

vector<int> GameUtil::GetSectionObjects(int x, int y)
{
	sections[x][y].sectionLock.lock_shared();
	vector<int> objects(sections[x][y].objects);
	sections[x][y].sectionLock.unlock_shared();

	return objects;
}
