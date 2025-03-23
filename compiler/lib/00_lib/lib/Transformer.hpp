#pragma once

#include <functional>

namespace nugdev::compiler::lib {

template <typename Element, template <typename...> class Container = std::vector>
class IFunctional {
public:
    using size_t = std::uint32_t;
    using index_t = std::uint32_t;

public:
    template <typename Mapper, typename Return = typename std::invoke_result<Mapper, const Element &>::type>
        requires std::is_invocable_v<Mapper, const Element &>
    Container<Return> map(Mapper mapper) {
        std::vector<Return> container;
        container.reserve(m_sizer());
        for (std::uint32_t i = 0; i < m_sizer(); ++i) {
            container.push_back(mapper(m_getter(i)));
        }
        return Container<Return>(container);
    }

    template <typename Mapper>
        requires std::is_invocable_v<Mapper, const Element &>
    Container<Element> filter(Mapper mapper) {
        std::vector<Element> container;
        container.reserve(m_sizer());
        for (std::uint32_t i = 0; i < m_sizer(); ++i) {
            if (mapper(m_getter(i))) {
                container.push_back(m_getter(i));
            }
        }
        return Container<Element>(container);
    }

    template <typename Pred>
        requires std::is_invocable_v<Pred, const Element &>
    std::optional<Element> find(Pred pred) {
        for (std::uint32_t i = 0; i < m_sizer(); ++i) {
            if (pred(m_getter(i))) {
                return m_getter(i);
            }
        }
        return std::nullopt;
    }

    template <typename Pred>
        requires std::is_invocable_v<Pred, const Element &>
    Container<Element> find_all(Pred pred) {
        std::vector<Element> container;
        container.reserve(m_sizer());
        for (std::uint32_t i = 0; i < m_sizer(); ++i) {
            if (pred(m_getter(i))) {
                container.push_back(m_getter(i));
            }
        }
        return Container<Element>(container);
    }

    Container<Element> slice(const index_t &start, const index_t &end) {
        Container<Element> container;
        container.reserve(end - start);
        for (std::uint32_t i = start; i < end; ++i) {
            container.push_back(m_getter(i));
        }
        return container;
    }

    template <typename Func>
        requires std::is_invocable_v<Func, const Element &>
    void foreach (Func func) {
        for (std::uint32_t i = 0; i < m_sizer(); ++i) {
            func(m_getter(i));
        }
    }

protected:
    IFunctional(std::function<std::uint32_t()> sizer, std::function<Element(const std::uint32_t &)> getter) : m_sizer(sizer), m_getter(getter) {
    }

private:
    std::function<std::uint32_t()> m_sizer;
    std::function<Element(const std::uint32_t &)> m_getter;
};

#define IMPLEMENT_FUNCTIONAL(TYPE, CONTAINER, FIELD) IFunctional<TYPE, CONTAINER>([this]() { return FIELD.size(); }, [this](const std::uint32_t &index) { return FIELD[index]; })

}  // namespace nugdev::compiler::lib
