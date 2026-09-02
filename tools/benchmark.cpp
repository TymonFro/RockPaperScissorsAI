#include <array>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <random>
#include <string>
#include <utility>
#include <vector>

#include "ai/NeuralOpponent.h"

// Strategia gracza: dostaje numer rundy, ruch AI i wynik (z perspektywy gracza) z POPRZEDNIEJ
// rundy, zwraca swoj ruch w tej rundzie. Dla round==0 lastAiMove/lastOutcome sa niezdefiniowane
// (przekazujemy Move::Rock / Outcome::Draw jako neutralny placeholder).
using PlayerStrategy = std::function<Move(int round, Move lastAiMove, Outcome lastOutcome)>;

// Fabryka: kazde powtorzenie scenariusza dostaje swiezy egzemplarz strategii (bo czesc
// strategii ma wlasny stan wewnetrzny, ktory nie moze przeciekac miedzy przebiegami).
using StrategyFactory = std::function<PlayerStrategy()>;

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

// "Strategiczny": krotkie fazy (12 rund) roznych wzorcow, zmieniane w kolko. Modeluje
// gracza, ktory swiadomie zmienia taktyke zanim AI zdazy sie na dobre douczyc - tu liczy
// sie szybkosc adaptacji, a nie pojemnosc na zapamietanie jednego dlugiego wzorca.
PlayerStrategy strategicMixStrategy() {
    static const std::array<Move, 3> cyc = {Move::Rock, Move::Paper, Move::Scissors};
    return [](int round, Move, Outcome) {
        const int phaseLen = 12;
        const int phase = (round / phaseLen) % 4;
        const int r = round % phaseLen;
        switch (phase) {
            case 0: return Move::Paper;                 // staly ruch
            case 1: return cyc[r % 3];                   // cykl w przod
            case 2: return cyc[2 - (r % 3)];             // cykl w tyl
            default: return cyc[(r / 2) % 3];            // kazdy ruch dwa razy pod rzad
        }
    };
}

// "Strategiczny-losowy": co ~12 rund gracz przerzuca sie na LOSOWO wybrany nowy wzorzec.
// Roznica wzgledem strategicMix: tam fazy leca w kolko w tej samej kolejnosci, wiec siec moze
// zapamietac sam meta-wzorzec. Tutaj nie ma czego zapamietac - liczy sie wylacznie to, jak
// szybko siec lapie KAZDY nowy wzorzec od zera. To najblizszy model czlowieka, ktory co
// kilkanascie rund wymysla nowa sztuczke.
PlayerStrategy randomPhaseStrategy() {
    return [rng = std::mt19937{std::random_device{}()}, phase = 0, phaseEnd = 0,
            kind = 0, base = 0](int round, Move, Outcome) mutable {
        if (round >= phaseEnd) {
            std::uniform_int_distribution<int> kindDist(0, 3);
            std::uniform_int_distribution<int> baseDist(0, 2);
            std::uniform_int_distribution<int> lenDist(8, 16);
            kind = kindDist(rng);
            base = baseDist(rng);
            phase = round;
            phaseEnd = round + lenDist(rng);
        }
        const int r = round - phase;
        switch (kind) {
            case 0: return kAllMoves[base];                       // staly ruch
            case 1: return kAllMoves[(base + r) % 3];             // cykl w przod
            case 2: return kAllMoves[(base + 3 - (r % 3)) % 3];   // cykl w tyl
            default: return kAllMoves[(base + r / 2) % 3];        // kazdy ruch dwa razy
        }
    };
}

// "Anty-AI": gracz czyta bota - patrzy co AI gralo ostatnio i gra ruch bijacy jego
// najczestszy ostatni wybor. Najblizszy odpowiednik czlowieka, ktory analizuje przeciwnika.
PlayerStrategy antiAiStrategy() {
    return [recent = std::deque<Move>{}](int round, Move lastAiMove, Outcome) mutable {
        if (round > 0) {
            recent.push_back(lastAiMove);
            if (recent.size() > 6) recent.pop_front();
        }
        if (recent.empty()) return Move::Rock;

        std::array<int, 3> counts{0, 0, 0};
        for (Move m : recent) counts[static_cast<int>(m)]++;

        int best = 0;
        for (int i = 1; i < 3; ++i) {
            if (counts[i] > counts[best]) best = i;
        }
        return counterMove(kAllMoves[best]);
    };
}

