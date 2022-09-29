#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "runtime/core/math/moyu_math.h"
#include "runtime/core/color/color.h"
#include "runtime/function/framework/object/object_id_allocator.h"

#include <string>
#include <vector>
#include <limits>

namespace Pilot
{
    struct BasePartDesc
    {
        bool m_is_active {false};
    };

    //========================================================================
    // Light Desc
    //========================================================================

    struct AmbientLightDesc : public BasePartDesc
    {
        Vector3 m_position;
        Color   m_color;
    };

    struct DirectionLightDesc : public BasePartDesc
    {
        GObjectID    m_gobject_id {k_invalid_gobject_id};
        GComponentID m_gcomponent_id {k_invalid_gcomponent_id};

        Vector3 m_position;
        Color   m_color;
        float   m_intensity {1.0f};
        Vector3 m_direction;
        Vector3 m_up;
        bool    m_shadowmap {false};
        Vector3 m_shadow_bounds;
        Vector2 m_shadowmap_size;
    };

    struct PointLightDesc : public BasePartDesc
    {
        GObjectID    m_gobject_id {k_invalid_gobject_id};
        GComponentID m_gcomponent_id {k_invalid_gcomponent_id};

        Vector3 m_position;
        Color m_color;
        float m_intensity {1.0f};
        float m_radius;
    };

    struct SpotLightDesc : public BasePartDesc
    {
        GObjectID    m_gobject_id {k_invalid_gobject_id};
        GComponentID m_gcomponent_id {k_invalid_gcomponent_id};

        Vector3 m_position;
        Color m_color;
        float m_intensity;
        float m_radius;
        float m_inner_radians;
        float m_outer_radians;
    };

    //========================================================================
    // Render Desc
    //========================================================================

    struct GameObjectMeshDesc : public BasePartDesc
    {
        std::string m_mesh_file;
    };

    struct SkeletonBindingDesc : public BasePartDesc
    {
        std::string m_skeleton_binding_file;
    };

    struct SkeletonAnimationResultTransform
    {
        Matrix4x4 m_matrix;
    };

    struct SkeletonAnimationResult : public BasePartDesc
    {
        std::vector<SkeletonAnimationResultTransform> m_transforms;
    };

    struct GameObjectMaterialDesc : public BasePartDesc
    {
        std::string m_base_color_texture_file;
        std::string m_metallic_roughness_texture_file;
        std::string m_normal_texture_file;
        std::string m_occlusion_texture_file;
        std::string m_emissive_texture_file;
        bool        m_with_texture {false};
    };

    struct GameObjectTransformDesc : public BasePartDesc
    {
        Matrix4x4  m_transform_matrix {Matrix4x4::Identity};
        Vector3    m_position {Vector3::Zero};
        Quaternion m_rotation {Quaternion::Identity};
        Vector3    m_scale {Vector3::One};
    };

    struct GameObjectComponentDesc
    {
        GComponentID            m_component_id;
        GameObjectTransformDesc m_transform_desc;
        GameObjectMeshDesc      m_mesh_desc;
        GameObjectMaterialDesc  m_material_desc;
        SkeletonBindingDesc     m_skeleton_binding_desc;
        SkeletonAnimationResult m_skeleton_animation_result;
        AmbientLightDesc        m_ambeint_light_desc;
        DirectionLightDesc      m_direction_light_desc;
        PointLightDesc          m_point_light_desc;
        SpotLightDesc           m_spot_light_desc;
    };

    struct GameObjectComponentId
    {
        GObjectID m_go_id {k_invalid_gobject_id};
        size_t m_component_id {k_invalid_gcomponent_id};

        bool operator==(const GameObjectComponentId& rhs) const { return m_go_id == rhs.m_go_id && m_component_id == rhs.m_component_id; }
        size_t getHashValue() const { return m_go_id ^ (m_component_id << 1); }
        bool   isValid() const { return m_go_id != k_invalid_gobject_id && m_component_id != k_invalid_gcomponent_id; }
    };

    class GameObjectDesc
    {
    public:
        GameObjectDesc() : m_go_id(0) {}
        GameObjectDesc(size_t go_id, const std::vector<GameObjectComponentDesc>& parts) : m_go_id(go_id), m_object_components(parts) {}

        GObjectID getId() const { return m_go_id; }
        const std::vector<GameObjectComponentDesc>& getObjectParts() const { return m_object_components; }

    private:
        GObjectID m_go_id {k_invalid_gobject_id};
        std::vector<GameObjectComponentDesc> m_object_components;
    };
} // namespace Pilot

template<>
struct std::hash<Pilot::GameObjectComponentId>
{
    size_t operator()(const Pilot::GameObjectComponentId& rhs) const noexcept { return rhs.getHashValue(); }
};
