#pragma once
#include <vector>
#include "Common.h"
#include <gdiplus.h>
using namespace Gdiplus;

enum class RailDir { HORIZONTAL, VERTICAL, TURN_RD, TURN_LD, TURN_RU, TURN_LU };

class Rail {
public:
    void PlaceRail(int tileX, int tileY, RailDir dir, int owner);
    bool HasRail(int tileX, int tileY);
    RailDir GetDir(int tileX, int tileY);
    void Draw(Graphics* g);
    void DrawPreview(Graphics* g, int x, int y, RailDir dir);
    Rail();
    ~Rail();
    int GetCount() { return (int)rails.size(); }
private:
    struct RailData {
       
        int tileX, tileY;
        RailDir dir;
        int owner;
    };
    std::vector<RailData> rails;
    Bitmap* railImage;
    Bitmap* turnImage;
    bool HasHorizontal(int tileX, int tileY);
    bool HasVertical(int tileX, int tileY);
    RailDir AutoDetectDir(int tileX, int tileY, RailDir baseDir);  // RailDir baseDir 추가
    void UpdateNeighbors(int tileX, int tileY);
};