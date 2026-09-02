#pragma once
#include <array>
#include <vector>
#include "../game/Move.h"

struct CountTable {
    std::vector<std::array<float, 3>> rows;

    explicit CountTable(int contexts) : rows(contexts, {0.0f, 0.0f, 0.0f}) {}

    Move predict(int ctx) const {
        int best = 0;
        for (int i = 1; i < 3; ++i) {
            if (rows[ctx][i] > rows[ctx][best]) best = i;
        }
        return kAllMoves[best];
    }

    Move bestResponse(int ctx) const {
        int best = 0;
        float bestScore = -1e6;
        for (int i = 0; i < 3; ++i) {
            float score = 0.0f;
            for (int j = 0; j < 3; ++j) {
                Outcome o = resolve(kAllMoves[i], kAllMoves[j]);
                score += rows[ctx][j] * (o == Outcome::Win ? 1.0f : (o == Outcome::Loss ? -1.0f : 0.0f));
            }
            if (score > bestScore) {
                bestScore = score;
                best = i;
            }
        }
        return kAllMoves[best];
    }

    void update(int ctx, Move actual) {
        for (int i = 0; i < 3; ++i) rows[ctx][i] *= 0.97f;
        rows[ctx][static_cast<int>(actual)] += 1.0f;
    }
};