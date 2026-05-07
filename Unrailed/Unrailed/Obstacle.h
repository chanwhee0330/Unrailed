#pragma once
#include <windows.h>
#include "Defines.h"

class Camera;

class Obstacle
{
public:
    Obstacle();
    ~Obstacle();
    void Init();
    void Update();
    void Render(HDC hdc, Camera* camera, int offsetY);
};
