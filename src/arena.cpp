#include "Arena.hpp"
#include "BeamSearch.hpp"
#include "MCTS.hpp"
#include "RHEA.hpp"
#include "Dummy.hpp"

int main() {
    int times = 100;

    Arena arena;
    arena.fight<
        RHEA,
        MCTS>(times);

    return 0;
}