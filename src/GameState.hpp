#ifndef GAME_STATE_HPP
#define GAME_STATE_HPP

#include "Common.hpp"
#include "Action.hpp"

#include <iostream>
#include <vector>
#include <cassert>
#include <climits>
#include <set>

enum class Cell {
    wall = -2,
    empty = -1,
    box = 0,
    rangeBox = 1,
    bombBox = 2
};

enum class ItemType : int {
    range = 1,
    bomb = 2
};

struct Bomb {
    int owner;
    int x, y;
    int time, range;

    friend std::ostream& operator<<(std::ostream& out, const Bomb& bomb);
};

struct Player {
    int id;
    int x, y;
    int bombs, range;

    Bomb placeBomb();

    friend std::ostream& operator<<(std::ostream& out, const Player& player);
};

struct Item {
    int x, y;
    ItemType type;

    friend std::ostream& operator<<(std::ostream& out, const Item& item);
};

struct ExplosionInfo {
    static constexpr char NONE = CHAR_MAX;
    char time = NONE;
    char ownerMask = 0;
};

struct Explosions {
    static constexpr int H = 11;
    static constexpr int W = 13;

    ExplosionInfo e[H][W];

    void explode(int x, int y, int time, int ownerId);
    bool isOwner(int x, int y, int id) const;
};

struct Board {
    static constexpr int H = 11;
    static constexpr int W = 13;
    Cell board[H][W];

    inline bool isBox(int x, int y) const;
    inline bool isDrop(int x, int y) const;
    inline Item drop(int x, int y) const;
};

bool Board::isBox(int x, int y) const {
    return board[y][x] != Cell::wall && board[y][x] != Cell::empty;
}

bool Board::isDrop(int x, int y) const {
    return board[y][x] == Cell::rangeBox || board[y][x] == Cell::bombBox;
}

Item Board::drop(int x, int y) const {
    return {x, y, board[y][x] == Cell::rangeBox ? ItemType::range : ItemType::bomb};
}

struct ExtraInfo {
    static constexpr int MAX_PLAYERS = 4;

    /* should work with such small types */
    char boxesDestroyed[MAX_PLAYERS] = {}; /* zero initialization */
    char rangesIncrease[MAX_PLAYERS] = {}; /* zero initialization */
    char bombsIncrease[MAX_PLAYERS] = {}; /* zero initialization */
};

struct GameState {
    static constexpr int H = 11;
    static constexpr int W = 13;
    static constexpr int DIR_COUNT = 5;
    static constexpr int dx[DIR_COUNT] = {-1, 1, 0, 0, 0};
    static constexpr int dy[DIR_COUNT] = {0, 0, 1, -1, 0};
    static constexpr int MAX_ACTIONS = 10;
    static constexpr int MAX_ACTION_TYPES = 2;
    static constexpr ActionType actionTypes[MAX_ACTION_TYPES] = {
        ActionType::bomb,
        ActionType::move
    };
    static constexpr int MAX_PLAYERS = 4;
    static constexpr int MAX_BOMB_TIME = 8;
    static int myid;

    Board board;
    std::vector<Player> players;
    std::vector<Bomb> bombs;
    std::vector<Item> items;

    Explosions getExplosions() const;
    const Player* getPlayer(int id) const;
    int countAllBombs(int id) const;
    std::optional<std::pair<GameState, ExtraInfo>> succ(const Explosions& explosions,
        const Actions& actions) const;
    bool validActions(const Actions& actions) const;
    bool canSurvive(const Explosions& explosions) const;
    std::vector<Action> getActions() const;
    std::set<Action> getPossibleActions() const;    
    std::set<Action> _getPossibleActions(Actions actions) const;

    static inline bool isInBounds(int x, int y);

    friend std::istream& operator>>(std::istream& in, GameState& gameState);
};

bool GameState::isInBounds(int x, int y) {
    return 0 <= x && x < W && 0 <= y && y < H;
}

#endif /* GAME_STATE_HPP */
