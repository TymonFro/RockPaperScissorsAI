#pragma once
#include <optional>
#include <SFML/Graphics.hpp>
#include <deque>
#include "../ai/NeuralOpponent.h"
#include "../game/Move.h"
#include "../game/RandomOpponent.h"
#include "../game/Stats.h"

struct GameEvent{
    Move playersMove, randomMove, aiMove;
    Outcome vsRandom, vsAI;
};

class App {
public:
    App();
    ~App();

    void run();

private:
    void processEvents();
    void update();
    void draw();
    void playRound(Move playerMove);

    sf::RenderWindow window_;
    sf::Clock deltaClock_;

    RandomOpponent randomOpponent_;
    NeuralOpponent neuralOpponent_;
    Stats stats_;

    std::optional<Move> lastRandomMove_;
    std::optional<Move> lastAIMove_;
    int roundNumber_ = 0;


    std::deque<GameEvent>gamesHistory;
};
