#include "Resource.h"

Resource::Resource(ResourceType type, float x, float y)
    : type(type), pos({ x, y }), spawnPos({ x, y }), size(64.0f),
    harvestProgress(0.0f), active(true), respawnStartTime(0), respawnDelay(10000) {
}

RECT Resource::GetRect() const {
    RECT rc;
    rc.left = (LONG)(pos.x + 8);
    rc.top = (LONG)(pos.y + 8);
    rc.right = (LONG)(pos.x + size - 8);
    rc.bottom = (LONG)(pos.y + size - 4);
    return rc;
}

bool Resource::Intersects(Player* player) const {
    if (!active) return false;

    RECT hit;
    RECT resourceRect = GetRect();
    RECT playerRect = player->GetRect();
    return IntersectRect(&hit, &resourceRect, &playerRect);
}

bool Resource::HasRailOnSpawn(Rail* rail) const {
    int tileX = (int)((spawnPos.x + size / 2.0f) / TILE_SIZE);
    int tileY = (int)((spawnPos.y + size / 2.0f) / TILE_SIZE);
    return rail->HasRail(tileX, tileY);
}

void Resource::StartRespawnTimer() {
    active = false;
    harvestProgress = 0.0f;
    respawnStartTime = GetTickCount64();
}

void Resource::Harvest(Player* player) {
    harvestProgress += 0.025f;
    if (harvestProgress >= 1.0f) {
        if (type == ResourceType::Tree) {
            player->AddWood(1);
        }
        else {
            player->AddStone(1);
        }
        StartRespawnTimer();
    }
}

void Resource::Update(Player* p1, Player* p2, Rail* rail) {
    if (active && HasRailOnSpawn(rail)) {
        StartRespawnTimer();
    }

    if (!active) {
        if (!HasRailOnSpawn(rail) && GetTickCount64() - respawnStartTime >= respawnDelay) {
            pos = spawnPos;
            active = true;
        }
        return;
    }

    if (Intersects(p1)) {
        Harvest(p1);
    }
    else if (Intersects(p2)) {
        Harvest(p2);
    }
}

void Resource::Draw(Graphics* g) {
    if (!active) return;

    float scale = 1.0f - harvestProgress * 0.6f;
    float drawSize = size * scale;
    float drawX = pos.x + (size - drawSize) / 2.0f;
    float drawY = pos.y + (size - drawSize) / 2.0f;

    if (type == ResourceType::Tree) {
        SolidBrush trunk(Color(255, 120, 72, 32));
        SolidBrush leaves(Color(255, 32, 150, 76));
        g->FillRectangle(&trunk, drawX + drawSize * 0.42f, drawY + drawSize * 0.48f, drawSize * 0.16f, drawSize * 0.42f);
        g->FillEllipse(&leaves, drawX + drawSize * 0.12f, drawY + drawSize * 0.02f, drawSize * 0.76f, drawSize * 0.62f);
    }
    else {
        SolidBrush rock(Color(255, 115, 120, 130));
        Pen edge(Color(255, 75, 80, 90), 2.0f);
        RectF rect(drawX + drawSize * 0.08f, drawY + drawSize * 0.22f, drawSize * 0.84f, drawSize * 0.58f);
        g->FillEllipse(&rock, rect);
        g->DrawEllipse(&edge, rect);
    }
}
