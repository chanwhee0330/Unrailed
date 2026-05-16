#include "Game.h"

Game::Game(HWND hWnd) : hWnd(hWnd), gameOver(false), winner(0) {
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
    train1 = new Train(0, 500, 1, L"Image\\train\\locomoto.png");
    train2 = new Train((float)(MAP_WIDTH * TILE_SIZE - 96), 1400, -1, L"Image\\train\\locomoto2.png");
    cam1 = {0, 0};
    cam2 = {0, 0};
    rail = new Rail();
    // train1
    int t1X = (int)(train1->GetPos().x / TILE_SIZE);
    int t1Y = (int)((train1->GetPos().y + 24) / TILE_SIZE); // +24 = 기차 높이(48)의 절반
    for (int i = 0; i < 5; i++)
        rail->PlaceRail(t1X + i, t1Y, RailDir::HORIZONTAL, 1);

    // train2
    int t2X = (int)(train2->GetPos().x / TILE_SIZE);
    int t2Y = (int)((train2->GetPos().y + 24) / TILE_SIZE);
    for (int i = 0; i < 5; i++)
        rail->PlaceRail(t2X + 2 - i, t2Y, RailDir::HORIZONTAL, 2); 
}

Game::~Game() {
    delete p1;
    delete p2;
    delete map;
    delete train1;
    delete train2;
    delete rail;

    SelectObject(memDC, oldBitmap);
    DeleteObject(memBitmap);
    DeleteDC(memDC);
}

void Game::Update() {
    if (gameOver) return;

    p1->Update(GetAsyncKeyState('W'), GetAsyncKeyState('S'), GetAsyncKeyState('A'), GetAsyncKeyState('D'), map);
    p2->Update(GetAsyncKeyState(VK_UP), GetAsyncKeyState(VK_DOWN), GetAsyncKeyState(VK_LEFT), GetAsyncKeyState(VK_RIGHT), map);
   
    bool rKey = GetAsyncKeyState('R') & 0x8000;
    if (rKey && !rKeyPrev) {
        selectedDir1 = (selectedDir1 == RailDir::HORIZONTAL) ? RailDir::VERTICAL : RailDir::HORIZONTAL;
    }
    rKeyPrev = rKey;

    bool twoKey = GetAsyncKeyState('2') & 0x8000;
    if (twoKey && !twoKeyPrev) {
        selectedDir2 = (selectedDir2 == RailDir::HORIZONTAL) ? RailDir::VERTICAL : RailDir::HORIZONTAL;
    }
    twoKeyPrev = twoKey;
    // 설치
    if (GetAsyncKeyState('E') & 0x8000) {
        int tileX = (int)(p1->GetPos().x / TILE_SIZE);
        int tileY = (int)(p1->GetPos().y / TILE_SIZE);
        rail->PlaceRail(tileX, tileY, selectedDir1, 1);
    }

    if (GetAsyncKeyState('1') & 0x8000) {
        int tileX = (int)(p2->GetPos().x / TILE_SIZE);
        int tileY = (int)(p2->GetPos().y / TILE_SIZE);
        rail->PlaceRail(tileX, tileY, selectedDir2, 2);
    }
    train1->Update(rail);
    train2->Update(rail);

    if (train1->IsFinished() && !train2->IsFinished()) { gameOver = true; winner = 1; }
    else if (train2->IsFinished() && !train1->IsFinished()) { gameOver = true; winner = 2; }
    else if (train1->IsFinished() && train2->IsFinished()) { gameOver = true; winner = 0; }
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
    //미리보기
    int preX1 = (int)(p1->GetPos().x / TILE_SIZE) * TILE_SIZE;
    int preY1 = (int)(p1->GetPos().y / TILE_SIZE) * TILE_SIZE;
    SolidBrush previewBrush1(Color(120, 255, 255, 0));
    g1.FillRectangle(&previewBrush1, (float)preX1, (float)preY1, (float)TILE_SIZE, (float)TILE_SIZE);
    rail->DrawPreview(&g1, preX1, preY1, selectedDir1);

    p1->Draw(&g1, cam1);
    p2->Draw(&g1, cam1);
    rail->Draw(&g1);
    train1->Draw(&g1);
    train2->Draw(&g1);
    // --- View 2 (Player 2) ---
    Graphics g2(memDC);
    g2.SetClip(Rect(0, halfH, SCREEN_WIDTH, halfH));
    g2.TranslateTransform(0, (REAL)halfH);
    g2.TranslateTransform(-cam2.x, -cam2.y);
    map->Draw(&g2, cam2, SCREEN_WIDTH, halfH);
    //미리보기
    int preX2 = (int)(p2->GetPos().x / TILE_SIZE) * TILE_SIZE;
    int preY2 = (int)(p2->GetPos().y / TILE_SIZE) * TILE_SIZE;
    SolidBrush previewBrush2(Color(120, 0, 255, 255));
    g2.FillRectangle(&previewBrush2, (float)preX2, (float)preY2, (float)TILE_SIZE, (float)TILE_SIZE);
    rail->DrawPreview(&g2, preX2, preY2, selectedDir2);

    p1->Draw(&g2, cam2);
    p2->Draw(&g2, cam2);
    rail->Draw(&g2);
    train1->Draw(&g2);
    train2->Draw(&g2);
    if (gameOver) {
        Graphics g(memDC);
        DrawVictoryScreen(&g);
    }
    // UI: Divider Line
    Pen pen(Color(255, 0, 0, 0), 5);
    g.DrawLine(&pen, 0, halfH, SCREEN_WIDTH, halfH);

    BitBlt(hDC, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, memDC, 0, 0, SRCCOPY);
}

void Game::DrawVictoryScreen(Graphics* g) {
    SolidBrush overlay(Color(180, 0, 0, 0));
    g->FillRectangle(&overlay, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);

    FontFamily   fontFamily(L"Arial");
    Font         font(&fontFamily, 80, FontStyleBold, UnitPixel);
    SolidBrush   textBrush(Color(255, 255, 255, 0));
    StringFormat sf;
    sf.SetAlignment(StringAlignmentCenter);
    sf.SetLineAlignment(StringAlignmentCenter);

    std::wstring msg;
    if (winner == 1) msg = L"Player 2 WIN!";
    else if (winner == 2) msg = L"Player 1 WIN!";
    else                  msg = L"DRAW!";

    RectF rect(0.0f, 0.0f, (REAL)SCREEN_WIDTH, (REAL)SCREEN_HEIGHT);
    g->DrawString(msg.c_str(), -1, &font, rect, &sf, &textBrush);
}
