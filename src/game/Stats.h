#pragma once
#include "Move.h"

struct Record {
    int wins = 0;
    int losses = 0;
    int draws = 0;

    int total() const { return wins + losses + draws; }
    float winRate() const { return total() == 0 ? 0.f : static_cast<float>(wins) / total(); }
    float winRateND() const { return total() == 0 ? 0.f : static_cast<float>(wins) / (static_cast<float>(wins + losses)); }
};

class Stats {
public:
    void record(Outcome vsRandom, Outcome vsAI);

    const Record& vsRandom() const { return vsRandom_; }
    const Record& vsAI() const { return vsAI_; }

private:
    static void apply(Record& r, Outcome o);

    Record vsRandom_;
    Record vsAI_;
};
