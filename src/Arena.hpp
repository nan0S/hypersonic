#ifndef ARENA_HPP
#define ARENA_HPP

#include "GameState.hpp"
#include "Action.hpp"
#include "Common.hpp"

#include <iostream>
#include <tuple>
#include <algorithm>
#include <iomanip>
#include <fstream>

class Arena {
public:
    template<typename... AgentTs>
    using Agents = std::tuple<AgentTs...>;

    template<typename... AgentTs>
    void fight(int times) {
        init<AgentTs...>();

        for (int i = 0; i < times; ++i) {
            auto winners = fight<AgentTs...>();
            for (const auto& winner : winners)
                stats[winner] += 1.f / winners.size();
        }

        std::cerr << "\nWinrates:\n" << std::string(25, '-') << std::endl;
        std::cerr << std::setprecision(1) << std::fixed;
        for (const auto& [name, points] : stats)
            std::cerr << name << ": " << points / times * 100 << "%" << std::endl;
    }

    template<typename... AgentTs>
    std::vector<std::string> fight() {
        constexpr int N = sizeof...(AgentTs);
        static_assert(2 <= N && N <= 4,
            "Provide from 2 to 4 agents!");

        std::tuple<AgentTs...> agents;
        Initer<N, AgentTs...>::init(agents);

        std::map<int, std::string> agentNames;
        Namer<N, AgentTs...>::name(agents, agentNames);

        auto game = getGame(N);
        stagnation = 0;
        int turn;

        for (turn = 0; !isTerminal(game); ++turn) {
            for (int i = 0; i < N; ++i)
                if (std::find_if(all(game.players), [i](const Player& player){
                        return player.id == i; }) == game.players.end())
                    lastAlive.erase(i);

            Actions agentActions;
            Advancer<N, AgentTs...>::advance(agents, game, agentActions);

            std::set<int> invalids;
            for (const auto& [id, action] : agentActions)
                if (!game.validActions({{id, action}})) {
                    std::cerr << "Invalid action " << id << " " << action << std::endl;
                    invalids.insert(id);
                }
            for (const auto& id : invalids)
                agentActions.erase(id);

            game.myid = -1;
            auto succInfo = game.succ(game.getExplosions(), agentActions);
            // if (!succInfo)
            //     for (const auto& [id, action] : agentActions) {
            //         debug(game.validActions({{id, action}}));
            //         debug(id, action);
            //     }
            assert(succInfo); // if not probably invalid action - get rid of that
            auto& [nextGame, extra] = succInfo.value();

            for (const auto& id : invalids) {
                for (int i = 0; i < int(nextGame.players.size()); ++i) {
                    const auto& player = nextGame.players[i];
                    if (player.id == id) {
                        std::swap(game.players[i], game.players.back());
                        nextGame.players.pop_back();
                        std::cerr << "Kicked out " << i << std::endl;
                        break;
                    }
                }
            }

            ++stagnation;

            for (int i = 0; i < N; ++i)
                if (extra.boxesDestroyed[i]) {
                    stagnation = 0;
                    scores[i] += extra.boxesDestroyed[i];
                }

            game = nextGame;

            std::cerr << "Done " << turn << " turns\r";
        }

        std::cerr << "\rTotal number of turns: " << turn << std::endl;

        std::cerr << "Winners: ";
        bool first = true;
        std::vector<std::string> winners;
        auto winnerIds = getWinnersIds(game);

        for (const auto& winnerId : winnerIds) {
            if (!first)
                std::cerr << ", ";
            else
                first = false;
            assert(agentNames.count(winnerId));
            std::string winnerName = agentNames[winnerId] +
                "-" + std::to_string(winnerId);
            std::cerr << winnerName;
            winners.push_back(winnerName);
        }
        std::cerr << std::endl;

        std::cerr << "Boxes destroyed:" << std::endl;
        for (const auto& [id, boxes] : scores) {
            std::string winnerName = agentNames[id] +
                "-" + std::to_string(id);
            std::cerr << winnerName << ": " << scores[id] << std::endl;
        }
        std::cerr << std::endl;

        return winners;
    }

private:
    template<int i, typename... AgentTs>
    struct Initer {
        static void init(Agents<AgentTs...>& agents) {
            auto& agent = std::get<i - 1>(agents);
            agent.init(GameState::H, GameState::W, i - 1, false);
            Initer<i - 1, AgentTs...>::init(agents);
        }
    };

    template<typename... AgentTs>
    struct Initer<0, AgentTs...> {
        static void init(Agents<AgentTs...>&) {}
    };

    template<int i, typename... AgentTs>
    struct Namer {
        static void name(const Agents<AgentTs...>& agents,
                         std::map<int, std::string>& names) {
            const auto& agent = std::get<i - 1>(agents);
            assert(!names.count(agent.myid));
            names[agent.myid] = agent.name;
            Namer<i - 1, AgentTs...>::name(agents, names);
        }
    };

    template<typename... AgentTs>
    struct Namer<0, AgentTs...> {
        static void name(const Agents<AgentTs...>&,
                         std::map<int, std::string>&) {}
    };

    template<int i, typename... AgentTs>
    struct Debuger {
        static void dbg(Agents<AgentTs...>& agents) {
            auto& agent = std::get<i - 1>(agents);
            debug(agent.height, agent.width, agent.myid);
            Debuger<i - 1, AgentTs...>::dbg(agents);
        }
    };

    template<typename... AgentTs>
    struct Debuger<0, AgentTs...> {
        static void dbg(Agents<AgentTs...>&) {}
    };

    template<int i, typename... AgentTs>
    struct Advancer {
        static void advance(Agents<AgentTs...>& agents,
                            GameState& game, Actions& agentActions) {
            auto& agent = std::get<i - 1>(agents);
            if (game.getPlayer(agent.myid)) {
                game.myid = i - 1;
                auto action = agent.getAction(game, timeLimit);
                assert(!agentActions.count(agent.myid));
                agentActions[agent.myid] = action;
            }
            Advancer<i - 1, AgentTs...>::advance(agents, game, agentActions);
        }
    };

    template<typename... AgentTs>
    struct Advancer<0, AgentTs...> {
        static void advance(Agents<AgentTs...>&,
                            GameState&, Actions&) {}
    };

    template<typename... AgentTs>
    void init() {
        stats.clear();

        constexpr int N = sizeof...(AgentTs);
        std::tuple<AgentTs...> agents;
        Initer<N, AgentTs...>::init(agents);

        std::map<int, std::string> agentNames;
        Namer<N, AgentTs...>::name(agents, agentNames);
        assert(agentNames.size() == N);

        for (const auto& [id, name] : agentNames) {
            auto nameWithId = name + "-" + std::to_string(id);
            stats[nameWithId] = 0;
        }

        for (int i = 0; i < TESTS; ++i) {
            std::string filename = "./tests/testcase" + std::to_string(i + 1) + ".txt";
            std::ifstream file(filename);
            assert(file.is_open());
            file >> initialGames[i];
        }
    }

    GameState getGame(const int numPlayers);
    bool isTerminal(const GameState& game);
    std::vector<int> getWinnersIds(const GameState& game);

private:
    static constexpr int timeLimit = 30;
    static constexpr std::pair<int, int> positions[] = {
        {0, 0},
        {GameState::W - 1, GameState::H - 1},
        {GameState::W - 1, 0},
        {0, GameState::H - 1}
    };
    static constexpr int TESTS = 2;

    std::map<std::string, float> stats;
    GameState initialGames[TESTS];
    std::map<int, int> scores;
    std::set<int> lastAlive;
    int stagnation = 0;
};

#endif /* ARENA_HPP */