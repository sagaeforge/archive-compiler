#include <iostream>
#include <vector>

#include "00_lib/iterator/Workbench.hpp"
#include "00_lib/lib/Exception.h"
#include "00_lib/lib/Transformer.hpp"

using namespace nugdev::compiler::lib;

int main() {
    std::vector<int> vec = {1, 2, 3, 4, 5};
    auto result = iterator::Workbench<int>::from(vec).stream([](const iterator::Workbench<int>::command_t &context) {
        auto value = context.value();
        return value * 2;
    });
    for (auto &value : result) {
        std::cout << value << std::endl;
    }
}