// "Ludzka losowosc": czlowiek proszony o losowe ruchy unika powtarzania tego samego -
// dobrze udokumentowane obciazenie poznawcze. Rozklad prawie rowny, ale powtorzenie
// poprzedniego ruchu jest wyraznie rzadsze niz 1/3.
PlayerStrategy humanRandomStrategy() {
    return [rng = std::mt19937{std::random_device{}()}, last = -1](int, Move, Outcome) mutable {
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);
        int pick;
        do {
            pick = static_cast<int>(dist(rng) * 3.0f) % 3;
        } while (pick == last && dist(rng) < 0.75f);  // 75% szans na odrzucenie powtorki
        last = pick;
        return kAllMoves[pick];
    };
}

// "Drugie dno": gracz zaklada, ze AI skontruje jego poprzedni ruch, i gra ruch bijacy
// TEN kontr-ruch ("wiem, ze ty wiesz"). Inny poziom rozumowania niz anty-AI, ktory patrzy
// na czestosci. Ta strategia jest CELOWO nieuzywana w zadnej mieszance treningowej -
// sluzy wylacznie jako held-out do oceny ziarna trenowanego na pelnej mieszance.
PlayerStrategy secondGuessStrategy() {
    return [lastPlayerMove = Move::Rock](int round, Move lastAiMove, Outcome) mutable {
        if (round == 0) return lastPlayerMove;
        if (lastAiMove == counterMove(lastPlayerMove)) {
            // AI skontrowalo moj poprzedni ruch - zaloz, ze zrobi to znowu, i wyprzedz kontre
            lastPlayerMove = counterMove(counterMove(lastPlayerMove));
        } else {
            // AI zagralo cos innego - po prostu pobij to, co zagralo
            lastPlayerMove = counterMove(lastAiMove);
        }
        return lastPlayerMove;
    };
}

void runScenario(const std::string& name, const StrategyFactory& factory, int rounds,
                 int windowSize, int repeats, const std::string& seedPath = "") {
    std::cout << "\n=== " << name << " (" << rounds << " rund";
    if (repeats > 1) std::cout << " x" << repeats << " przebiegow";
    if (!seedPath.empty()) std::cout << ", start z ziarna";
    std::cout << ") ===\n";

    long long grandWin = 0, grandLoss = 0, grandDraw = 0;

    for (int rep = 0; rep < repeats; ++rep) {
        NeuralOpponent ai(/*persist=*/false);  // swieza, losowa siec - nie dotyka plikow users/
        if (!seedPath.empty()) ai.loadFromFile(seedPath);  // ...albo start z wytrenowanego ziarna
        PlayerStrategy strategy = factory();

        Move lastAiMove = Move::Rock;
        Outcome lastOutcome = Outcome::Draw;

        int wWin = 0, wLoss = 0, wDraw = 0;

        for (int round = 0; round < rounds; ++round) {
            const Move playerMove = strategy(round, lastAiMove, lastOutcome);
            const Move aiMove = ai.predict();
            const Outcome outcome = resolve(playerMove, aiMove);  // z perspektywy gracza

            if (outcome == Outcome::Win) { wWin++; grandWin++; }
            else if (outcome == Outcome::Loss) { wLoss++; grandLoss++; }
            else { wDraw++; grandDraw++; }

            ai.learn(aiMove, playerMove);
            ai.update(playerMove, aiMove);

            lastAiMove = aiMove;
            lastOutcome = outcome;

            // Rozbicie na okna pokazujemy tylko przy pojedynczym przebiegu - przy usrednianiu
            // po wielu przebiegach interesuje nas juz tylko podsumowanie.
            if (repeats == 1 && ((round + 1) % windowSize == 0 || round + 1 == rounds)) {
                const int n = wWin + wLoss + wDraw;
                std::printf("  rundy %4d-%4d: gracz=%2d (%3.0f%%)  AI=%2d (%3.0f%%)  remisy=%2d (%3.0f%%)\n",
                            round + 2 - n, round + 1,
                            wWin, 100.0 * wWin / n, wLoss, 100.0 * wLoss / n, wDraw, 100.0 * wDraw / n);
                wWin = wLoss = wDraw = 0;
            }
        }
    }

    const double total = static_cast<double>(rounds) * repeats;
    std::printf("  RAZEM: gracz=%.1f%%  AI=%.1f%%  remisy=%.1f%%\n",
                100.0 * grandWin / total, 100.0 * grandLoss / total, 100.0 * grandDraw / total);
}

