#pragma once
#include <windows.h>
#include "Defines.h"

class Camera;

class Map
{
public:
    Map();
    ~Map();
    void Init();
    void Update();
    void Render(HDC hdc, Camera* camera, int offsetY);
};
