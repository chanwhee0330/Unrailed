#pragma once
#include "Defines.h"

class Train;
class Bomb;

class GameManager
{
public:
    GameManager();
    ~GameManager();
    void Init(Train* t1, Train* t2, Bomb* bomb);
    void Update();
private:
    Train* m_pTrain1;
    Train* m_pTrain2;
    Bomb*  m_pBomb;
};