// Trening ziarna. WAZNE: uczymy WYLACZNIE na strategiach "treningowych" (stale, cykle,
// win-stay/lose-shift, ludzka losowosc). Scenariusze STRATEGICZNY-LOSOWY i ANTY-AI sa
// swiadomie WYLACZONE z treningu - sluza potem jako held-out, czyli uczciwy sprawdzian
// czy ziarno niesie ogolna wiedze o ludzkich tendencjach, czy tylko zapamietalo skrypty.
void trainSeed(const std::string& path, int blockRounds, int passes, bool holdout) {
    // Mieszanka bazowa - uzywana w obu trybach.
    std::vector<std::pair<std::string, StrategyFactory>> training = {
        {"staly Kamien",        [] { return constantStrategy(Move::Rock); }},
        {"staly Papier",        [] { return constantStrategy(Move::Paper); }},
        {"staly Nozyce",        [] { return constantStrategy(Move::Scissors); }},
        {"cykl",                [] { return cycleStrategy(); }},
        {"stale+cykl",          [] { return constantThenCycleStrategy(); }},
        {"rotacja stalych",     [] { return rotatingConstantStrategy(); }},
        {"win-stay/lose-shift", [] { return winStayLoseShiftStrategy(); }},
        {"ludzka losowosc",     [] { return humanRandomStrategy(); }},
    };

    // Tryb pelny (domyslny, pod realna gre): dokladamy przeciwnikow adaptacyjnych,
    // zeby ziarno nauczylo sie tez nie byc czytelnym dla kogos, kto kontruje bota.
    // Tryb --holdout (pod ocene): pomijamy je, zeby scenariusze benchmarku
    // STRATEGICZNY / STRATEGICZNY-LOSOWY / ANTY-AI zostaly uczciwym sprawdzianem.
    if (!holdout) {
        training.push_back({"strategiczny",        [] { return strategicMixStrategy(); }});
        training.push_back({"strategiczny-losowy", [] { return randomPhaseStrategy(); }});
        training.push_back({"anty-AI",             [] { return antiAiStrategy(); }});
        training.push_back({"czysto losowy",       [] { return uniformRandomStrategy(); }});
    }

    NeuralOpponent ai(/*persist=*/false);  // start od losowych wag (genNet)
    long long win = 0, loss = 0, draw = 0;

    for (int pass = 0; pass < passes; ++pass) {
        for (const auto& [name, factory] : training) {
            PlayerStrategy strategy = factory();
            Move lastAiMove = Move::Rock;
            Outcome lastOutcome = Outcome::Draw;

            for (int round = 0; round < blockRounds; ++round) {
                const Move playerMove = strategy(round, lastAiMove, lastOutcome);
                const Move aiMove = ai.predict();
                const Outcome outcome = resolve(playerMove, aiMove);

                if (outcome == Outcome::Win) win++;
                else if (outcome == Outcome::Loss) loss++;
                else draw++;

                ai.learn(aiMove, playerMove);
                ai.update(playerMove, aiMove);

                lastAiMove = aiMove;
                lastOutcome = outcome;
            }
        }
        std::printf("  przebieg %d/%d gotowy\n", pass + 1, passes);
    }

    ai.saveToFile(path);

    const double total = static_cast<double>(blockRounds) * training.size() * passes;
    std::printf("\nZiarno zapisane do: %s\n", path.c_str());
    std::printf("Rund treningowych: %.0f  (na mieszance %zu strategii x %d przebiegow)\n",
                total, training.size(), passes);
    std::printf("Wynik w trakcie treningu: gracz=%.1f%%  AI=%.1f%%  remisy=%.1f%%\n",
                100.0 * win / total, 100.0 * loss / total, 100.0 * draw / total);
    //std::printf("\nUWAGA: te liczby to wynik NA DANYCH TRENINGOWYCH - nie mowia nic o jakosci ziarna.\n");
    // if (holdout) {
    //     std::printf("Tryb --holdout: held-out sa STRATEGICZNY, STRATEGICZNY-LOSOWY, ANTY-AI i DRUGIE DNO.\n");
    // } else {
    //     std::printf("Tryb pelny: jedynym uczciwym held-out jest DRUGIE DNO (nie ma go w zadnej mieszance).\n");
    // }
    // std::printf("Porownanie:\n"
    //             "  ./bin/benchmark --repeats 20                     (start losowy)\n"
    //             "  ./bin/benchmark --repeats 20 --seed %s (start z ziarna)\n", path.c_str());
}

