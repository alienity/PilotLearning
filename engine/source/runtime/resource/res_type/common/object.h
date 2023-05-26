#pragma once

#include "runtime/resource/res_type/common_serializer.h"

namespace MoYu
{
    struct ComponentDefinitionRes
    {
        std::string m_type_name;
        std::string m_component;
    };
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ComponentDefinitionRes, m_type_name, m_component)

    struct ObjectInstanceRes
    {
        std::string  m_name;
   
        int              m_id;
        int              m_parent_id;
        std::vector<int> m_chilren_id;

        std::vector<ComponentDefinitionRes> m_instanced_components;
    };
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ObjectInstanceRes,
                                       m_name,
                                       m_id,
                                       m_parent_id,
                                       m_chilren_id,
                                       m_instanced_components)
} // namespace MoYu
