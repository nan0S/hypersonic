#include "BeamSearch.hpp"

#include <cassert>
#include <chrono>
#include <vector>
#include <algorithm>

int BeamSearch::totalBeamLengthSum = 0;

BeamSearch::BeamSearch() {
    name = "BeamSearch";
}

Action BeamSearch::getAction(const GameState& game, const int timeLimit) {
    Timer timer(timeLimit);

    State::evalFunction = eval;

    auto possibleActions = game.getPossibleActions();
    auto me = game.getPlayer(myid);
    assert(me);
    Action action = {ActionType::move, me->x, me->y};

    static constexpr int BEAM_SIZE = GameState::H * GameState::W * LOCAL_BEAM_WIDTH;
    static constexpr int MAX_BEAM_SIZE =  BEAM_SIZE < BEAM_WIDTH ? BEAM_WIDTH : BEAM_SIZE;

    static std::array<State, MAX_BEAM_SIZE> beam;
    std::vector<State> localBeams[game.H][game.W];
    beam[0] = State::getInitialState(game);
    int currentBeam = 1;

    /* logging */
    int totalBeamLength = currentBeam;
    int notNextReason = 0;
    int survivalReason = 0;
    int visitedReason = 0;
    int depth = 0;

    // for (depth = 0; depth < BEAM_DEPTH; ++depth) {
    for (depth = 0; timer.isTimeLeft(); ++depth) {
        std::set<hash_t> visited;

        for (int k = 0; k < currentBeam; ++k) {
            const auto& state = beam[k];

            for (int i = 0; i < game.DIR_COUNT; ++i)
                for (const auto& actionType : game.actionTypes) {
                    auto currentMe = state.game.getPlayer(myid);
                    assert(currentMe && currentMe->id == myid);
                    Action currentAction = { actionType,
                        currentMe->x + game.dx[i],currentMe->y + game.dy[i]};

                    if (depth == 0 && !possibleActions.count(currentAction))
                        continue;

                    auto next = state.succ({{myid, currentAction}});
                    if (!next) {
                        ++notNextReason;
                        continue;
                    }

                    const auto& nextState = next.value();
                    if (visited.count(nextState.hash)) {
                        ++visitedReason;
                        continue;
                    }
                    visited.insert(nextState.hash);

                    if (!nextState.game.canSurvive(nextState.explosions)) {
                        ++survivalReason;
                        continue;
                    }

                    auto nextMe = nextState.game.getPlayer(myid);
                    assert(nextMe && nextMe->id == myid);
                    localBeams[nextMe->y][nextMe->x].push_back(nextState);
                }

            if (!timer.isTimeLeft())
                break;
        }

        if (!timer.isTimeLeft())
            break;

        assert(currentBeam <= MAX_BEAM_SIZE);

        currentBeam = 0;
        for (int y = 0; y < height; ++y)
            for (int x = 0; x < width; ++x) {
                int localBeamLength = localBeams[y][x].size();
                int beamToSort = std::min(localBeamLength, LOCAL_BEAM_WIDTH);
                std::partial_sort(localBeams[y][x].data(),
                    localBeams[y][x].data() + beamToSort,
                    localBeams[y][x].data() + localBeamLength,
                    std::greater<State>());

                std::move(localBeams[y][x].data(),
                    localBeams[y][x].data() + beamToSort,
                    beam.data() + currentBeam);
                currentBeam += beamToSort;
                localBeams[y][x].clear();
            }

        totalBeamLength += currentBeam;

        int beamToSort = std::min(currentBeam, BEAM_WIDTH);
        std::partial_sort(beam.data(),
            beam.data() + beamToSort,
            beam.data() + currentBeam,
            std::greater<State>());
        currentBeam = beamToSort;

        if (currentBeam > 0)
            action = beam[0].firstAction;
    }

    totalBeamLengthSum += totalBeamLength;

    if (verbose) {
        std::cerr << "totalBeamLength: " << totalBeamLength << std::endl;
        std::cerr << "notNextReason: " << notNextReason << std::endl;
        std::cerr << "visitedReason: " << visitedReason << std::endl;
        std::cerr << "survivalReason: " << survivalReason << std::endl;
        std::cerr << "totalBeamLengthSum: " << totalBeamLengthSum << std::endl;
        std::cerr << "beam depth: " << depth << std::endl;

        if (!timer.isTimeLeft())
            std::cerr << "No time left!" << std::endl;
        else
            std::cerr << "Just ended!" << std::endl;
    }

    return action;
}

BeamSearch::eval_t BeamSearch::eval(const State& s) {
    eval_t points = 0.0;

    const eval_t boxReward = 10;
    points += boxReward * s.boxAdd;
    points += s.boxSum * 1;

    auto me = s.game.getPlayer(s.game.myid);
    assert(me);

    points += 0.65 * std::min(5, s.myRange) + 0.5 * s.myRange;
    points += 3.2 * std::min(s.myBombs, 3) + 1.9 * s.myBombs;
    points += 0.2 * (s.myBombs - me->bombs);

    for (int y = 0; y < s.game.H; ++y)
        for (int x = 0; x < s.game.W; ++x)
            if (s.game.board.isBox(x, y) && s.explosions.isOwner(x, y, s.game.myid))
                points += 1 * (boxReward - s.explosions.e[y][x].time);
            
    eval_t sumDists = 0;
    for (int y = 0; y < s.game.H; ++y)
        for (int x = 0; x < s.game.W; ++x)
            if (s.game.board.isBox(x, y))
                sumDists += std::abs(me->x - x) + std::abs(me->y - y);
    points -= 0.1 * sumDists;
    
    points -= 0.04 * std::abs(me->y - s.game.H * 0.5);
    points -= 0.04 * std::abs(me->x - s.game.W * 0.5);

    return points;
}
