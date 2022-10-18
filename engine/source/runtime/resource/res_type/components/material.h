#pragma once
#include "runtime/core/math/moyu_math.h"
#include "runtime/core/meta/reflection/reflection.h"

namespace Pilot
{
    REFLECTION_TYPE(MaterialBase)
    CLASS(MaterialBase, Fields)
    {
        REFLECTION_BODY(MaterialBase);

    public:
        std::string shader_name;

        bool m_blend {false};
        bool m_double_sided {false};
    };

    REFLECTION_TYPE(MaterialRes)
    CLASS(MaterialRes : public MaterialBase, Fields)
    {
        REFLECTION_BODY(MaterialRes);

    public:
        Vector4 m_base_color_factor {1.0f, 1.0f, 1.0f, 1.0f};
        float   m_metallic_factor {1.0f};
        float   m_roughness_factor {1.0f};
        float   m_normal_scale {1.0f};
        float   m_occlusion_strength {1.0f};
        Vector3 m_emissive_factor {0.0f, 0.0f, 0.0f};

        std::string m_base_colour_texture_file;
        std::string m_metallic_roughness_texture_file;
        std::string m_normal_texture_file;
        std::string m_occlusion_texture_file;
        std::string m_emissive_texture_file;
    };

    inline bool operator==(const MaterialRes& lhs, const MaterialRes& rhs)
    {
        return lhs.m_blend == rhs.m_blend && lhs.m_double_sided == rhs.m_double_sided &&
               lhs.m_base_color_factor == rhs.m_base_color_factor && lhs.m_metallic_factor == rhs.m_metallic_factor &&
               lhs.m_roughness_factor == rhs.m_roughness_factor && lhs.m_normal_scale == rhs.m_normal_scale &&
               lhs.m_occlusion_strength == rhs.m_occlusion_strength && lhs.m_emissive_factor == rhs.m_emissive_factor &&
               lhs.m_base_colour_texture_file == rhs.m_base_colour_texture_file &&
               lhs.m_metallic_roughness_texture_file == rhs.m_metallic_roughness_texture_file &&
               lhs.m_normal_texture_file == rhs.m_normal_texture_file &&
               lhs.m_occlusion_texture_file == rhs.m_occlusion_texture_file &&
               lhs.m_emissive_texture_file == rhs.m_emissive_texture_file;
    }
    inline bool operator!=(const MaterialRes& lhs, const MaterialRes& rhs) { return !(lhs == rhs); }

} // namespace Pilot