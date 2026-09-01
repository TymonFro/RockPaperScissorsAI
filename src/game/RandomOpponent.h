#pragma once
#include <random>
#include "Move.h"

class RandomOpponent {
public:
    RandomOpponent();
    Move pick();

private:
    std::mt19937 rng_;
    std::uniform_int_distribution<int> dist_;
};
