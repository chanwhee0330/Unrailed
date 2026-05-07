#pragma once
#include <windows.h>
#include "Defines.h"

class Camera;

class Resource
{
public:
    Resource();
    ~Resource();
    void Init();
    void Update();
    void Render(HDC hdc, Camera* camera, int offsetY);
};
