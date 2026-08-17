#include "ParseStrategy.h"

namespace nugdev::compiler::parsing {

bool ParseStrategy::contains(const tokenize::TokenStreamIterator &itr, const std::vector<tokenize::TokenType> &types) {
    if (itr.valid() == false) {
        return false;
    }

    for (const auto &type : types) {
        if (itr.value().get_type() == type) {
            return true;
        }
    }
    return false;
}

} // namespace nugdev::compiler::parsing
