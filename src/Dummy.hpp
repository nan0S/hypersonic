#ifndef DUMMY_HPP
#define DUMMY_HPP

#include "Agent.hpp"

class Dummy : public Agent {
public:
    Dummy();

    Action getAction(const GameState& game,
                     const int timeLimit) override;

private:
    bool firstAction = true;
};
    
#endif /* DUMMY_HPP */