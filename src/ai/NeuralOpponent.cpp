#include "NeuralOpponent.h"
#include <filesystem>
#include <iostream>

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
    scores.fill(0.0f);
    m0Table.clear(); m1Table.clear(); m2Table.clear(); wslsTable.clear();

    std::ifstream fin(path + ".tables");
    if (fin.good()) {
        if (!(m0Table.load(fin) && m1Table.load(fin) && m2Table.load(fin) && wslsTable.load(fin))) {
            m0Table.clear(); m1Table.clear(); m2Table.clear(); wslsTable.clear();
        }
    }

}

void  NeuralOpponent::saveToFile(const std::string& path) {
    std::filesystem::create_directories(std::filesystem::path(path).parent_path());
    saveNet(net, path);
    std::ofstream fout(path + ".tables");
    m0Table.save(fout);
    m1Table.save(fout);
    m2Table.save(fout);
    wslsTable.save(fout);
    fout.close();
}

inline float evaluate(Move a, vector<float>& output) {
    float score = 0.0f;
    for (int i = 0; i < 3; ++i) {
        score += output[i] * (resolve(a, kAllMoves[i]) == Outcome::Win ? 1.0f : (resolve(a, kAllMoves[i]) == Outcome::Loss ? -1.0f : 0.0f));
    }
    return score;
}

Move NeuralOpponent::predict() {
    // Network
    
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

    lastPredictions[0] = kAllMoves[best];

    // other predictors

    Move p1 = moveHistory[histryWindow - 1].first;
    Move p2 = moveHistory[histryWindow - 2].first;
    Outcome lastOut = resolve(moveHistory[histryWindow - 1].first, moveHistory[histryWindow - 1].second);

    lastCtx[0] = 0;
    lastCtx[1] = static_cast<int>(p1);
    lastCtx[2] = static_cast<int>(p2) * 3 + static_cast<int>(p1);
    lastCtx[3] = static_cast<int>(p1) * 3 + static_cast<int>(lastOut);

    //lastPredictions[1] = counterMove(m0Table.predict(lastCtx[0]));
    lastPredictions[2] = counterMove(m1Table.predict(lastCtx[1]));
    lastPredictions[3] = counterMove(m2Table.predict(lastCtx[2]));
    lastPredictions[4] = counterMove(wslsTable.predict(lastCtx[3]));

    // lastPredictions[5] = m0Table.bestResponse(lastCtx[0]);
    lastPredictions[5] = m1Table.bestResponse(lastCtx[1]);
    lastPredictions[6] = m2Table.bestResponse(lastCtx[2]);
    lastPredictions[7] = wslsTable.bestResponse(lastCtx[3]);

    lastPredictions[1] = counterMove(lastPredictions[3]);
    // lastPredictions[10] = counterMove(lastPredictions[3]);
    // lastPredictions[11] = counterMove(lastPredictions[4]);
    // lastPredictions[12] = counterMove(counterMove(lastPredictions[2]));
    // lastPredictions[13] = counterMove(counterMove(lastPredictions[3]));
    // lastPredictions[14] = counterMove(counterMove(lastPredictions[4]));

    // rand Predictor

    lastPredictions[kPredictors -1] = kAllMoves[std::uniform_int_distribution<int>(0, 2)(rng_)];

    
    for(int i = 0; i < kPredictors; ++i){
        if(scores[i] > scores[choosenPredictor]){
            choosenPredictor = i;
        }
    }

    // if(choosenPredictor == 0) {
    //     neuralUsedCnt++;
    //     cerr << "NeuralOpponent used " << neuralUsedCnt << " times" << endl;
    // }

    // float w[3] = {0.0f, 0.0f, 0.0f}; // glosowanie
    // for (int i = 0; i < kPredictors; ++i) {
    //     w[static_cast<int>(lastPredictions[i])] += std::exp(scores[i]);
    // }
    // int bestMove = 0;
    // for (int i = 1; i < 3; ++i) if (w[i] > w[bestMove]) bestMove = i;
    // return kAllMoves[bestMove];

    return lastPredictions[choosenPredictor];
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
    for(int i = 0; i < kPredictors; ++i){
        switch(resolve(lastPredictions[i], playerMove)){
            case Outcome::Win: scores[i] = scores[i]*0.9f + 1.0f; break;
            case Outcome::Loss: scores[i] = scores[i]*0.9f - 1.0f; break;
            case Outcome::Draw: scores[i] = scores[i]*0.9f + 0.0f; break;
        }
    }
    scores[kPredictors -1] = 0;

    m0Table.update(lastCtx[0], playerMove);
    m1Table.update(lastCtx[1], playerMove);
    m2Table.update(lastCtx[2], playerMove);
    wslsTable.update(lastCtx[3], playerMove);


    moveHistory.pop_front();
    moveHistory.push_back({playerMove, aiMove});
    roundsPlayed++;
}
