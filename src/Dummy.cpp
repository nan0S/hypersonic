#include "Dummy.hpp"

Dummy::Dummy() {
    name = "Dummy";
}

Action Dummy::getAction(const GameState& game, const int) {
    auto me = game.getPlayer(myid);
    assert(me);
    if (firstAction) {
        firstAction = false;
        return {ActionType::bomb, me->x, me->y};
    }
    return {ActionType::move, me->x, me->y};
}