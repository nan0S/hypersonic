#include "State.hpp"

std::function<State::eval_t(const State&)> State::evalFunction = State::defaultEval;

State State::getInitialState(const GameState& game) {
    State initialState;

    initialState.game = game;
    initialState.explosions = game.getExplosions();
    assert(game.getPlayer(game.myid));
    initialState.myRange = game.getPlayer(game.myid)->range;
    initialState.myBombs = game.countAllBombs(game.myid);
    initialState.boxAdd = 0;
    initialState.boxSum = 0;
    initialState.firstLayer = true;
    initialState.score = State::evalFunction(initialState);
    initialState.hash = initialState.getHash();

    return initialState;
}

Action State::getRandomAction() {
    auto me = game.getPlayer(game.myid);
    assert(me);
    int dir = Random::rand(game.DIR_COUNT);
    return {
        game.actionTypes[Random::rand(game.MAX_ACTION_TYPES)],
        me->x + game.dx[dir], me->y + game.dy[dir]
    };
}

State::eval_t State::eval() {
    return evalFunction(*this);
}

hash_t State::getHash() {
    hash_t power = 1e9 + 123;
    hash_t hash = 0;
    auto addHashElem = [power](hash_t& h, hash_t x) {
        h = h * power + x;
    };

    addHashElem(hash, firstLayer);
    addHashElem(hash, boxAdd);
    addHashElem(hash, myRange);
    addHashElem(hash, myBombs);
    addHashElem(hash, boxSum);

    for (int y = 0; y < game.H; ++y)
        for (int x = 0; x < game.W; ++x)
            addHashElem(hash, explosions.e[y][x].time);

    hash_t playersHash = 0;
    for (const auto& player : game.players) {
        hash_t pHash = 0;
        addHashElem(pHash, player.id);
        addHashElem(pHash, player.x);
        addHashElem(pHash, player.y);
        addHashElem(pHash, player.bombs);
        addHashElem(pHash, player.range);
        playersHash ^= pHash;
    }
    addHashElem(hash, playersHash);

    hash_t bombsHash = 0;
    for (const auto& bomb : game.bombs) {
        hash_t bHash = 0;
        addHashElem(bHash, bomb.owner);
        addHashElem(bHash, bomb.x);
        addHashElem(bHash, bomb.y);
        addHashElem(bHash, bomb.time);
        addHashElem(bHash, bomb.range);
        bombsHash ^= bHash;
    }
    addHashElem(hash, bombsHash);

    hash_t itemsHash = 0;
    for (const auto& item : game.items) {
        hash_t iHash = 0;
        addHashElem(iHash, item.x);
        addHashElem(iHash, item.y);
        addHashElem(iHash, hash_t(item.type));
        itemsHash ^= iHash;
    }
    addHashElem(hash, itemsHash);

    return hash;
}

std::optional<State> State::succ(const Actions& actions) const {
    auto succInfo = game.succ(explosions, actions);
    if (!succInfo)
        return {};

    const auto& [nextGame, extra] = succInfo.value();

    State nextState;
    nextState.game = nextGame;
    nextState.explosions = nextState.game.getExplosions();

    assert(actions.count(game.myid));
    if (firstLayer)
        nextState.firstAction = actions.at(game.myid);
    else
        nextState.firstAction = firstAction;

    nextState.myRange = myRange + extra.rangesIncrease[game.myid];
    assert(myRange <= nextState.myRange && nextState.myRange <= myRange + 1);
    nextState.myBombs = myBombs + extra.bombsIncrease[game.myid];
    nextState.boxAdd = boxAdd + extra.boxesDestroyed[game.myid];
    nextState.boxSum = boxSum + boxAdd;
    nextState.firstLayer = false;

    nextState.score = State::evalFunction(nextState);
    nextState.hash = nextState.getHash();

    return nextState;
}

bool State::canSurvive() const {
    return game.canSurvive(explosions);
}

State::eval_t State::defaultEval(const State& s) {
    eval_t points = 0.0;

    const eval_t boxReward = 10;
    points += boxReward * s.boxAdd;
    points += s.boxSum;

    auto me = s.game.getPlayer(s.game.myid);
    assert(me);

    points += 0.9 * std::min(5, s.myRange) + 0.4 * s.myRange;
    points += 3.4 * std::min(2, s.myBombs) + 1.7 * std::min(4, s.myBombs) + 0.7 * s.myBombs;
    points -= 0.17 * (s.myBombs - me->bombs);

    for (int y = 0; y < s.game.H; ++y)
        for (int x = 0; x < s.game.W; ++x)
            if (s.game.board.isBox(x, y) && s.explosions.isOwner(x, y, s.game.myid))
                points += boxReward - s.explosions.e[y][x].time;
            
    points -= 0.04 * std::abs(me->y - s.game.H * 0.5);
    points -= 0.04 * std::abs(me->x - s.game.W * 0.5);

    return points;
}

bool State::operator>(const State& other) const {
    return score > other.score;
}
