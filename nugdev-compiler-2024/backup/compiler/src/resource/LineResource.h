#pragma once

#include "00_lib/resource/Resource.h"

namespace nugdev::compiler::lib {

class LineResource : public Resource {
  public:
    LineResource(const ResourceTag &tag, const lib::String &line);

    ResourceTag get_tag() const override;
    std::uint32_t get_size() const override;

    ResourcePosition get_position() const override;
    lib::Stream<Char> to_stream() const override;

    void set_content(const ResourcePosition &position, const String &content) override;
    bool has_overwrite(const ResourcePosition &position, const String &content) const override;

  private:
    lib::String m_line;
    ResourceTag m_tag;
    std::uint32_t m_index;
};

} // namespace nugdev::compiler::lib