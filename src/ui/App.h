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
    void resetSession();
    void switchUser(const std::string& username);
    void requestSwitchUser(const std::string& username);

    sf::RenderWindow window_;
    sf::Clock deltaClock_;

    RandomOpponent randomOpponent_;
    NeuralOpponent neuralOpponent_;
    Stats stats_;

    std::optional<Move> lastRandomMove_;
    std::optional<Move> lastAIMove_;
    int roundNumber_ = 0;

    std::string currentUser_ = "default";
    bool showNewUserInput = false;
    char newUserInput[32] = "";
    bool wasSaved = 1;
    std::string pendingUser_;
    bool confirmSwitchPending_ = false;
    bool confirmClosePending_ = false;
    std::deque<GameEvent>gamesHistory;

    std::vector<std::string> listUsers() ;
};
