#include "GameState.hpp"
#include "Common.hpp"
#include "State.hpp"

static constexpr int MAX_TURNS = 15;
static constexpr int TIME_LIMIT = 500;

int main() {
    int myid;
    GameState gameState;
    int totalTurns = 0;
    int deadRounds = 0;
    int allRounds = 0;

    std::cin >> myid;
    std::cin >> gameState;

    State initialState = State::getInitialState(gameState);

    Timer timer(TIME_LIMIT);
    while (timer.isTimeLeft()) {
        State state = initialState;

        for (int i = 0; i < MAX_TURNS && timer.isTimeLeft(); ++i) {
            std::vector<Action> actions = state.game.getActions();
            int actionIdx = Random::rand<int>(0, actions.size() - 1);
            const auto& action = actions[actionIdx];

            auto next = state.succ({{ myid, action }});
            ++totalTurns;
            if (!next) {
                ++deadRounds;
                break;
            }

            state = next.value();
        }

        ++allRounds;
    }

    std::cout << "dead/all rounds: " << deadRounds << "/" << allRounds << std::endl;
    std::cout << "totalTurns: " << totalTurns << std::endl;
    std::cout << totalTurns / allRounds << std::endl;

    return 0;
}
