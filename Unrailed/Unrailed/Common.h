#pragma once
#include <windows.h>
#include <gdiplus.h>
#include <vector>
#include <string>

using namespace Gdiplus;

#define SCREEN_WIDTH 1920
#define SCREEN_HEIGHT 1080
#define MAP_WIDTH 90
#define MAP_HEIGHT 60
#define TILE_SIZE 32

struct Vec2 {
    float x, y;
};

struct Camera {
    float x, y;
};
