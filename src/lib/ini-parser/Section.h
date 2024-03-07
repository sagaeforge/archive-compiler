//
// Created by nugdev on 3/7/24.
//

#pragma once

#include <string>

#include "./Content.h"
#include "./Comment.h"

namespace nugdev::ndk::ini {

  struct Section final: Content {
    std::wstring name;
    std::shared_ptr<Comment> comment;

    explicit Section(std::wstring _name, std::shared_ptr<Comment> _comment) : Content(), name(std::move(_name)), comment(std::move(_comment)) {}

    [[nodiscard]] ContentType getType() const override { return ContentType::Section; }
  };

}
