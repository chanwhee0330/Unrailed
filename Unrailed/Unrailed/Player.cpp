#include "Player.h"
#include "Map.h"

Player::Player(int id, float x, float y, Color color) 
    : id(id), pos({x, y}), color(color), speed(5.0f), size(40.0f) {}

Player::~Player() {}

void Player::Update(bool up, bool down, bool left, bool right, Map* map) {
    Vec2 nextPos = pos;
    if (up) nextPos.y -= speed;
    if (down) nextPos.y += speed;
    if (left) nextPos.x -= speed;
    if (right) nextPos.x += speed;

    // Basic collision check (checking 4 corners of the player)
    bool collision = false;
    if (map->IsSolid(nextPos.x, nextPos.y) || 
        map->IsSolid(nextPos.x + size, nextPos.y) ||
        map->IsSolid(nextPos.x, nextPos.y + size) ||
        map->IsSolid(nextPos.x + size, nextPos.y + size)) {
        collision = true;
    }

    if (!collision) {
        pos = nextPos;
    }

    // Map boundaries
    if (pos.x < 0) pos.x = 0;
    if (pos.y < 0) pos.y = 0;
    if (pos.x > MAP_WIDTH * TILE_SIZE - size) pos.x = MAP_WIDTH * TILE_SIZE - size;
    if (pos.y > MAP_HEIGHT * TILE_SIZE - size) pos.y = MAP_HEIGHT * TILE_SIZE - size;
}

void Player::Draw(Graphics* g, const Camera& cam) {
    SolidBrush brush(color);
    g->FillEllipse(&brush, pos.x, pos.y, size, size);
}
