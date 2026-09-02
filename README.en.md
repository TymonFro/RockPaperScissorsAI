[English](README.en.md) | [Polski](README.md)

# RPS — rock, paper, scissors with a learning AI

A game where you play against two opponents at once: a **random** one (the baseline) and an
**AI** that learns your habits and tries to exploit them. UI built with Dear ImGui + SFML.

The neural network is written from scratch in C++ (no ML libraries) — dense layers, ReLU,
backpropagation, the Adam optimizer, and online learning after every round.

Note: the application's interface is currently in Polish.

![alt text](assets/image.png)

<!-- ## How the AI works

The move is chosen by an **ensemble of predictors**, not by the network alone:

- **the neural network** — the input is a one-hot encoding of the last few rounds (player's move
  + AI's move), the output is a distribution over the player's next move; the AI's move is picked
  by expected value,
- **count tables** — frequency, Markov of order 1 and 2, plus one conditioned on the previous
  round's outcome (which captures the "switch after a loss" reflex); these learn within ~30 rounds,
  so much faster than the network,
- **a random predictor** with its score pinned to zero — a safety net: when every pattern-based
  predictor fails (a human is reading the bot), the AI falls back to random play, which makes it
  impossible to push it below ~33%.

Every round each predictor gets a **decaying form score** (win +1, loss −1, draw 0, multiplied
by 0.9), and the one currently on top gets to play.

User profiles are stored in `users/` (network weights + tables), and a new profile starts from
a pre-trained seed, `data/seed.txt`, so it doesn't begin from scratch. -->

## Requirements

```bash
sudo apt install build-essential pkg-config libsfml-dev
sudo apt install g++-mingw-w64-x86-64      # only for the Windows release
```

## Commands

```bash
make               # build the game -> bin/rps
make run           # build (generating the seed if needed) and run
make clean         # remove build artifacts

make seed          # train the seed used by new profiles -> data/seed.txt
make seed-eval     # seed trained without the held-out opponents (for fair comparisons only)

make benchmark     # build the testing tool -> bin/benchmark
make windows       # cross-compile a static .exe -> precompiled/windows/
make linux-dist    # Linux release -> precompiled/linux/
make precompiled   # both releases at once
```

**Important**: after changing the network architecture (layer sizes, `histryWindow`, `encodeSize`),
delete the old profiles from `users/` and regenerate the seed (`make seed`) — the save format has
no version header, so old files will be read as garbage.

## Benchmark

Plays the AI against scripted player profiles (constant move, cycles, win-stay/lose-shift,
human "randomness", a player switching tactics every dozen or so rounds, a player countering
the bot) and reports win percentages. Faster and more repeatable than clicking through the UI.

```bash
./bin/benchmark                              # single run, broken down into time windows
./bin/benchmark --repeats 20                 # 20 runs, averaged summaries
./bin/benchmark --repeats 20 --seed data/seed.txt   # start from the seed instead of a random net
./bin/benchmark --repeats 20 --seed users/tymo.txt  # evaluate a specific profile

./bin/benchmark --train-seed data/seed.txt            # train the seed (full opponent mix)
./bin/benchmark --train-seed X.txt --holdout          # without the held-out opponents
```

The benchmark never modifies the file you point it at, and never touches `users/`.

## Layout

```
src/game/    game rules, random opponent, statistics
src/ai/      network (Network.h), tables (CountTable.h), predictor ensemble (NeuralOpponent)
src/ui/      window, layout, click handling (App)
tools/       benchmark + the Windows cross-compilation script
third_party/ Dear ImGui and ImGui-SFML (untouched)
```
