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


/**
 * 유틸리티 함수들
 */

// 벡터에서 요소 제거
template<typename T>
inline void vector_remove(std::vector<T> &vec, const T &value) {
    vec.erase(std::remove(vec.begin(), vec.end(), value), vec.end());
}

// 벡터 포함 검사
template<typename T>
inline bool vector_contains(const std::vector<T> &vec, const T &value) {
    return std::find(vec.begin(), vec.end(), value) != vec.end();
}
