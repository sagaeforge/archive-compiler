#pragma once

#include "01_lib/String.h"

namespace nugdev::compiler::scanning {

/**
 * @brief 어떠한 리소스를 받았을 때, 그를 줄 단위로 짤라주는 역할임.
 *
 */
class Scanner {
public:
  Scanner() = default;
  ~Scanner() = default;

public:
  std::vector<lib::String> scan_with_line(const lib::String &line);
  std::vector<lib::String> scan_with_file(const lib::String &file_path);
};

} // namespace nugdev::compiler::scanning