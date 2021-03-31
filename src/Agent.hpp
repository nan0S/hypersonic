#ifndef AGENT_HPP
#define AGENT_HPP

#include "Action.hpp"
#include "GameState.hpp"

class Agent {
public:
    virtual void init(int height, int width, int myid, bool verbose=true);
    virtual Action getAction(const GameState& game, const int timeLimit);
    virtual ~Agent() = default;

public:
    std::string name = "UNKNOWN";
    int height, width;
    int myid;
    bool verbose;
};

#endif /* AGENT_HPP */
