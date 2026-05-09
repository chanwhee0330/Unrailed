#include "Train.h"

Train::Train(float x, float y, int direction, const wchar_t* imagePath)
    : pos({ x, y }), speed(5.0f), heat(0.0f), dir(direction), finished(false) {
    image = new Bitmap(imagePath);
    lastTime = GetTickCount64();
}

Train::~Train() 
{
    delete image;
   
}

void Train::Update() {
    if (finished) return;

    // 과열 상태면 이동 정지 (열은 계속 쌓임)
    if (!IsOverheated()) {
        ULONGLONG now = GetTickCount64();
        float     delta = (now - lastTime) / 1000.0f;
        lastTime = now;

        pos.x += speed * dir * delta;

        // 오른쪽 끝 도달 (기차 1)
        if (dir == 1 && pos.x >= MAP_WIDTH * TILE_SIZE)
            finished = true;

        // 왼쪽 끝 도달 (기차 2)
        if (dir == -1 && pos.x <= 0)
            finished = true;
    }

    // 열 누적
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