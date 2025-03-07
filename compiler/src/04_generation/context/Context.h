#pragma once

#include "00_app/lib/UnicodeStringHash.h"
#include "00_app/stream/Stream.hpp"
#include "00_app/tag/Tag.h"

#include <any>
#include <unicode/unistr.h>
#include <unordered_map>

namespace nugdev::compiler::generation {

struct ContextTag : public Tag {};

struct Context {
    icu::UnicodeString m_name;
    ContextTag m_id;
    std::vector<icu::UnicodeString> m_labels;
    std::unordered_map<icu::UnicodeString, std::any> m_variables;

    std::optional<icu::UnicodeString> find_label(const icu::UnicodeString &name) const;
};

struct ContextStack {
  public:
    using ContextStream = stream::MutableStream<Context>;
    using Iterator = typename ContextStream::iterator_t;

  public:
    ContextStack();

  public:
    Iterator push(const Context &context);
    Context pop();
    Context top() const;
    std::optional<Iterator> find_by_name(const icu::UnicodeString &name) const;
    bool empty() const;

  private:
    ContextStream m_contexts;
};

} // namespace nugdev::compiler::generation
