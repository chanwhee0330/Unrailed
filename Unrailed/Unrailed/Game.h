#pragma once
#include <windows.h>

class Map;
class Player;
class Camera;
class Train;
class Rail;
class Bomb;
class Resource;
class Obstacle;
class GameManager;

class Game
{
public:
    Game(HWND hWnd);
    ~Game();
    void Init();
    void Update();
    void Render();
    void OnKeyDown(WPARAM key);
    void OnKeyUp(WPARAM key);
private:
    HWND         m_hWnd;
    HDC          m_hDC;
    Map*         m_pMap;
    Player*      m_pPlayer1;
    Player*      m_pPlayer2;
    Camera*      m_pCamera1;
    Camera*      m_pCamera2;
    Train*       m_pTrain1;
    Train*       m_pTrain2;
    Rail*        m_pRail;
    Bomb*        m_pBomb;
    Resource*    m_pResource;
    Obstacle*    m_pObstacle;
    GameManager* m_pGameManager;
};
