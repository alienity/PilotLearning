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

        std::string m_base_color_texture_file;
        std::string m_metallic_roughness_texture_file;
        std::string m_normal_texture_file;
        std::string m_occlusion_texture_file;
        std::string m_emissive_texture_file;

        float _specularAntiAliasingVariance {0};
        float _specularAntiAliasingThreshold {0};
    };

    inline bool operator==(const MaterialRes& lhs, const MaterialRes& rhs)
    {
#define MatPropEqual(a) lhs.a == rhs.a


        return MatPropEqual(m_blend) &&
               MatPropEqual(m_double_sided) && 
               MatPropEqual(m_base_color_factor) && 
               MatPropEqual(m_metallic_factor) && 
               MatPropEqual(m_roughness_factor) && 
               MatPropEqual(m_normal_scale) && 
               MatPropEqual(m_occlusion_strength) && 
               MatPropEqual(m_emissive_factor) && 
               MatPropEqual(m_base_color_texture_file) && 
               MatPropEqual(m_metallic_roughness_texture_file) && 
               MatPropEqual(m_normal_texture_file) && 
               MatPropEqual(m_occlusion_texture_file) && 
               MatPropEqual(m_emissive_texture_file) && 
               MatPropEqual(_specularAntiAliasingVariance) && 
               MatPropEqual(_specularAntiAliasingThreshold);
    }
    inline bool operator!=(const MaterialRes& lhs, const MaterialRes& rhs) { return !(lhs == rhs); }

} // namespace Pilot