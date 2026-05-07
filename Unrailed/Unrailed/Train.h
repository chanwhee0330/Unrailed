#pragma once
#include <windows.h>
#include "Defines.h"

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
};
