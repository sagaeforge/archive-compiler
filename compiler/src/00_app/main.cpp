#include "00_app/repl/repl.h"

int main() {
    using namespace nugdev::compiler::repl;
    Repl{}.run();
    return 0;
}