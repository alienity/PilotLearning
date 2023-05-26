#pragma once
#include "runtime/resource/res_type/common_serializer.h"
#include "runtime/resource/res_type/components/material.h"

namespace MoYu
{
    struct MeshRendererComponentRes
    {
        bool        m_is_mesh_data {false};
        std::string m_sub_mesh_file {""};
        std::string m_mesh_data_path {""};

        std::string m_material_file {""};
        bool        m_is_material_init {false};
        MaterialRes m_material_data {};
    };
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(MeshRendererComponentRes, m_sub_mesh_file, m_material_file)
} // namespace MoYu