#pragma once
#include <windows.h>
#include "Defines.h"

class Camera;

class Rail
{
public:
    Rail();
    ~Rail();
    void Init();
    void Update();
    void Render(HDC hdc, Camera* camera, int offsetY);
};
