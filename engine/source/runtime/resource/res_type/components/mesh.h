#pragma once
#include "runtime/core/math/moyu_math.h"
#include "runtime/core/meta/reflection/reflection.h"
#include "runtime/resource/res_type/components/mesh.h"

#include "runtime/resource/res_type/components/material.h"

namespace MoYu
{
    REFLECTION_TYPE(SubMeshRes)
    CLASS(SubMeshRes, Fields)
    {
        REFLECTION_BODY(SubMeshRes);

    public:
        std::string m_obj_file_ref;
        Transform   m_transform;
        std::string m_material;
    };

    REFLECTION_TYPE(MeshComponentRes)
    CLASS(MeshComponentRes, Fields)
    {
        REFLECTION_BODY(MeshComponentRes);

    public:
        std::vector<SubMeshRes> m_sub_meshes;
    };
} // namespace MoYu