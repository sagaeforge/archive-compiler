#pragma once

#include <optional>

#include <01_lib/String.h>

namespace nugdev::lib {

template <typename Source, typename Target> class Serializable {
public:
  using SourceType = Source;
  using TargetType = Target;
  template <typename T> using OptionalType = std::optional<T>;

public:
  virtual ~Serializable() = default;

  virtual std::optional<Target> serialize(const Source &source) const = 0;
  virtual std::optional<Source> deserialize(const Target &target) const = 0;
};

} // namespace nugdev::lib