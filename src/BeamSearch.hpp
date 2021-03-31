#ifndef BEAMSEARCH_HPP
#define BEAMSEARCH_HPP

#include "Agent.hpp"
#include "Action.hpp"
#include "GameState.hpp"
#include "Common.hpp"
#include "State.hpp"

class BeamSearch : public Agent {
public:
    using eval_t = State::eval_t;

    BeamSearch();

    Action getAction(const GameState& game,
                     const int timeLimit) override;

    static eval_t eval(const State& state);

private:
    static constexpr int BEAM_WIDTH = 200;
    static constexpr int LOCAL_BEAM_WIDTH = 20;
    static constexpr int BEAM_DEPTH = 15;

    static int totalBeamLengthSum;
};
    
#endif /* BEAMSEARCH_HPP */
