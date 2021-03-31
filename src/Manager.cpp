#include "Manager.hpp"

void Manager::init() {
    int width, height, myid;
    std::cin >> width >> height >> myid;
    GameState::myid = myid;
    assert(width == GameState::W && height == GameState::H);
}
