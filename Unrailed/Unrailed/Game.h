#pragma once
#include "Common.h"
#include "Player.h"
#include "Map.h"

class Game {
public:
    Game(HWND hWnd);
    ~Game();

    void Update();
    void Draw(HDC hDC);

private:
    HWND hWnd;
    Player* p1;
    Player* p2;
    Map* map;
    Camera cam1;
    Camera cam2;

    HDC memDC;
    HBITMAP memBitmap;
    HBITMAP oldBitmap;
};
