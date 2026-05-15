#include "Train.h"

Train::Train(float x, float y, int direction, const wchar_t* imagePath)
    : pos({ x, y }), speed(5.0f), heat(0.0f), dirX((float)direction), dirY(0.0f), finished(false) {
    image = new Bitmap(imagePath);
    lastTime = GetTickCount64();
}

Train::~Train() 
{
    delete image;
   
}

void Train::UpdateDirection(RailDir rd) {
    switch (rd) {
    case RailDir::HORIZONTAL: dirY = 0;  break;
    case RailDir::VERTICAL:   dirX = 0;  break;
    case RailDir::TURN_RD:
        if (dirX < 0) { dirX = 0;  dirY = 1; }
        else if (dirY < 0) { dirX = 1;  dirY = 0; }
        break;
    case RailDir::TURN_LD:
        if (dirX > 0) { dirX = 0;  dirY = 1; }
        else if (dirY < 0) { dirX = -1; dirY = 0; }
        break;
    case RailDir::TURN_RU:
        if (dirX < 0) { dirX = 0;  dirY = -1; }
        else if (dirY > 0) { dirX = 1;  dirY = 0; }
        break;
    case RailDir::TURN_LU:
        if (dirX > 0) { dirX = 0;  dirY = -1; }
        else if (dirY > 0) { dirX = -1; dirY = 0; }
        break;
    }
}
void Train::Update(Rail* rail) {
    if (finished) return;
    if (!IsOverheated()) {
        ULONGLONG now = GetTickCount64();
        float delta = (now - lastTime) / 1000.0f;
        lastTime = now;

        // 현재 타일 확인
        int tileX = (int)((pos.x + 48) / TILE_SIZE);
        int tileY = (int)((pos.y + 24) / TILE_SIZE);

        if (rail->HasRail(tileX, tileY)) {
            // 현재 타일 레일 방향으로 방향 업데이트 후 이동
            UpdateDirection(rail->GetDir(tileX, tileY));
            pos.x += speed * dirX * delta;
            pos.y += speed * dirY * delta;
            wchar_t buf[200];
            swprintf(buf, 200, L"trainTileX=%d trainTileY=%d hasRail=%d\n",
                tileX, tileY, rail->HasRail(tileX, tileY));
            OutputDebugStringW(buf);
        }
        else {
            finished = true;
        }
    }
    heat += HEAT_RATE;
    if (heat > MAX_HEAT) heat = MAX_HEAT;
}

void Train::Draw(Graphics* g) {
    // 기차 몸체
    SolidBrush bodyBrush(Color(255, 80, 80, 80));
    g->DrawImage(image, pos.x, pos.y, 96.0f, 48.0f);

    // 과열 상태면 붉은 오버레이
    if (IsOverheated()) {
        SolidBrush overHeatBrush(Color(120, 255, 0, 0));
        g->FillRectangle(&overHeatBrush, pos.x, pos.y, 96.0f, 48.0f);
    }

    DrawHeatBar(g);
}

void Train::Cool(float amount) {
    heat -= amount;
    if (heat < 0.0f) heat = 0.0f;
}

void Train::DrawHeatBar(Graphics* g) {
    const float barW = 96.0f;
    const float barH = 8.0f;
    const float barX = pos.x;
    const float barY = pos.y - 14.0f;
    float       ratio = heat / MAX_HEAT;

    // 배경
    SolidBrush bgBrush(Color(255, 0, 0, 0));
    g->FillRectangle(&bgBrush, barX, barY, barW, barH);

    // 게이지 (초록 → 빨강)
    BYTE r = (BYTE)(255 * ratio);
    BYTE g_ = (BYTE)(255 * (1.0f - ratio));
    SolidBrush heatBrush(Color(255, r, g_, 0));
    g->FillRectangle(&heatBrush, barX, barY, barW * ratio, barH);
}