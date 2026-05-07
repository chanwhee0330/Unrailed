#include "Train.h"
#include "Camera.h"

Train::Train(int num) : m_num(num), m_x(0), m_y(0), m_derailed(false), m_overheated(false) {}
Train::~Train() {}
void Train::Init() {}
void Train::Update() {}
void Train::Render(HDC hdc, Camera* camera) {}
