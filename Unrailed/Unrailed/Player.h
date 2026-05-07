#pragma once
#include <windows.h>
#include "Defines.h"

class Camera;

class Player
{
public:
    Player(int num);
    ~Player();
    void Init();
    void Update();
    void Render(HDC hdc, Camera* camera);
    void OnKeyDown(WPARAM key);
    void OnKeyUp(WPARAM key);
    float GetX() const { return m_x; }
    float GetY() const { return m_y; }
private:
    int   m_num;
    float m_x, m_y;
};
