//
// Created by nugdev on 3/7/24.
//

#pragma once

#include "./ContentType.h"

namespace nugdev::ndk::ini {

  struct Content {
    virtual ~Content() = default;

    [[nodiscard]] virtual ContentType getType() const { return ContentType::None; };

  };

}