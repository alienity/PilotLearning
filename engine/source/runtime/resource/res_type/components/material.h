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

} // namespace Pilot