#ifndef STATE_HPP
#define STATE_HPP

#include "Action.hpp"
#include "GameState.hpp"

#include <functional>

struct State {
    using eval_t = double;

    static State getInitialState(const GameState& game);
    Action getRandomAction();

    eval_t eval();
    hash_t getHash();
    std::optional<State> succ(const Actions& actions) const;
    bool canSurvive() const;

    static eval_t defaultEval(const State& state);
    bool operator>(const State& other) const;

    static std::function<eval_t(const State&)> evalFunction;

    GameState game;
    Explosions explosions;
    Action firstAction;
    eval_t score = 0.0;
    hash_t hash = 0;
    int myRange = 0, myBombs = 0;
    int boxAdd = 0, boxSum = 0;
    bool firstLayer = true;
};

#endif /* STATE_HPP */
