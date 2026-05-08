#include "Train.h"
#include "Camera.h"

Train::Train(int num) : m_num(num), m_x(0), m_y(0), m_derailed(false), m_overheated(false), m_pImage(nullptr) {}
Train::~Train()
{
	delete m_pImage;
}

void Train::Init() 
{
	m_pImage = new Image(L"train/locomoto.png");
	m_lastTime = GetTickCount();
}
void Train::Update() 
{
	DWORD currentTime = GetTickCount();
	DWORD elapsed = currentTime - m_lastTime;  // 경과 시간(ms)

	if (elapsed >= 100)  // 100ms마다 실행
	{
		// 기차 이동 로직
		m_x += m_speed;

		m_lastTime = currentTime;
	}
}
void Train::Render(HDC hdc, Camera* camera) 
{
	int screenX = (int)(m_x - camera->GetX());
	int screenY = (int)(m_y - camera->GetY());

	Graphics graphics(hdc);
	graphics.DrawImage(m_pImage, screenX, screenY, 64, 32);  // 가로64 세로32
}

