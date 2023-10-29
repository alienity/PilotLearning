#pragma once
#include "runtime/resource/res_type/common_serializer.h"
#include "runtime/function/render/render_common.h"

namespace MoYu
{
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(SceneImage, m_is_srgb, m_auto_mips, m_mip_levels, m_image_file)
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(MaterialImage, m_tilling, m_image)

    struct MaterialRes
    {
        std::string shader_name {""};

        bool    m_blend {false};
        bool    m_double_sided {false};

        glm::float4 m_base_color_factor {1.0f, 1.0f, 1.0f, 1.0f};
        float   m_metallic_factor {1.0f};
        float   m_roughness_factor {0.0f};
        float   m_reflectance_factor {0.5f};
        float   m_clearcoat_factor {1.0f};
        float   m_clearcoat_roughness_factor {0.0f};
        float   m_anisotropy_factor {0.0f};

        MaterialImage m_base_color_texture_file {};
        MaterialImage m_metallic_roughness_texture_file {};
        MaterialImage m_normal_texture_file {};
        MaterialImage m_occlusion_texture_file {};
        MaterialImage m_emissive_texture_file {};

        float _specularAntiAliasingVariance {0};
        float _specularAntiAliasingThreshold {0};
    };
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(MaterialRes,
                                       shader_name,
                                       m_blend,
                                       m_double_sided,
                                       m_base_color_factor,
                                       m_metallic_factor,
                                       m_roughness_factor,
                                       m_reflectance_factor,
                                       m_clearcoat_factor,
                                       m_clearcoat_roughness_factor,
                                       m_anisotropy_factor,
                                       m_base_color_texture_file,
                                       m_metallic_roughness_texture_file,
                                       m_normal_texture_file,
                                       m_occlusion_texture_file,
                                       m_emissive_texture_file,
                                       _specularAntiAliasingVariance,
                                       _specularAntiAliasingThreshold)

} // namespace MoYu