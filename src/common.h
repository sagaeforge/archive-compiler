//
// Created by lambda on 10/25/25.
//

#pragma once

#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include <fstream>

using ch_t = char;
using string_t = std::string;
using stringView_t = std::string_view;
using size_t = std::size_t;

constexpr const char SOURCE_CODE_FILE_EXTENSION[] = ".l";

#include "logger.hpp"
