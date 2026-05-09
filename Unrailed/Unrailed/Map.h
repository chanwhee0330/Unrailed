#pragma once
#include "Common.h"
#include <fstream>
#include <sstream>

class Map {
public:
    Map();
    ~Map();

    void LoadMap(const std::wstring& csvPath);
    void Draw(Graphics* g, const Camera& cam, int viewWidth, int viewHeight);
    bool IsSolid(float x, float y);

private:
    int tiles[MAP_HEIGHT][MAP_WIDTH];
    Image* tileSet;
};
