#pragma once
#include "Common.h"
#include <atlimage.h>

enum class PlayerDir {
    Down,Left,Right,Up
};

class Player {
public:
    Player(int id, float x, float y, Color color);
    ~Player();

    void Update(bool up, bool down, bool left, bool right, class Map* map);
    void Draw(HDC hdc, const Camera& cam, int offsetY);
    void DrawInventory(HDC hdc, const Camera& cam, int offsetY);

    Vec2 GetPos() const { return pos; }
    RECT GetRect() const;
    void AddWood(int amount) { wood += amount; }
    void AddStone(int amount) { stone += amount; }

private:
    int id;
    Vec2 pos;
    float speed;
    Color color;
    float size;

    PlayerDir dir;
    bool isMoving;

    CImage idleSheet;
    CImage walkSheet;

    int frame;
    int wood;
    int stone;
    DWORD lastFrameTime;
    DWORD frameDelay;

    void LoadImages();
    int GetFrameCount() const;
    int GetDirectionRow() const;
};
