//
// Created by nugdev on 3/7/24.
//

#pragma once

#include <string>

#include "./Content.h"

namespace nugdev::ndk::ini {

  struct Line {
    std::uint32_t number;
    std::shared_ptr<Content> content;
  };

}