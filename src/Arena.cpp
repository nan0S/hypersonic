#include "Arena.hpp"

GameState Arena::getGame(const int numPlayers) {
    int gameIdx = Random::rand(TESTS);
    auto game = initialGames[gameIdx];

    int perm[numPlayers];
    std::iota(perm, perm + numPlayers, 0);
    std::shuffle(perm, perm + numPlayers, Random::rng);

    game.players.clear();
    for (int i = 0; i < numPlayers; ++i)
        game.players.push_back({i,
            positions[perm[i]].first, positions[perm[i]].second,
            1, 3});

    scores.clear();
    lastAlive.clear();
    for (int i = 0; i < numPlayers; ++i)
        lastAlive.insert(i);

    return game;
}

bool Arena::isTerminal(const GameState& game) {
    return game.players.size() <= 1 || stagnation >= 20;
}

std::vector<int> Arena::getWinnersIds(const GameState& game) {
    switch (int(game.players.size())) {
        case 0: {
            assert(lastAlive.size() > 1);
            std::vector<int> winners;
            int winnerScore = -1;

            for (int id : lastAlive)
                if (scores[id] > winnerScore) {
                    winnerScore = scores[id];
                    winners = {id};
                }
                else if (scores[id] == winnerScore)
                    winners.push_back(id);

            assert(!winners.empty());
            return winners;
        }
        case 1:
            return {game.players[0].id};
        default:
            std::vector<int> winners;
            int winnerScore = -1;

            for (const auto& player : game.players)
                if (scores[player.id] > winnerScore) {
                    winnerScore = scores[player.id];
                    winners = {player.id};
                }
                else if (scores[player.id] == winnerScore)
                    winners.push_back(player.id);

            assert(!winners.empty());
            return winners;
    }
}