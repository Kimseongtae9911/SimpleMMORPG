#pragma once

constexpr int SECTION_SIZE = VIEW_RANGE * 2;
constexpr int SECTION_NUM = static_cast<int>((W_WIDTH / SECTION_SIZE + 1));

class Section {
public:
    using ObjectSet = std::unordered_set<int>;

    Section() {
        objects.store(std::make_shared<ObjectSet>());
		m_objectPool.push(std::make_shared<ObjectSet>());
    }

    void Insert(int id) {
        while (true) {
            auto current = objects.load();
            if (current->find(id) != current->end())
                return;

            std::shared_ptr<ObjectSet> newSet = nullptr;
			if (!m_objectPool.try_pop(newSet))
                newSet = std::make_shared<ObjectSet>();

            *newSet = *current;
            newSet->insert(id);

            if (objects.compare_exchange_weak(current, newSet)) {
                current->clear();
				m_objectPool.push(current);
                break;
            }
            else {
				newSet->clear();
                m_objectPool.push(newSet);
            }
        }
    }

    void Erase(int id) {
        while (true) {
            auto current = objects.load();
            if (current->find(id) == current->end())
                return;

            std::shared_ptr<ObjectSet> newSet = nullptr;
            if (!m_objectPool.try_pop(newSet))
                newSet = std::make_shared<ObjectSet>();

            *newSet = *current;
            newSet->erase(id);

            if (objects.compare_exchange_weak(current, newSet)) {
                current->clear();
                m_objectPool.push(current);
                break;
            }
            else {
                newSet->clear();
                m_objectPool.push(newSet);
            }
        }
    }

    std::vector<int> GetActiveObjects() const {
        auto snapshot = objects.load();
        return std::vector<int>(snapshot->begin(), snapshot->end());
    }

private:
    std::atomic<std::shared_ptr<ObjectSet>> objects;
	tbb::concurrent_queue<std::shared_ptr<ObjectSet>> m_objectPool;
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

