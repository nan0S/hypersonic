
#include "GameState.hpp"

#include <cassert>
#include <cstring>
#include <map>
#include <set>
#include <functional>
#include <algorithm>
#include <tuple>
#undef all
#include <bitset>
#define all(x) (x).begin(), (x).end()

int GameState::myid;

std::istream& operator>>(std::istream& in, GameState& game) {
    for (int y = 0; y < game.H; ++y)
        for (int x = 0; x < game.W; ++x) {
            char c;
            in >> c;
            assert(c == '.' || c == 'X' || std::isdigit(c));
            game.board.board[y][x] =
                c == '.' ? Cell::empty :
                c == 'X' ? Cell::wall :
                Cell(c - '0');
        }

    int entityCount;
    in >> entityCount;
    for (int i = 0; i < entityCount; ++i) {
        int entityType, owner;
        int x, y;
        int param1, param2;
        in >> entityType >> owner;
        in >> x >> y;
        in >> param1 >> param2;
        switch (entityType) {
            case 0:
                game.players.push_back({owner, x, y, param1, param2});
                break;
            case 1:
                game.bombs.push_back({owner, x, y, param1, param2});
                break;
            case 2:
                game.items.push_back({x, y, ItemType(param1)});
                break;
            default:
                assert(false);
        }
    }

    return in;
}

void Explosions::explode(int x, int y, int time, int ownerId) {
    if (e[y][x].time < time)
        return;
    e[y][x].time = time;
    assert(0 <= ownerId && ownerId < 4);
    e[y][x].ownerMask |= 1 << ownerId;
}

bool Explosions::isOwner(int x, int y, int id) const {
    return e[y][x].time != ExplosionInfo::NONE && (e[y][x].ownerMask & 1 << id);
}

Explosions GameState::getExplosions() const {
    std::map<Point, Bomb> bombsAt;
    std::array<std::vector<Point>, MAX_BOMB_TIME> explodeAtTime;
    for (const auto& bomb : bombs) {
        auto bPoint = point(bomb);
        bombsAt[bPoint] = bomb;
        explodeAtTime[bomb.time - 1].push_back(bPoint);
    }

    std::map<Point, Item> itemsAt;
    for (const auto& item : items)
        itemsAt[point(item)] = item;

    Board boardC = this->board;
    assert(&boardC != &this->board);
    Explosions explosions;
    for (int y = 0; y < H; ++y)
        for (int x = 0; x < W; ++x) {
            assert(explosions.e[y][x].time == ExplosionInfo::NONE);
            assert(explosions.e[y][x].ownerMask == 0);
        }

    std::set<Point> visited;
    std::function<void(const Bomb&, int)> bombFloodFill = [&](
        const Bomb& bomb, int time) {

        if (visited.count(point(bomb)))
            return;

        visited.insert(point(bomb));
        explosions.explode(bomb.x, bomb.y, time, bomb.owner);

        for (int i = 0; i < DIR_COUNT - 1; ++i)
            for (int l = 1; l < bomb.range; ++l) {
                int nx = bomb.x + l * dx[i];
                int ny = bomb.y + l * dy[i];
                if (!isInBounds(nx, ny))
                    continue;
                if (boardC.board[ny][nx] == Cell::wall)
                    break;

                explosions.explode(nx, ny, time, bomb.owner);
                auto nPoint = point(nx, ny);

                bool stopped = false;
                if (bombsAt.count(nPoint)) {
                    assert(!itemsAt.count(nPoint));
                    stopped = true;
                    bombFloodFill(bombsAt[nPoint], time);
                }
                else if (itemsAt.count(nPoint)) {
                    stopped = true;
                    assert(point(itemsAt[nPoint]) == nPoint);
                    visited.insert(nPoint);
                }

                if (boardC.board[ny][nx] != Cell::empty)
                    stopped = true;
                if (boardC.isBox(nx, ny))
                    visited.insert(nPoint);
                if (stopped)
                    break;
            }
    };

    for (int t = 0; t < MAX_BOMB_TIME; ++t) {
        visited.clear();
        for (const auto& p : explodeAtTime[t])
            if (bombsAt.count(p))
                bombFloodFill(bombsAt[p], t + 1);
        for (const auto& p : visited) {
            bombsAt.erase(p);
            itemsAt.erase(p);
            boardC.board[p.y][p.x] = Cell::empty;
        }
    }

    return explosions;
}

const Player* GameState::getPlayer(int id) const {
    const auto it = std::find_if(all(players), [id](const Player& player) { 
        return player.id == id; });
    if (it == players.end())
        return nullptr;
    return &(*it);
}

int GameState::countAllBombs(int id) const {
    assert(getPlayer(id));
    return getPlayer(id)->bombs +
        std::count_if(all(bombs), [id](const Bomb& bomb) {
            return bomb.owner == id; });
}

