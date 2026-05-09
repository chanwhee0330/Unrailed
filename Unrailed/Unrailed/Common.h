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
#define MAX_HEAT   100.0f //최대과열
#define HEAT_RATE  0.07f //시간이 지날 때 마다 오름
#define COOL_AMOUNT 5.0f // 냉각시 내려가는 양