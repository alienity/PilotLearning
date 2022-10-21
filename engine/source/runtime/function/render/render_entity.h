#pragma once

#include "runtime/core/math/moyu_math.h"
#include "function/framework/object/object_id_allocator.h"

#include <cstdint>
#include <vector>

namespace Pilot
{
    class RenderEntity
    {
    public:
        GObjectID    m_gobject_id {k_invalid_gobject_id};
        GComponentID m_gcomponent_id {k_invalid_gcomponent_id};

        Matrix4x4 m_model_matrix {Matrix4x4::Identity};

        // mesh
        size_t                 m_mesh_asset_id {0};
        bool                   m_enable_vertex_blending {false};
        std::vector<Matrix4x4> m_joint_matrices;
        AxisAlignedBox         m_bounding_box;

        // material
        size_t  m_material_asset_id {0};
    };
} // namespace Pilot
