#include "Common.hpp"

#include <cassert>

Point point(int x, int y) {
    return { x, y };
}

bool operator==(const Point& a, const Point& b) {
    return a.x == b.x && a.y == b.y;
}

bool operator!=(const Point& a, const Point& b) {
    return a.x != b.x || a.y != b.y;
}

bool operator<(const Point& a, const Point& b) {
    if (a.x == b.x)
        return a.y < b.y;
    return a.x < b.x;
}

Timer::Timer(time_t timeLimit) : timeLimit(timeLimit), start(clock_t::now()) {

}

void Timer::set(float timeLimit) {
    this->timeLimit = timeLimit;
}

bool Timer::isTimeLeft() const {
    return getTimePassed() <= timeLimit - margin;
}

void Timer::reset() {
    start = clock_t::now();
}

Timer::time_t Timer::getTimePassed() const {
    auto now = clock_t::now();
    return std::chrono::duration<time_t>(now - start).count() * 1000;
}

Timer::time_t Timer::getTimeLeft() const {
    return timeLimit - getTimePassed();
}

std::mt19937 Random::rng(std::random_device{}());

BenchTimer::BenchTimer(const std::string& name) :
    name(name), start(clock_t::now()) {

}

void BenchTimer::stop() {
    assert(!stopped);
    doStop();
}

void BenchTimer::doStop() {
    assert(!stopped);
    stopped = true;
    auto end = clock_t::now();
    auto delta = std::chrono::duration<double>(end - start).count();
    std::cerr << name << ": " << delta * 1000 << "ms" << std::endl; 
}

BenchTimer::~BenchTimer() {
    if (!stopped)
        doStop();
}
