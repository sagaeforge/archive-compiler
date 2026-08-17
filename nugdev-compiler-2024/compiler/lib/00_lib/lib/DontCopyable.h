#pragma once

namespace nugdev::compiler::lib {

class DontCopyable {
public:
    DontCopyable(const DontCopyable &) = delete;
    DontCopyable &operator=(const DontCopyable &) = delete;
};

}  // namespace nugdev::compiler::lib