//
// Created by nugdev on 3/7/24.
//

#pragma once

#include <string>

#include "./Content.h"

namespace nugdev::ndk::ini {

  struct Comment final: Content {
    std::wstring comment;

    explicit Comment(std::wstring _comment) : Content(), comment(std::move(_comment)) {}

    [[nodiscard]] ContentType getType() const override { return ContentType::Comment; }
  };


}