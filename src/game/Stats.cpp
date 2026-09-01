#include "Stats.h"

void Stats::apply(Record& r, Outcome o) {
    switch (o) {
        case Outcome::Win: r.wins++; break;
        case Outcome::Loss: r.losses++; break;
        case Outcome::Draw: r.draws++; break;
    }
}

void Stats::record(Outcome vsRandom, Outcome vsAI) {
    apply(vsRandom_, vsRandom);
    apply(vsAI_, vsAI);
}
