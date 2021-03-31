#include "Agent.hpp"

void Agent::init(int height, int width, int myid, bool verbose) {
    this->height = height;
    this->width = width;
    this->myid = myid;
    this->verbose = verbose;
}

Action Agent::getAction(const GameState& game, const int) {
    auto me = game.getPlayer(myid);
    assert(me);

    Action action;
    action.type = ActionType::move;
    action.x = me->x;
    action.y = me->y;

    return action;
}
