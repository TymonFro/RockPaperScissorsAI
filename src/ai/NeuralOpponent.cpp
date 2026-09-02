#include "NeuralOpponent.h"
#include <filesystem>

Move counterMove(Move m) {
    switch (m) {
        case Move::Rock: return Move::Paper;
        case Move::Paper: return Move::Scissors;
        case Move::Scissors: return Move::Rock;
    }
    return Move::Rock;
}

NeuralOpponent::NeuralOpponent(bool persist) : rng_(std::random_device{}()), persist_(persist) {
    if (persist_) loadFromFile(currentPath_);
    else genNet(net);
    moveHistory.assign(histryWindow, {Move::Rock, Move::Rock});
    input.resize(inLayerSize);
    targetOutput.resize(outputNeurons);
    outputGradients.resize(outputNeurons);

}

void NeuralOpponent::loadFromFile(const std::string& path) {
    std::filesystem::create_directories(std::filesystem::path(path).parent_path());
    loadNet(net, path);
    currentPath_ = path;
    moveHistory.assign(histryWindow, {Move::Rock, Move::Rock});
    roundsPlayed = 0;
}

void  NeuralOpponent::saveToFile(const std::string& path) {
    std::filesystem::create_directories(std::filesystem::path(path).parent_path());
    saveNet(net, path);
}

inline float evaluate(Move a, vector<float>& output) {
    float score = 0.0f;
    for (int i = 0; i < 3; ++i) {
        score += output[i] * (resolve(a, kAllMoves[i]) == Outcome::Win ? 1.0f : (resolve(a, kAllMoves[i]) == Outcome::Loss ? -1.0f : 0.0f));
    }
    return score;
}

Move NeuralOpponent::predict() {
    int best = -1e6;
    float bestScore = -1e6;

    input.assign(inLayerSize, 0.0f);
    for (int i = max(0, histryWindow - roundsPlayed); i < histryWindow; ++i) {
        input[i * encodeSize + static_cast<int>(moveHistory[i].first)] = 1.0f; // Player's move
        input[i * encodeSize + 3 + static_cast<int>(moveHistory[i].second)] = 1.0f; // AI's move
    }

    net.forwardPropagation(input);
    //const vector<float>& output = net.l3.neuronsOutput;

    for(int i = 0; i < 3; ++i){
        if(evaluate(kAllMoves[i], net.l3.neuronsOutput) > bestScore){
            best = i;
            bestScore = evaluate(kAllMoves[i], net.l3.neuronsOutput);
        }
    }

    return kAllMoves[best];
}

void NeuralOpponent::learn(Move aiMove, Move playersMove) {
    //Outcome outcome = resolve(playersMove, aiMove);
    
    targetOutput.assign(outputNeurons, 0.0f);
    targetOutput[static_cast<int>(playersMove)] = 1.0f;

    outputGradients.assign(outputNeurons, 0.0f);
    for (int i = 0; i < outputNeurons; ++i) {
        outputGradients[i] = net.l3.neuronsOutput[i] - targetOutput[i];
    }

    net.backPropagation(outputGradients);
    net.updateWeigths();
}

void NeuralOpponent::update(Move playerMove, Move aiMove) {
    moveHistory.pop_front();
    moveHistory.push_back({playerMove, aiMove});
    roundsPlayed++;
}
