#pragma once
#include "Common.h"

class Train {
public:
    // direction: +1 = 왼→오른쪽,  -1 = 오른→왼쪽
    Train(float x, float y, int direction, const wchar_t* imagePath);
    ~Train();

    void Update();
    void Draw(Graphics* g);
    void Cool(float amount);

    Vec2  GetPos()       const { return pos; }
    float GetHeat()      const { return heat; }
    bool  IsOverheated() const { return heat >= MAX_HEAT; }
    bool  IsFinished()   const { return finished; }

private:
    Vec2  pos;
    float speed;
    float heat;
    int   dir;       // +1 or -1
    bool  finished;
    ULONGLONG lastTime;
    Bitmap* image;
    void DrawHeatBar(Graphics* g);
};