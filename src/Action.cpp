#include "Action.hpp"

#include <tuple>

bool Action::operator<(const Action& o) const {
    return std::make_tuple(type, x, y) < std::make_tuple(o.type, o.x, o.y);
}

std::ostream& operator<<(std::ostream& out, const Action& action) {
    return out << (action.type == ActionType::move ? "MOVE" : "BOMB")
               << " " << action.x << " " << action.y;
}
