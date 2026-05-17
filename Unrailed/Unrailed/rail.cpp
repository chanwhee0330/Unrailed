#include "Rail.h"

Rail::Rail() {
    railImage = new Bitmap(L"Image\\train\\rail.png"); 
    turnImage = new Bitmap(L"Image\\train\\turn rail.png");
}
bool Rail::HasHorizontal(int tileX, int tileY) {
    for (auto& r : rails)
        if (r.tileX == tileX && r.tileY == tileY && r.dir == RailDir::HORIZONTAL) return true;
    return false;
}

bool Rail::HasVertical(int tileX, int tileY) {
    for (auto& r : rails)
        if (r.tileX == tileX && r.tileY == tileY && r.dir == RailDir::VERTICAL) return true;
    return false;
}
void Rail::PlaceRail(int tileX, int tileY, RailDir dir, int owner) {
    if (HasRail(tileX, tileY)) return;

    // 수평 설치 시 위아래에 수직 레일 있으면 막기
    if (dir == RailDir::HORIZONTAL) {
        if (HasVertical(tileX, tileY - 1) || HasVertical(tileX, tileY + 1)) return;
    }
    // 수직 설치 시 좌우에 수평 레일 있으면 막기
    if (dir == RailDir::VERTICAL) {
        if (HasHorizontal(tileX - 1, tileY) || HasHorizontal(tileX + 1, tileY)) return;
    }

    RailDir autoDir = AutoDetectDir(tileX, tileY, dir);
    rails.push_back({ tileX, tileY, autoDir, owner });
    UpdateNeighbors(tileX, tileY);
}
bool Rail::HasRail(int tileX, int tileY) {
    for (auto& r : rails)
        if (r.tileX == tileX && r.tileY == tileY) return true;
    return false;
}

RailDir Rail::GetDir(int tileX, int tileY) {
    for (auto& r : rails)
        if (r.tileX == tileX && r.tileY == tileY) return r.dir;
    return RailDir::HORIZONTAL; // 기본값
}
void Rail::DrawPreview(Graphics* g, int x, int y, RailDir dir) {
    GraphicsState state = g->Save();
    g->TranslateTransform(x + TILE_SIZE / 2.0f, y + TILE_SIZE / 2.0f);

    bool isTurn = (dir == RailDir::TURN_RD || dir == RailDir::TURN_LD ||
        dir == RailDir::TURN_RU || dir == RailDir::TURN_LU);
    Bitmap* img = isTurn ? turnImage : railImage;

    switch (dir) {
    case RailDir::VERTICAL: g->RotateTransform(90);  break;  // ← 추가
    case RailDir::TURN_RD:  /* 0도 */                break;
    case RailDir::TURN_LD:  g->RotateTransform(90);  break;
    case RailDir::TURN_LU:  g->RotateTransform(180); break;
    case RailDir::TURN_RU:  g->RotateTransform(270); break;
    default: break;
    }

    // 반투명하게 그리기
    ColorMatrix cm = { 1,0,0,0,0, 0,1,0,0,0, 0,0,1,0,0, 0,0,0,0.5f,0, 0,0,0,0,1 };
    ImageAttributes ia;
    ia.SetColorMatrix(&cm);
    g->DrawImage(img, RectF(-TILE_SIZE / 2.0f, -TILE_SIZE / 2.0f, (float)TILE_SIZE, (float)TILE_SIZE),
        0, 0, (float)img->GetWidth(), (float)img->GetHeight(), UnitPixel, &ia);

    g->Restore(state);
}
void Rail::UpdateNeighbors(int tileX, int tileY) {
    int dx[] = { -1, 1, 0, 0 };
    int dy[] = { 0, 0, -1, 1 };
    for (int i = 0; i < 4; i++) {
        int nx = tileX + dx[i];
        int ny = tileY + dy[i];
        for (auto& r : rails) {
            if (r.tileX == nx && r.tileY == ny) {
                r.dir = AutoDetectDir(nx, ny, r.dir);  // r.dir 추가
            }
        }
    }
}
RailDir Rail::AutoDetectDir(int tileX, int tileY, RailDir baseDir) {
    bool left = HasRail(tileX - 1, tileY);
    bool right = HasRail(tileX + 1, tileY);
    bool up = HasRail(tileX, tileY - 1);
    bool down = HasRail(tileX, tileY + 1);

    // 수직 레일 설치 시
    if (baseDir == RailDir::VERTICAL) {
        if (right && (up || down)) return RailDir::TURN_RD; // 오른쪽에 레일 있으면
        if (left && (up || down)) return RailDir::TURN_LD;
        return RailDir::VERTICAL;
    }
    // 수평 레일 설치 시
    if (down && (left || right)) return baseDir == RailDir::HORIZONTAL && right ? RailDir::TURN_RD : RailDir::TURN_LD;
    if (up && (left || right)) return baseDir == RailDir::HORIZONTAL && right ? RailDir::TURN_RU : RailDir::TURN_LU;
    return baseDir;
}
void Rail::Draw(Graphics* g) {
    for (auto& r : rails) {
        float x = (float)(r.tileX * TILE_SIZE);
        float y = (float)(r.tileY * TILE_SIZE);

        GraphicsState state = g->Save();
        g->TranslateTransform(x + TILE_SIZE / 2.0f, y + TILE_SIZE / 2.0f);

        bool isTurn = (r.dir == RailDir::TURN_RD || r.dir == RailDir::TURN_LD ||
            r.dir == RailDir::TURN_RU || r.dir == RailDir::TURN_LU);
        Bitmap* img = isTurn ? turnImage : railImage;

        switch (r.dir) {
        case RailDir::VERTICAL: g->RotateTransform(90);  break;  // ← 추가
        case RailDir::TURN_RD:  /* 0도 */                break;
        case RailDir::TURN_LD:  g->RotateTransform(90);  break;
        case RailDir::TURN_LU:  g->RotateTransform(180); break;
        case RailDir::TURN_RU:  g->RotateTransform(270); break;
        default: break;
        }

        g->DrawImage(img, -TILE_SIZE / 2.0f, -TILE_SIZE / 2.0f, (float)TILE_SIZE, (float)TILE_SIZE);
        g->Restore(state);
    }
}

Rail::~Rail() {
    delete railImage;
    delete turnImage;
}
