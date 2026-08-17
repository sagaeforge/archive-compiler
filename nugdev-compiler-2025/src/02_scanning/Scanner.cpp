#include "Scanner.h"

#include <fstream>
#include <iostream>

namespace nugdev::compiler::scanning {

std::vector<lib::String> Scanner::scan_with_line(const lib::String &line) {
  return line.split("\n");
}

std::vector<lib::String> Scanner::scan_with_file(const lib::String &file_path) {
  std::string path = file_path.to_string();
  std::ifstream file(path, std::ios::in);

  if (!file.is_open() || !file.good()) {
    return {};
  }

  std::vector<lib::String> result;
  std::string line;

  while (file.good() && std::getline(file, line)) {
    result.push_back(lib::String(line));
  }

  file.close();
  return result;
}

} // namespace nugdev::compiler::scanning