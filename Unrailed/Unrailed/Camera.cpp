#include "Camera.h"

Camera::Camera() : m_x(0), m_y(0) {}
Camera::~Camera() {}
void Camera::Update(float targetX, float targetY)
{
    m_x = (int)(targetX - SCREEN_W / 2);
    m_y = (int)(targetY - HALF_H  / 2);
    if (m_x < 0) m_x = 0;
    if (m_y < 0) m_y = 0;
}
