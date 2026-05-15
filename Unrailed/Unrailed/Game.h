#pragma once
#include "Common.h"
#include "Player.h"
#include "Map.h"
#include "Train.h"
#include "Rail.h"

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
    Train* train;
    Train* train1;   // 왼에서오른쪽
    Train* train2;   // 오른에서왼쪽
    Camera cam1;
    Camera cam2;
    Rail* rail;

    bool    gameOver;
    int     winner;   // 1 or 2
    bool rKeyPrev = false;
    bool twoKeyPrev = false;
    HDC memDC;
    HBITMAP memBitmap;
    HBITMAP oldBitmap;
    RailDir selectedDir1 = RailDir::HORIZONTAL;
    RailDir selectedDir2 = RailDir::HORIZONTAL;
    static void ClampCamera(Camera& cam, int viewW, int viewH);
    void DrawVictoryScreen(Graphics* g);
};
