#pragma once

#include "00_lib/resource/Resource.h"

namespace nugdev::compiler::lib {

class Resource;

class ResourceContainer {
  public:
    ResourceContainer() = default;

    template <typename Type, typename... Args>
        requires std::is_base_of_v<Resource, Type>
    static std::shared_ptr<Type> create(Args &&...args) {
        auto tag = make_tag<ResourceTag>();
        auto resource = std::make_shared<Type>(tag, std::forward<Args>(args)...);
        m_resources.push_back(resource);
        return resource;
    }

    template <typename Type, typename... Args>
        requires std::is_base_of_v<Resource, Type>
    static std::shared_ptr<Type> create(const ResourceTag &tag, Args &&...args) {
        auto resource = std::make_shared<Type>(tag, std::forward<Args>(args)...);
        m_resources.push_back(resource);
        return resource;
    }

    static bool has(const ResourceTag &tag) {
        return std::find_if(m_resources.begin(), m_resources.end(), [&tag](const std::shared_ptr<Resource> &resource) { return resource->get_tag() == tag; }) !=
               m_resources.end();
    }

    template <typename Type> static std::shared_ptr<Type> get(const ResourceTag &tag) {
        auto itr =
            std::find_if(m_resources.begin(), m_resources.end(), [&tag](const std::shared_ptr<Resource> &resource) { return resource->get_tag() == tag; });
        if (itr == m_resources.end()) {
            throw std::runtime_error("Resource not found");
        }
        return std::dynamic_pointer_cast<Type>(*itr);
    }

    template <typename Type> static void set_content(const ResourcePosition &position, const String &content) {
        auto resource = get<Type>(position.tag);
        resource->set_content(position, content);
    }

  private:
    static std::vector<std::shared_ptr<Resource>> m_resources;
};

} // namespace nugdev::compiler::lib
