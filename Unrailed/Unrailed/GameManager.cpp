#include "GameManager.h"
#include "Train.h"
#include "Bomb.h"

GameManager::GameManager() : m_pTrain1(nullptr), m_pTrain2(nullptr), m_pBomb(nullptr) {}
GameManager::~GameManager() {}
void GameManager::Init(Train* t1, Train* t2, Bomb* bomb) { m_pTrain1=t1; m_pTrain2=t2; m_pBomb=bomb; }
void GameManager::Update() {}
