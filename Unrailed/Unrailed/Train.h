#pragma once
#include <windows.h>
#include <gdiplus.h>
#include "Defines.h"
using namespace Gdiplus;
class Camera;

class Train
{
public:
    Train(int num);
    ~Train();
    void Init();
    void Update();
    void Render(HDC hdc, Camera* camera);
    float GetX() const { return m_x; }
    float GetY() const { return m_y; }
    bool  IsDerailed()   const { return m_derailed; }
    bool  IsOverheated() const { return m_overheated; }
private:
    int   m_num;
    float m_x, m_y;
    bool  m_derailed, m_overheated;
    Image* m_pImage;
    DWORD m_lastTime;  // 마지막 업데이트 시간
    float m_speed = 2.f;  // 기차 속도
};
