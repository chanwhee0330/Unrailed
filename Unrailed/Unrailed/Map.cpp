#include "Map.h"

Map::Map() : tileSet(nullptr) {
    for (int y = 0; y < MAP_HEIGHT; ++y) {
        for (int x = 0; x < MAP_WIDTH; ++x) {
            tiles[y][x] = 0;
        }
    }
    // Load the background image as a Bitmap to use GetPixel
    tileSet = new Bitmap(L"Image\\Map\\UnrailedMap.png");
    if (tileSet->GetLastStatus() != Ok) {
        // Fallback or debug message could go here
    }
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
    if (tileSet) {
        g->DrawImage(tileSet, 0, 0);
    }
}

bool Map::IsSolid(float x, float y) {
    if (!tileSet || tileSet->GetLastStatus() != Ok) return false;

    int imgW = tileSet->GetWidth();
    int imgH = tileSet->GetHeight();

    if (x < 0 || y < 0 || x >= imgW || y >= imgH) return true;

    Color color;
    tileSet->GetPixel((int)x, (int)y, &color);

    BYTE r = color.GetR();
    BYTE g = color.GetG();
    BYTE b = color.GetB();

    // "Green" detection: 
    // We want to be careful here. Green usually has a high G value relative to R and B.
    // Grass in maps is often something like (100, 200, 100).
    bool isGreen = (g > 80 && g > r && g > b);

    // If it's green, it's NOT solid (movable). Otherwise, it IS solid.
    return !isGreen;
}
