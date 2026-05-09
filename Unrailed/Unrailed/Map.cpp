#include "Map.h"

Map::Map() : tileSet(nullptr) {
    for (int y = 0; y < MAP_HEIGHT; ++y) {
        for (int x = 0; x < MAP_WIDTH; ++x) {
            tiles[y][x] = 0;
        }
    }
    // For now, we still use the background image if available
    tileSet = new Image(L"Image\\Map\\UnrailedMap.png");
}

Map::~Map() {
    delete tileSet;
}

void Map::LoadMap(const std::wstring& csvPath) {
    std::ifstream file(csvPath);
    if (!file.is_open()) return;

    std::string line;
    int y = 0;
    while (std::getline(file, line) && y < MAP_HEIGHT) {
        std::stringstream ss(line);
        std::string cell;
        int x = 0;
        while (std::getline(ss, cell, ',') && x < MAP_WIDTH) {
            tiles[y][x] = std::stoi(cell);
            x++;
        }
        y++;
    }
}

void Map::Draw(Graphics* g, const Camera& cam, int viewWidth, int viewHeight) {
    // Just draw the background for now
    // In a real tile engine, we'd loop through visible tiles
    g->DrawImage(tileSet, 0, 0);
}

bool Map::IsSolid(float x, float y) {
    int tx = (int)(x / TILE_SIZE);
    int ty = (int)(y / TILE_SIZE);

    if (tx < 0 || tx >= MAP_WIDTH || ty < 0 || ty >= MAP_HEIGHT) return true;

    // Hardcoded wall IDs based on observation of CSV (values != 16 seem to be walls/river)
    // Looking at the CSV sample: 16 was dominant (likely ground), 12, 25, 48, etc were in the middle.
    // Let's assume for now any ID != 16 is solid in Layer 1.
    return tiles[ty][tx] != 16;
}
