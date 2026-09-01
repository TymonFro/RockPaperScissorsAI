#pragma once
#include <array>
#include <random>
#include "../game/Move.h"
#include "Network.h"

Move counterMove(Move m);

class NeuralOpponent {
public:
    // persist=false: nigdy nie czyta/zapisuje "defaultNet.txt" (zawsze startuje z losowej
    // sieci w pamieci) - do uzytku w benchmarku, zeby nie nadpisywac realnego zapisu z gry.
    explicit NeuralOpponent(bool persist = true);
    void activateNetwork();

    Move predict();
    void update(Move playerMove, Move aiMove);
    void learn(Move aiMove, Move playersMove);
private:
    std::mt19937 rng_;
    Network net;
    deque<pair<Move, Move>> moveHistory;
    vector<float> input;
    vector<float> targetOutput;
    vector<float> outputGradients;
    int roundsPlayed = 0;
    bool persist_ = true;
};
