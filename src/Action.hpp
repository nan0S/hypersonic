#ifndef ACTION_HPP
#define ACTION_HPP

#include <iostream>
#include <map>

enum class ActionType {
    move = 0,
    bomb = 1
};

struct Action {
    ActionType type = ActionType::move;
    union { int x = 0; int dx; };
    union { int y = 0; int dy; };

    bool operator<(const Action& a) const;

    friend std::ostream& operator<<(std::ostream& out,
                                    const Action& action);
};

/* player id -> action */
using Actions = std::map<int, Action>;

#endif /* ACTION_HPP */
