#include "Player.h"
#include "Camera.h"

Player::Player(int num) : m_num(num), m_x(0), m_y(0) {}
Player::~Player() {}
void Player::Init() {}
void Player::Update() {}
void Player::Render(HDC hdc, Camera* camera) {}
void Player::OnKeyDown(WPARAM key) {}
void Player::OnKeyUp(WPARAM key) {}
