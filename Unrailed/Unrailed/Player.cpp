#include "Player.h"
#include "Map.h"

Player::Player(int id, float x, float y, Color color) 
    : id(id), pos({ x, y }), color(color), speed(5.0f), size(64.0f),
    dir(PlayerDir::Down), isMoving(false),
    frame(0), wood(0), stone(0), lastFrameTime(GetTickCount()), frameDelay(120)
{
    LoadImages();
}

Player::~Player() {}

void Player::LoadImages() {
    if (id == 1) {
        idleSheet.Load(L"Image\\Player\\Player1\\Slime1_Idle_with_shadow.png");
        walkSheet.Load(L"Image\\Player\\Player1\\Slime1_Walk_with_shadow.png");
    }
    else {
        idleSheet.Load(L"Image\\Player\\Player2\\Slime3_Idle_with_shadow.png");
        walkSheet.Load(L"Image\\Player\\Player2\\Slime3_Walk_with_shadow.png");
    }
}

int Player::GetFrameCount() const {
    return isMoving ? 8 : 6;
}

int Player::GetDirectionRow() const {
    switch (dir) {
    case PlayerDir::Down:  return 0;
    case PlayerDir::Up:    return 1;
    case PlayerDir::Left:  return 2;
    case PlayerDir::Right: return 3;
    }
    return 0;
}

RECT Player::GetRect() const {
    RECT rc;
    rc.left = (LONG)(pos.x + 12);
    rc.top = (LONG)(pos.y + 16);
    rc.right = (LONG)(pos.x + size - 12);
    rc.bottom = (LONG)(pos.y + size - 6);
    return rc;
}

void Player::Update(bool up, bool down, bool left, bool right, Map* map) {
    Vec2 nextPos = pos;
    isMoving = false;

    if (up) {
        nextPos.y -= speed;
        dir = PlayerDir::Up;
        isMoving = true;
    }
    if (down) {
        nextPos.y += speed;
        dir = PlayerDir::Down;
        isMoving = true;
    }
    if (left) {
        nextPos.x -= speed;
        dir = PlayerDir::Left;
        isMoving = true;
    }
    if (right) {
        nextPos.x += speed;
        dir = PlayerDir::Right;
        isMoving = true;
    }

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

    if (pos.x < 0) pos.x = 0;
    if (pos.y < 0) pos.y = 0;
    if (pos.x > MAP_WIDTH * TILE_SIZE - size) pos.x = MAP_WIDTH * TILE_SIZE - size;
    if (pos.y > MAP_HEIGHT * TILE_SIZE - size) pos.y = MAP_HEIGHT * TILE_SIZE - size;

    DWORD now = GetTickCount();
    if (now - lastFrameTime >= frameDelay) {
        int maxFrame = GetFrameCount();
        frame = (frame + 1) % maxFrame;
        lastFrameTime = now;
    }
}

void Player::Draw(HDC hdc, const Camera& cam, int offsetY) {
    CImage* sheet = isMoving ? &walkSheet : &idleSheet;

    int drawX = (int)(pos.x - cam.x);
    int drawY = (int)(pos.y - cam.y) + offsetY;

    if (!sheet->IsNull()) {
        int frameWidth = sheet->GetWidth() / GetFrameCount();
        int frameHeight = sheet->GetHeight() / 4;
        int sourceX = (frame % GetFrameCount()) * frameWidth;
        int sourceY = GetDirectionRow() * frameHeight;

        sheet->Draw(
            hdc,
            drawX,
            drawY,
            (int)size,
            (int)size,
            sourceX,
            sourceY,
            frameWidth,
            frameHeight
        );
    }
}

void Player::DrawInventory(HDC hdc, const Camera& cam, int offsetY) {
    int drawX = (int)(pos.x - cam.x);
    int drawY = (int)(pos.y - cam.y) + offsetY;

    wchar_t text[64];
    swprintf_s(text, L"나무 : %d / 돌 : %d", wood, stone);

    int oldBkMode = SetBkMode(hdc, TRANSPARENT);
    COLORREF oldColor = SetTextColor(hdc, RGB(255, 255, 255));
    HFONT font = CreateFontW(18, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Arial");
    HFONT oldFont = (HFONT)SelectObject(hdc, font);

    SetTextColor(hdc, RGB(0, 0, 0));
    TextOutW(hdc, drawX, drawY, text, (int)wcslen(text));

    SelectObject(hdc, oldFont);
    DeleteObject(font);
    SetTextColor(hdc, oldColor);
    SetBkMode(hdc, oldBkMode);
}