/* get the next state, if actions are invalid or you die returns {} (nothing) */
std::optional<std::pair<GameState, ExtraInfo>> GameState::succ(
        const Explosions& explosions,
        const Actions& actions) const {
    if (!validActions(actions))
        return {};

    GameState next;
    next.board = board;
    assert(next.players.empty() &&
        next.bombs.empty() &&
        next.items.empty());
    ExtraInfo extra;

    /* calculate items created by box destruction */
    std::map<Point, Item> allItems;
    for (int y = 0; y < H; ++y)
        for (int x = 0; x < W; ++x)
            if (explosions.e[y][x].time == 1 && board.isBox(x, y)) {
                next.board.board[y][x] = Cell::empty;
                if (board.isDrop(x, y))
                    allItems[point(x, y)] = board.drop(x, y);
                for (int i = 0; i < MAX_PLAYERS; ++i)
                    if (explosions.e[y][x].ownerMask & 1 << i)
                        ++extra.boxesDestroyed[i];
            }

    /* calculate survived players */
    std::map<int, Player> playersAlive;
    for (const auto& player : this->players)
        if (explosions.e[player.y][player.x].time == 1) {
            if (player.id == myid)
                return {};
        }
        else
            playersAlive[player.id] = player;

    /* calculate survived items */
    for (const auto& item : this->items) {
        assert(explosions.e[item.y][item.x].time >= 1);
        if (explosions.e[item.y][item.x].time > 1)
            allItems[point(item)] = item;
    }

    /* calculate survived bombs */
    int bombsExploded[MAX_PLAYERS] = {};
    for (int i = 0; i < MAX_PLAYERS; ++i)
        assert(bombsExploded[i] == 0);

    for (auto bomb : this->bombs)
        if (explosions.e[bomb.y][bomb.x].time == 1)
            ++bombsExploded[bomb.owner];
        else {
            --bomb.time;
            next.bombs.push_back(bomb);
        }

    /* calculate actions influence */
    std::set<Point> playerAt;
    for (auto [id, player] : playersAlive) {
        assert(id == player.id);
        if (actions.count(id)) {
            const auto& action = actions.at(id);
            if (action.type == ActionType::bomb)
                next.bombs.push_back(player.placeBomb());

            if (point(action) != point(player)) {
                assert(std::abs(action.x - player.x) + std::abs(action.y - player.y) == 1);
                player.x = action.x;
                player.y = action.y;

                auto p = point(player);
                if (allItems.count(p))
                    switch (allItems[p].type) {
                        case ItemType::bomb:
                            ++extra.bombsIncrease[id];
                            ++player.bombs;
                            break;
                        case ItemType::range:
                            ++player.range;
                            ++extra.rangesIncrease[id];
                            break;
                        default:
                            assert(false);
                    }
            }
        }

        player.bombs += bombsExploded[player.id];
        playerAt.insert(point(player));
        next.players.push_back(player);
    }

    for (const auto& [p, item] : allItems)
        if (!playerAt.count(p))
            next.items.push_back(item);

    return std::make_pair(next, extra);
}

/* get technically valid actions - you may die, but technically you can go there */
bool GameState::validActions(const Actions& actions) const {
    std::set<Point> bombsAt;
    for (const auto& bomb : bombs)
        bombsAt.insert(point(bomb));

    for (const auto& player : players)
        if (actions.count(player.id)) {
            const auto& action = actions.at(player.id);
            if (std::abs(action.x - player.x) + std::abs(action.y - player.y) >= 2)
                return false;
            if (!isInBounds(action.x, action.y))
                return false;
            auto aPoint = point(action), pPoint = point(player);
            if (aPoint != pPoint) {
                if (board.board[action.y][action.x] != Cell::empty)
                    return false;
                if (bombsAt.count(aPoint))
                    return false;
            }
            if (action.type == ActionType::bomb) {
                assert(player.bombs >= 0);
                if (player.bombs == 0)
                    return false;
                if (bombsAt.count(pPoint))
                    return false;
            }
        }

    return true;
}

/* get all valid actions in the current state (your actions) */
std::vector<Action> GameState::getActions() const {
    std::vector<Action> actions;
    actions.reserve(MAX_ACTIONS);

    auto me = getPlayer(myid);
    int posX = me->x, posY = me->y;

    for(int i = 0; i < DIR_COUNT; i++)
        for (const auto& actionType : actionTypes) {
            Action action = {actionType, posX + dx[i], posY + dy[i]};
            if(validActions({{myid, action}}))
            {
                actions.push_back(action);
            }
        }

    return actions;
}

/* check if you can survive assuming you just done your move in 
 * the current round */
