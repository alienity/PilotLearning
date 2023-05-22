#pragma once

#include <string>
#include <vector>

namespace MoYu
{
    class Component;

    class ComponentDefinitionRes
    {
    public:
        std::string m_type_name;
        std::string m_component;
    };

    class ObjectDefinitionRes
    {
    public:
        std::vector<ComponentDefinitionRes> m_components;
    };

    class ObjectInstanceRes
    {
    public:
        std::string  m_name;
        std::string  m_definition;
        
        std::uint32_t              m_id;
        std::uint32_t              m_parent_id;
        std::uint32_t              m_sibling_index;
        std::vector<std::uint32_t> m_chilren_id;

        std::vector<ComponentDefinitionRes> m_instanced_components;
    };
} // namespace MoYu
