//
// Created by nugdev on 3/7/24.
//

#pragma once

#include <string>

#include "./Content.h"
#include "./Comment.h"

namespace nugdev::ndk::ini {

  struct Field final: Content {
    std::wstring section;
    std::wstring key;
    std::shared_ptr<Comment> comment;
    std::wstring value;

    explicit Field(std::wstring _section, std::wstring _key, std::wstring _value, std::shared_ptr<Comment> _comment) : Content(), section(std::move(_section)), key(_key), comment(std::move(_comment)), value(_value) {}

    [[nodiscard]] ContentType getType() const override { return ContentType::Field; }
  };


}