#pragma once
#include "Common.h"

class Player {
public:
    Player(int id, float x, float y, Color color);
    ~Player();

    void Update(bool up, bool down, bool left, bool right, class Map* map);
    void Draw(Graphics* g, const Camera& cam);

    Vec2 GetPos() const { return pos; }

private:
    int id;
    Vec2 pos;
    float speed;
    Color color;
    float size;
};
