#ifndef COMMON_HPP
#define COMMON_HPP

#include <iostream>
#include <chrono>
#include <random>

#define all(x) (x).begin(), (x).end()

#if 0
#define SIG() std::cerr << __func__ << std::endl
#else
#define SIG()
#endif
using hash_t = uint64_t;

#if defined(LOCAL) && !defined(NDEBUG)
#define DEBUG
#endif

#ifdef DEBUG

inline void __debug(const char* s, const char*) {
    std::cerr << s << ": ";
}

template<typename T>
void __debug(const char* s, const T& x) {
    std::cerr << s << ": " << x << " ";
}

template<typename T, typename... Args>
void __debug(const char* s, const T& x, const Args&... rest) {
    int bracket = 0;
    char c;
    while ((c = *s) != ',' || bracket)
    {
        std::cerr << *s++;
        switch (c)
        {
            case '(':
            case '{':
            case '[':
                ++bracket;
                break;
            case ')':
            case '}':
            case ']':
                --bracket;
        }
    }
    std::cerr << ": ";
    std::cerr << x << ",";
    __debug(s + 1, rest...);

}

template<typename... Args>
void _debug(const char* s, const Args&... rest) {
    __debug(s, rest...);
    std::cerr << std::endl;
}

#define debug(...) _debug(#__VA_ARGS__, __VA_ARGS__)
#else
#define debug(...)
#endif /* DEBUG */

struct Point {
    int x, y;
};

Point point(int x, int y);

template<typename T>
Point point(const T& p) {
    return { p.x, p.y };
}

bool operator==(const Point& a, const Point& b);
bool operator!=(const Point& a, const Point& b);
bool operator<(const Point& a, const Point& b);

class Timer {
public:
    using time_t = float;
    using clock_t = std::chrono::high_resolution_clock;

    Timer() = default;
    Timer(time_t timeLimit);
    void set(float timeLimit);
    bool isTimeLeft() const;
    time_t getTimePassed() const;
    time_t getTimeLeft() const;
    void reset();

private:
    static constexpr int margin = 5;
    time_t timeLimit;
    clock_t::time_point start;
};

class BenchTimer {
public:
    using clock_t = std::chrono::high_resolution_clock;

    BenchTimer(const std::string& name="BenchTimer");
    void stop();
    ~BenchTimer();

private:
    void doStop();

private:
    std::string name;
    clock_t::time_point start;
    bool stopped = false;
};

#if 0
#define COMBINE1(X,Y) X##Y  // helper macro
#define COMBINE(X,Y) COMBINE1(X,Y)
#define BENCH() BenchTimer COMBINE(benchTimer, __LINE__)(__PRETTY_FUNCTION__)
#else
#define BENCH()
#endif

namespace Random {
    extern std::mt19937 rng;

    template<typename T>
    using T_Int = std::enable_if_t<
        std::is_integral_v<T>, T>;

    template<typename T>
    using NT_Int = std::enable_if_t<
        !std::is_integral_v<T>, T>;

    template<typename T>
    using dist_t = std::conditional_t<
        std::is_integral_v<T>,
        std::uniform_int_distribution<T>,
        std::uniform_real_distribution<T>
    >;

    /* not integeres, random from [0, 1] */
    template<typename T>
    NT_Int<T> rand() {
        return dist_t<T>{0, 1}(rng);
    }

    /* integers, random from [0, n-1] */
    template<typename T>
    T_Int<T> rand(T n) {
        return dist_t<T>{0, n-1}(rng);
    }

    /* not integers, random from [0, x] */
    template<typename T>
    NT_Int<T> rand(T x) {
        return dist_t<T>{0, x}(rng);
    }

    /* integers, random from [a, b] */
    template<typename T>
    T rand(T a, T b) {
        return dist_t<T>{a, b}(rng);
    }
}

template<typename T>
bool almostEqual(T x, T y, T eps=0.001) {
    return std::abs(x - y) <= eps;
}

#endif /* COMMON_HPP */
