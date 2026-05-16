#pragma once
#include "Common.h"
#include "Player.h"
#include "Rail.h"

enum class ResourceType {
    Tree,
    Rock
};

class Resource {
public:
    Resource(ResourceType type, float x, float y);

    void Update(Player* p1, Player* p2, Rail* rail);
    void Draw(Graphics* g);

    bool IsDestroyed() const { return !active; }
    RECT GetRect() const;

private:
    ResourceType type;
    Vec2 pos;
    Vec2 spawnPos;
    float size;
    float harvestProgress;
    bool active;
    ULONGLONG respawnStartTime;
    ULONGLONG respawnDelay;

    void Harvest(Player* player);
    bool Intersects(Player* player) const;
    bool HasRailOnSpawn(Rail* rail) const;
    void StartRespawnTimer();
};
