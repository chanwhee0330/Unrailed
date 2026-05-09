#include "Game.h"

Game::Game(HWND hWnd) : hWnd(hWnd) {
    p1 = new Player(1, 300, 400, Color(255, 255, 0, 0));
    p2 = new Player(2, 2500, 960, Color(255, 0, 0, 255));
    map = new Map();
    // Assuming CSV is in the same directory as the executable or project root
    // For now, let's use the absolute path we found
    map->LoadMap(L"C:\\Users\\G\\Documents\\unTiled map_Tile Layer 1.csv");

    HDC hdc = GetDC(hWnd);
    memDC = CreateCompatibleDC(hdc);
    memBitmap = CreateCompatibleBitmap(hdc, SCREEN_WIDTH, SCREEN_HEIGHT);
    oldBitmap = (HBITMAP)SelectObject(memDC, memBitmap);
    ReleaseDC(hWnd, hdc);

    cam1 = {0, 0};
    cam2 = {0, 0};
}

Game::~Game() {
    delete p1;
    delete p2;
    delete map;

    SelectObject(memDC, oldBitmap);
    DeleteObject(memBitmap);
    DeleteDC(memDC);
}

void Game::Update() {
    p1->Update(GetAsyncKeyState('W'), GetAsyncKeyState('S'), GetAsyncKeyState('A'), GetAsyncKeyState('D'), map);
    p2->Update(GetAsyncKeyState(VK_UP), GetAsyncKeyState(VK_DOWN), GetAsyncKeyState(VK_LEFT), GetAsyncKeyState(VK_RIGHT), map);

    // Update cameras
    int halfH = SCREEN_HEIGHT / 2;

    cam1.x = p1->GetPos().x - SCREEN_WIDTH / 2;
    cam1.y = p1->GetPos().y - (halfH / 2);
    if (cam1.x < 0) cam1.x = 0;
    if (cam1.y < 0) cam1.y = 0;
    if (cam1.x > MAP_WIDTH * TILE_SIZE - SCREEN_WIDTH) cam1.x = MAP_WIDTH * TILE_SIZE - SCREEN_WIDTH;
    if (cam1.y > MAP_HEIGHT * TILE_SIZE - halfH) cam1.y = MAP_HEIGHT * TILE_SIZE - halfH;

    cam2.x = p2->GetPos().x - SCREEN_WIDTH / 2;
    cam2.y = p2->GetPos().y - (halfH / 2);
    if (cam2.x < 0) cam2.x = 0;
    if (cam2.y < 0) cam2.y = 0;
    if (cam2.x > MAP_WIDTH * TILE_SIZE - SCREEN_WIDTH) cam2.x = MAP_WIDTH * TILE_SIZE - SCREEN_WIDTH;
    if (cam2.y > MAP_HEIGHT * TILE_SIZE - halfH) cam2.y = MAP_HEIGHT * TILE_SIZE - halfH;

    InvalidateRect(hWnd, NULL, FALSE);
}

void Game::Draw(HDC hDC) {
    Graphics g(memDC);
    int halfH = SCREEN_HEIGHT / 2;

    // --- View 1 (Player 1) ---
    Graphics g1(memDC);
    g1.SetClip(Rect(0, 0, SCREEN_WIDTH, halfH));
    g1.Clear(Color(255, 255, 255));
    g1.TranslateTransform(-cam1.x, -cam1.y);
    map->Draw(&g1, cam1, SCREEN_WIDTH, halfH);
    p1->Draw(&g1, cam1);
    p2->Draw(&g1, cam1);

    // --- View 2 (Player 2) ---
    Graphics g2(memDC);
    g2.SetClip(Rect(0, halfH, SCREEN_WIDTH, halfH));
    g2.TranslateTransform(0, (REAL)halfH);
    g2.TranslateTransform(-cam2.x, -cam2.y);
    map->Draw(&g2, cam2, SCREEN_WIDTH, halfH);
    p1->Draw(&g2, cam2);
    p2->Draw(&g2, cam2);

    // UI: Divider Line
    Pen pen(Color(255, 0, 0, 0), 5);
    g.DrawLine(&pen, 0, halfH, SCREEN_WIDTH, halfH);

    BitBlt(hDC, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, memDC, 0, 0, SRCCOPY);
}