bool GameState::canSurvive(const Explosions& explosions) const {
    auto me = getPlayer(myid);
    if (!me)
        return false;
    if (explosions.e[me->y][me->x].time == 1)
        return false;

    if (explosions.e[me->y][me->x].time == ExplosionInfo::NONE)
        return true;

    int maxBombTime = 0;
    std::set<Point> bombAt;
    for (const auto& bomb : bombs) {
        if (bomb.time > maxBombTime)
            maxBombTime = bomb.time;
        bombAt.insert(point(bomb));
    }
    assert(maxBombTime <= MAX_BOMB_TIME);

    static std::bitset<H * W> dp, next;
    dp.reset();
    next.reset();
    dp[me->y * W + me->x] = true;

    for (int t = 0; t < maxBombTime; ++t) {
        bool survive = false;

        for (int y = 0; y < H; ++y)
            for (int x = 0; x < W; ++x) {
                if (!dp[y * W + x])
                    continue;
                if (explosions.e[y][x].time == t + 1)
                    continue;
                if (explosions.e[y][x].time == ExplosionInfo::NONE)
                    return true;
                for (int i = 0; i < DIR_COUNT; ++i) {
                    int nx = x + dx[i];
                    int ny = y + dy[i];
                    if (!isInBounds(nx, ny))
                        continue;
                    if (next[ny * W + nx])
                        continue;
                    if (board.board[ny][nx] == Cell::wall)
                        continue;
                    if (board.isBox(nx, ny) && explosions.e[ny][nx].time >= t + 1)
                        continue;
                    if (point(nx, ny) != point(x, y) && bombAt.count(point(nx, ny)) && explosions.e[ny][nx].time >= t + 1)
                        continue;
                    next[ny * W + nx] = true;
                    survive = true;
                }
            }

        if (!survive)
            return false;

        dp = next;
        next.reset();
    }

    return true;
}

/* get possible actions, assuming you do not want to die
 * and all the other players will place the bomb where
 * they stand right now, if all actions with pessimistic
 * assumption are forbidden, return without that assumption,
 * if still all actions are forbidden, return all actions */
std::set<Action> GameState::_getPossibleActions(Actions actions) const {
    std::set<Action> possibleActions;
    auto explosions = getExplosions();
    auto me = getPlayer(myid);
    assert(me);

    for (int i = 0; i < DIR_COUNT; ++i)
        for (const auto& actionType : actionTypes) {
            Action myAction = {actionType, me->x + dx[i], me->y + dy[i]};
            actions[myid] = myAction;
            auto succInfo = succ(explosions, actions);
            if (!succInfo)
                continue;
            const auto& [nextState, extra] = succInfo.value();
            assert(nextState.getPlayer(myid));
            if (nextState.canSurvive(nextState.getExplosions()))
                possibleActions.insert(myAction);
        }

    return possibleActions;
}

std::set<Action> GameState::getPossibleActions() const {
    std::set<Point> bombsAt;
    for (const auto& bomb : bombs)
        bombsAt.insert(point(bomb));

    Actions enemyActions;
    const Player* me = nullptr;
    for (const auto& player : players)
        if (player.id != myid) {
            assert(player.bombs >= 0);
            if (player.bombs == 0 || bombsAt.count(point(player)))
                continue;
            enemyActions[player.id] = { ActionType::bomb, player.x, player.y };
        }
        else
            me = &player;
    assert(me);

    auto possibleActions = _getPossibleActions(enemyActions);
    if (possibleActions.empty()) {
        possibleActions = _getPossibleActions({});

        if (possibleActions.empty())
            for (int i = 0; i < DIR_COUNT; ++i)
                for (const auto& actionType : actionTypes)
                    possibleActions.insert({actionType, me->x + dx[i], me->y + dy[i]});
    }
    assert(!possibleActions.empty());

    return possibleActions;
}

Bomb Player::placeBomb() {
    assert(bombs > 0);
    --bombs;
    return {id, x, y, GameState::MAX_BOMB_TIME, range};
}

std::ostream& operator<<(std::ostream& out, const Bomb& bomb) {
    return out << "owner: " << bomb.owner
               << ", (x: " << bomb.x << ", y: " <<  bomb.y
               << "), time: " << bomb.time << ", range: " << bomb.range; 
}

std::ostream& operator<<(std::ostream& out, const Player& player) {
    return out << "id: " << player.id
               << ", (x: " << player.x << ", y: " <<  player.y
               << "), bombs: " << player.bombs << ", range: " << player.range; 
}

std::ostream& operator<<(std::ostream& out, const Item& item) {
    return out << "type: " << (item.type == ItemType::range ? "RANGE" : "BOMB")
               << ", (x: " << item.x << ", y: " <<  item.y;
}
