#include "Game.h"
#include "Map.h"
#include "Player.h"
#include "Camera.h"
#include "Train.h"
#include "Rail.h"
#include "Bomb.h"
#include "Resource.h"
#include "Obstacle.h"
#include "GameManager.h"

Game::Game(HWND hWnd) : m_hWnd(hWnd)
{
    m_hDC        = GetDC(hWnd);
    m_pMap        = new Map();
    m_pPlayer1    = new Player(1);
    m_pPlayer2    = new Player(2);
    m_pCamera1    = new Camera();
    m_pCamera2    = new Camera();
    m_pTrain1     = new Train(1);
    m_pTrain2     = new Train(2);
    m_pRail       = new Rail();
    m_pBomb       = new Bomb();
    m_pResource   = new Resource();
    m_pObstacle   = new Obstacle();
    m_pGameManager= new GameManager();
}

Game::~Game()
{
    ReleaseDC(m_hWnd, m_hDC);
    delete m_pMap;
    delete m_pPlayer1;  delete m_pPlayer2;
    delete m_pCamera1;  delete m_pCamera2;
    delete m_pTrain1;   delete m_pTrain2;
    delete m_pRail;
    delete m_pBomb;
    delete m_pResource;
    delete m_pObstacle;
    delete m_pGameManager;
}

void Game::Init()
{
    m_pMap->Init();
    m_pPlayer1->Init();  m_pPlayer2->Init();
    m_pTrain1->Init();   m_pTrain2->Init();
    m_pRail->Init();
    m_pBomb->Init();
    m_pResource->Init();
    m_pObstacle->Init();
    m_pGameManager->Init(m_pTrain1, m_pTrain2, m_pBomb);
}

void Game::Update()
{
    m_pPlayer1->Update();  m_pPlayer2->Update();
    m_pTrain1->Update();   m_pTrain2->Update();
    m_pRail->Update();
    m_pBomb->Update();
    m_pResource->Update();
    m_pObstacle->Update();
    m_pCamera1->Update(m_pPlayer1->GetX(), m_pPlayer1->GetY());
    m_pCamera2->Update(m_pPlayer2->GetX(), m_pPlayer2->GetY());
    m_pGameManager->Update();
}

void Game::Render()
{
    // 백버퍼 생성
    HDC hMemDC = CreateCompatibleDC(m_hDC);
    HBITMAP hBitmap = CreateCompatibleBitmap(m_hDC, SCREEN_W, SCREEN_H);
    HBITMAP hOldBitmap = (HBITMAP)SelectObject(hMemDC, hBitmap);

    // 배경 초기화
    HBRUSH hBrush = CreateSolidBrush(RGB(0, 0, 0));
    RECT rect = { 0, 0, SCREEN_W, SCREEN_H };
    FillRect(hMemDC, &rect, hBrush);
    DeleteObject(hBrush);

    // 플레이어1 화면 (상단) - m_hDC → hMemDC
    m_pMap->Render(hMemDC, m_pCamera1, 0);
    m_pResource->Render(hMemDC, m_pCamera1, 0);
    m_pObstacle->Render(hMemDC, m_pCamera1, 0);
    m_pRail->Render(hMemDC, m_pCamera1, 0);
    m_pPlayer1->Render(hMemDC, m_pCamera1);
    m_pTrain1->Render(hMemDC, m_pCamera1);
    m_pBomb->Render(hMemDC, m_pCamera1);

    // 플레이어2 화면 (하단) - m_hDC → hMemDC
    m_pMap->Render(hMemDC, m_pCamera2, HALF_H);
    m_pResource->Render(hMemDC, m_pCamera2, HALF_H);
    m_pObstacle->Render(hMemDC, m_pCamera2, HALF_H);
    m_pRail->Render(hMemDC, m_pCamera2, HALF_H);
    m_pPlayer2->Render(hMemDC, m_pCamera2);
    m_pTrain2->Render(hMemDC, m_pCamera2);
    m_pBomb->Render(hMemDC, m_pCamera2);

    // 백버퍼 → 화면에 복사
    RECT clientRect;
    GetClientRect(m_hWnd, &clientRect);
    int winW = clientRect.right;
    int winH = clientRect.bottom;
    StretchBlt(m_hDC, 0, 0, winW, winH, hMemDC, 0, 0, SCREEN_W, SCREEN_H, SRCCOPY);

    // 정리
    SelectObject(hMemDC, hOldBitmap);
    DeleteObject(hBitmap);
    DeleteDC(hMemDC);
}

void Game::OnKeyDown(WPARAM key)
{
    m_pPlayer1->OnKeyDown(key);
    m_pPlayer2->OnKeyDown(key);
}

void Game::OnKeyUp(WPARAM key)
{
    m_pPlayer1->OnKeyUp(key);
    m_pPlayer2->OnKeyUp(key);
}
