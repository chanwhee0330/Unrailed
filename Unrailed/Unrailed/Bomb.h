#pragma once
#include <windows.h>
#include "Defines.h"

class Camera;

class Bomb
{
public:
    Bomb();
    ~Bomb();
    void Init();
    void Update();
    void Render(HDC hdc, Camera* camera);
};
