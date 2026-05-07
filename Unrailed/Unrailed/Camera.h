#pragma once
#include "Defines.h"

class Camera
{
public:
    Camera();
    ~Camera();
    void Update(float targetX, float targetY);
    int GetX() const { return m_x; }
    int GetY() const { return m_y; }
private:
    int m_x, m_y;
};
