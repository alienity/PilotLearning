#pragma once

#include "runtime/resource/res_type/common_serializer.h"
#include <vector>

namespace MoYu
{
    struct ComponentDefinitionRes
    {
        std::string m_type_name;
        std::string m_component_name;
        std::vector<uint64_t> m_component_data;
    };
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ComponentDefinitionRes, m_type_name, m_component_name, m_component_data)

    struct ObjectInstanceRes
    {
        std::string m_name;
   
        int m_id;
        int m_parent_id;
        int m_sibling_index;
        std::vector<int> m_chilren_id;

        std::vector<ComponentDefinitionRes> m_instanced_components;
    };
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ObjectInstanceRes,
                                       m_name,
                                       m_id,
                                       m_parent_id,
                                       m_sibling_index,
                                       m_chilren_id,
                                       m_instanced_components)
} // namespace MoYu