int main(int argc, char** argv) {
    // ./benchmark                         -> pojedynczy przebieg z rozbiciem na okna czasowe
    // ./benchmark --repeats 20            -> 20 przebiegow, tylko usrednione podsumowania
    // ./benchmark --seed data/seed.txt    -> kazdy scenariusz startuje z wytrenowanego ziarna
    // ./benchmark --train-seed [sciezka]  -> trenuje ziarno na PELNEJ mieszance (pod realna gre)
    // ./benchmark --train-seed p --holdout -> trenuje bez przeciwnikow held-out (pod uczciwa ocene)
    int repeats = 1;
    std::string seedPath;
    std::string trainPath;
    bool doTrain = false;
    bool holdout = false;

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--repeats" && i + 1 < argc) {
            repeats = std::max(1, std::atoi(argv[++i]));
        } else if (arg == "--seed" && i + 1 < argc) {
            seedPath = argv[++i];
        } else if (arg == "--train-seed") {
            doTrain = true;
            if (i + 1 < argc && argv[i + 1][0] != '-') trainPath = argv[++i];
        } else if (arg == "--holdout") {
            holdout = true;
        }
    }

    if (doTrain) {
        if (trainPath.empty()) trainPath = "data/seed.txt";
        // std::printf("Trening ziarna -> %s  (%s)\n", trainPath.c_str(),
        //             holdout ? "mieszanka BEZ przeciwnikow held-out" : "PELNA mieszanka");
        trainSeed(trainPath, /*blockRounds=*/300, /*passes=*/4, holdout);
        return 0;
    }

    runScenario("Staly ruch (Kamien)", [] { return constantStrategy(Move::Rock); }, 200, 25, repeats, seedPath);
    runScenario("Cykl Kamien->Papier->Nozyce", [] { return cycleStrategy(); }, 300, 30, repeats, seedPath);
    runScenario("Win-stay / lose-shift", [] { return winStayLoseShiftStrategy(); }, 300, 30, repeats, seedPath);
    runScenario("20 rund stale + 20 rund cyklu (na przemian)", [] { return constantThenCycleStrategy(); }, 360, 20, repeats, seedPath);
    runScenario("Rotacja stalych ruchow co 15 rund", [] { return rotatingConstantStrategy(); }, 360, 15, repeats, seedPath);
    runScenario("Ludzka losowosc (unikanie powtorzen)", [] { return humanRandomStrategy(); }, 300, 30, repeats, seedPath);
    std::cout << "\n--- ponizej: HELD-OUT (wylaczone z treningu ziarna) ---\n";
    runScenario("STRATEGICZNY: krotkie fazy po 12 rund", [] { return strategicMixStrategy(); }, 120, 12, repeats, seedPath);
    runScenario("STRATEGICZNY-LOSOWY: nowy wzorzec co ~12 rund", [] { return randomPhaseStrategy(); }, 120, 12, repeats, seedPath);
    runScenario("ANTY-AI: gracz kontruje bota", [] { return antiAiStrategy(); }, 120, 12, repeats, seedPath);
    runScenario("DRUGIE DNO: gracz wyprzedza kontre AI", [] { return secondGuessStrategy(); }, 120, 12, repeats, seedPath);
    runScenario("Prawdziwie losowe (kontrolne, sufit ~33%)", [] { return uniformRandomStrategy(); }, 300, 30, repeats, seedPath);
    return 0;
}
