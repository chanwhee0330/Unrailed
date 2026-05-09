#pragma once
#include "Common.h"
#include "Player.h"
#include "Map.h"
#include "Train.h"

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

    bool    gameOver;
    int     winner;   // 1 or 2

    HDC memDC;
    HBITMAP memBitmap;
    HBITMAP oldBitmap;

    static void ClampCamera(Camera& cam, int viewW, int viewH);
    void DrawVictoryScreen(Graphics* g);
};
