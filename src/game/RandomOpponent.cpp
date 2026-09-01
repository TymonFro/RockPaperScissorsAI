#include "RandomOpponent.h"

RandomOpponent::RandomOpponent()
    : rng_(std::random_device{}()), dist_(0, 2) {}

Move RandomOpponent::pick() {
    return kAllMoves[dist_(rng_)];
}
