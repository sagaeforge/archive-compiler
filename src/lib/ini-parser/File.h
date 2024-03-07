//
// Created by nugdev on 3/6/24.
//

#pragma once

#include <any>
#include <string>
#include <vector>

#include "Line.h"

namespace nugdev::ndk::ini {

  class File {
  public:
    explicit File(std::wstring _path) : path(std::move(_path)), lines() {}
    ~File() = default;

  public:
    void parse();
    void save();
    void save(const std::wstring& _path);
    [[nodiscard]] std::vector<Line> getLines() const { return lines; }

  public:
    friend std::wostream& operator<<(std::wostream& _os, const File& _file);

  private:
    std::wstring path;
    std::vector<Line> lines;
  };

  std::wostream& operator<<(std::wostream& _os, const File& _file);

}
