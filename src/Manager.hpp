#ifndef MANAGER_HPP
#define MANAGER_HPP

#include "GameState.hpp"
#include "Agent.hpp"
#include "Action.hpp"

#include <iostream>
#include <cassert>
#include <chrono>

class Manager {
public:
    static void init();
    template<typename AgentT>
    static void run();

private:
    static constexpr int firstRoundTimeLimit = 1000;
    static constexpr int timeLimit = 100;
};

template<typename AgentT>
void Manager::run() {
    AgentT agent;
    agent.init(GameState::H, GameState::W, GameState::myid);

    bool firstRound = true;

    /* game loop */
    while (true) {
        GameState gameState;
        std::cin >> gameState;

        auto start = std::chrono::high_resolution_clock::now();
        Action action = agent.getAction(gameState, firstRound ? firstRoundTimeLimit : timeLimit);
        std::cout << action << std::endl;
        auto end = std::chrono::high_resolution_clock::now();
        
        firstRound = false;

        double passed = std::chrono::duration<double>(end - start).count();
        std::cerr << "time elapsed: " << passed * 1000 << "ms" << std::endl;
        std::cerr << "action chosen: " << action << std::endl;
    }
}

#endif /* MANAGER_HPP */
