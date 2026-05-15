#pragma once
#include "Common.h"
#include "Rail.h"

class Train {
public:
    Train(float x, float y, int direction, const wchar_t* imagePath);
    ~Train();

    void Update(Rail* rail);
    void Draw(Graphics* g);
    void Cool(float amount);

    Vec2  GetPos()       const { return pos; }
    float GetHeat()      const { return heat; }
    bool  IsOverheated() const { return heat >= MAX_HEAT; }
    bool  IsFinished()   const { return finished; }
    void UpdateDirection(RailDir rd);

private:
    float snapToTile(float v) { return (float)((int)(v / TILE_SIZE) * TILE_SIZE); }    Vec2  pos;
    float speed;
    float heat;
    float dirX, dirY;
    bool  finished;
    ULONGLONG lastTime;
    Bitmap* image;
    void DrawHeatBar(Graphics* g);
};