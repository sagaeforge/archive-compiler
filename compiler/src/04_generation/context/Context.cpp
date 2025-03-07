#include "04_generation/context/Context.h"

namespace nugdev::compiler::generation {

ContextStack::ContextStack() : m_contexts({}) {}

ContextStack::Iterator ContextStack::push(const Context &context) { return m_contexts.push(context); }

Context ContextStack::pop() { return m_contexts.pop(); }

Context ContextStack::top() const {
    if (!m_contexts.current().valid()) {
        throw std::runtime_error("ContextStack is empty");
    }
    return m_contexts.current().value();
}
std::optional<ContextStack::Iterator> ContextStack::find_by_name(const icu::UnicodeString &name) const {
    for (auto it = m_contexts.begin(); it != m_contexts.end(); ++it) {
        if (it->m_name == name) {
            return it;
        }
    }
    return std::nullopt;
}

bool ContextStack::empty() const { return m_contexts.empty(); }

std::optional<icu::UnicodeString> Context::find_label(const icu::UnicodeString &name) const {
    for (auto &label : m_labels) {
        if (label == name) {
            return label;
        }
    }
    return std::nullopt;
}
} // namespace nugdev::compiler::generation
