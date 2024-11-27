#pragma once
#include "runtime/resource/res_type/common_serializer.h"
#include "material.h"
#include <vector>

namespace MoYu
{
    struct MeshComponentRes
    {
        bool m_is_mesh_data {false};
        std::string m_sub_mesh_file {""};
        std::string m_mesh_data_path {""};
    };
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(MeshComponentRes, m_is_mesh_data, m_sub_mesh_file, m_mesh_data_path)

    struct MeshRendererComponentRes
    {
        MeshComponentRes     m_mesh_res {};
        MaterialComponentRes m_material_res {};
    };
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(MeshRendererComponentRes, m_mesh_res, m_material_res)
} // namespace MoYu