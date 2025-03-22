#include "LineResource.h"
#include "00_lib/resource/Resource.h"

namespace nugdev::compiler::lib {

LineResource::LineResource(const ResourceTag &tag, const lib::String &line) : m_tag(tag), m_line(line), m_index(0) {}

ResourceTag LineResource::get_tag() const { return m_tag; }

std::uint32_t LineResource::get_size() const { return m_line.length(); }

ResourcePosition LineResource::get_position() const { return ResourcePosition{.tag = m_tag, .row = 0, .column = m_index}; }

lib::Stream<Char> LineResource::to_stream() const {
    std::vector<Char> chars;
    chars.reserve(m_line.length());
    for (auto itr = 0; itr < m_line.length(); itr++) {
        chars.push_back(m_line[itr]);
    }
    return lib::Stream<Char>(chars);
}

void LineResource::set_content(const ResourcePosition &position, const String &content) {
    auto lines = m_line.split("\n");
    if (lines.size() < position.row) {
        lines.resize(position.row, "");
    }

    auto row = lines[position.row - 1];
    for (auto i = row.length(); i < position.column + content.length(); i++) {
        row.append(" ");
    }

    for (auto i = position.column; i < position.column + content.length(); i++) {
        row.setCharAt(i, content[i - position.column]);
    }

    lines[position.row - 1] = row;

    // 대충 구현하기.
    m_line = String(lines, "\n");
}

bool LineResource::has_overwrite(const ResourcePosition &position, const String &content) const {
    auto lines = m_line.split("\n");
    if (lines.size() < position.row) {
        return false;
    }

    auto row = lines[position.row - 1];
    if (row.length() < position.column + content.length()) {
        return false;
    }

    for (auto i = position.column; i < position.column + content.length(); i++) {
        if (row[i] != content[i - position.column]) {
            return false;
        }
    }

    return true;
}

} // namespace nugdev::compiler::lib
