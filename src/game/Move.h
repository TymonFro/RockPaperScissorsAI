#pragma once
#include <array>

enum class Move { Rock, Paper, Scissors };
enum class Outcome { Win, Loss, Draw };

inline constexpr std::array<Move, 3> kAllMoves = {Move::Rock, Move::Paper, Move::Scissors};

constexpr const char* moveName(Move m) {
    switch (m) {
        case Move::Rock: return "Kamien";
        case Move::Paper: return "Papier";
        case Move::Scissors: return "Nozyce";
    }
    return "?";
}

// Wynik z perspektywy gracza 'a' grajacego przeciwko 'b'.
constexpr Outcome resolve(Move a, Move b) {
    if (a == b) return Outcome::Draw;
    const bool aWins = (a == Move::Rock && b == Move::Scissors) ||
                        (a == Move::Paper && b == Move::Rock) ||
                        (a == Move::Scissors && b == Move::Paper);
    return aWins ? Outcome::Win : Outcome::Loss;
}

constexpr const char* outcomeName(Outcome o) {
    switch (o) {
        case Outcome::Win: return "Wygrana";
        case Outcome::Loss: return "Przegrana";
        case Outcome::Draw: return "Remis";
    }
    return "?";
}
