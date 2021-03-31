#include "Manager.hpp"
#include "BeamSearch.hpp"
#include "MCTS.hpp"
#include "RHEA.hpp"

int main() {
    srand(time(NULL));

    using AgentT = BeamSearch;
    // using AgentT = MCTS;
    // using AgentT = RHEA;
    
    Manager::init();
    Manager::run<AgentT>();
        
    return 0;
}
