#include <array>
#include <functional>
#include <iostream>
#include <random>
#include <string>

#include "ai/NeuralOpponent.h"

// Strategia gracza: dostaje numer rundy, ruch AI i wynik (z perspektywy gracza) z POPRZEDNIEJ
// rundy, zwraca swoj ruch w tej rundzie. Dla round==0 lastAiMove/lastOutcome sa niezdefiniowane
// (przekazujemy Move::Rock / Outcome::Draw jako neutralny placeholder).
using PlayerStrategy = std::function<Move(int round, Move lastAiMove, Outcome lastOutcome)>;

PlayerStrategy constantStrategy(Move m) {
    return [m](int, Move, Outcome) { return m; };
}

PlayerStrategy cycleStrategy() {
    static const std::array<Move, 3> cyc = {Move::Rock, Move::Paper, Move::Scissors};
    return [](int round, Move, Outcome) { return cyc[round % 3]; };
}

// 20 rund tego samego ruchu, potem 20 rund cyklu, na przemian.
PlayerStrategy constantThenCycleStrategy() {
    static const std::array<Move, 3> cyc = {Move::Rock, Move::Paper, Move::Scissors};
    return [](int round, Move, Outcome) {
        const int phaseLen = 20;
        const int phase = (round / phaseLen) % 2;
        const int r = round % phaseLen;
        return phase == 0 ? Move::Rock : cyc[r % 3];
    };
}

// Rotacja miedzy trzema stalymi ruchami co 15 rund - test szybkosci re-adaptacji
// po nagle zmienie (prostej) strategii gracza.
PlayerStrategy rotatingConstantStrategy() {
    static const std::array<Move, 3> rota = {Move::Rock, Move::Scissors, Move::Paper};
    return [](int round, Move, Outcome) {
        const int phaseLen = 15;
        return rota[(round / phaseLen) % 3];
    };
}

// Ludzka heurystyka "win-stay / lose-shift": po wygranej gracz powtarza ruch, po przegranej
// przechodzi na ruch, ktory pobilby ruch AI z poprzedniej rundy (naturalny odruch "rewanzu"),
// po remisie zmienia na kolejny w cyklu.
PlayerStrategy winStayLoseShiftStrategy() {
    return [lastPlayerMove = Move::Rock](int round, Move lastAiMove, Outcome lastOutcome) mutable {
        if (round == 0) {
            lastPlayerMove = Move::Rock;
            return lastPlayerMove;
        }
        if (lastOutcome == Outcome::Win) {
            // stay
        } else if (lastOutcome == Outcome::Loss) {
            lastPlayerMove = counterMove(lastAiMove);  // "rewanz" za ruch AI ktory wygral
        } else {
            lastPlayerMove = counterMove(lastPlayerMove);
        }
        return lastPlayerMove;
    };
}

PlayerStrategy uniformRandomStrategy() {
    return [rng = std::mt19937{std::random_device{}()}](int, Move, Outcome) mutable {
        std::uniform_int_distribution<int> dist(0, 2);
        return kAllMoves[dist(rng)];
    };
}

void runScenario(const std::string& name, PlayerStrategy strategy, int rounds, int windowSize) {
    std::cout << "\n=== " << name << " (" << rounds << " rund) ===\n";

    NeuralOpponent ai(/*persist=*/false);  // zawsze swieza, losowa siec - brak wplywu na defaultNet.txt

    Move lastAiMove = Move::Rock;
    Outcome lastOutcome = Outcome::Draw;

    int wWin = 0, wLoss = 0, wDraw = 0;
    long long totalWin = 0, totalLoss = 0, totalDraw = 0;

    for (int round = 0; round < rounds; ++round) {
        const Move playerMove = strategy(round, lastAiMove, lastOutcome);
        const Move aiMove = ai.predict();
        const Outcome outcome = resolve(playerMove, aiMove);  // z perspektywy gracza

        if (outcome == Outcome::Win) { wWin++; totalWin++; }
        else if (outcome == Outcome::Loss) { wLoss++; totalLoss++; }
        else { wDraw++; totalDraw++; }

        ai.learn(aiMove, playerMove);
        ai.update(playerMove, aiMove);

        lastAiMove = aiMove;
        lastOutcome = outcome;

        if ((round + 1) % windowSize == 0 || round + 1 == rounds) {
            const int n = wWin + wLoss + wDraw;
            std::printf("  rundy %4d-%4d: gracz=%2d (%3.0f%%)  AI=%2d (%3.0f%%)  remisy=%2d (%3.0f%%)\n",
                        round + 2 - n, round + 1,
                        wWin, 100.0 * wWin / n, wLoss, 100.0 * wLoss / n, wDraw, 100.0 * wDraw / n);
            wWin = wLoss = wDraw = 0;
        }
    }

    std::printf("  RAZEM: gracz=%.1f%%  AI=%.1f%%  remisy=%.1f%%\n",
                100.0 * totalWin / rounds, 100.0 * totalLoss / rounds, 100.0 * totalDraw / rounds);
}

int main() {
    runScenario("Staly ruch (Kamien)", constantStrategy(Move::Rock), 200, 25);
    runScenario("Cykl Kamien->Papier->Nozyce", cycleStrategy(), 300, 30);
    runScenario("Win-stay / lose-shift", winStayLoseShiftStrategy(), 300, 30);
    runScenario("20 rund stale + 20 rund cyklu (na przemian)", constantThenCycleStrategy(), 360, 20);
    runScenario("Rotacja stalych ruchow co 15 rund", rotatingConstantStrategy(), 360, 15);
    runScenario("Prawdziwie losowe (kontrolne, sufit ~33%)", uniformRandomStrategy(), 300, 30);
    return 0;
}
