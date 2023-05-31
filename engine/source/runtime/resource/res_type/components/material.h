#pragma once
#include "runtime/resource/res_type/common_serializer.h"
#include "runtime/function/render/render_common.h"

namespace MoYu
{
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(SceneImage, m_is_srgb, m_auto_mips, m_mip_levels, m_image_file)

    struct MaterialRes
    {
        std::string shader_name {""};

        bool    m_blend {false};
        bool    m_double_sided {false};

        Vector4 m_base_color_factor {1.0f, 1.0f, 1.0f, 1.0f};
        float   m_metallic_factor {1.0f};
        float   m_roughness_factor {1.0f};
        float   m_normal_scale {1.0f};
        float   m_occlusion_strength {1.0f};
        Vector3 m_emissive_factor {0.0f, 0.0f, 0.0f};

        SceneImage m_base_color_texture_file {};
        SceneImage m_metallic_roughness_texture_file {};
        SceneImage m_normal_texture_file {};
        SceneImage m_occlusion_texture_file {};
        SceneImage m_emissive_texture_file {};

        float _specularAntiAliasingVariance {0};
        float _specularAntiAliasingThreshold {0};
    };
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(MaterialRes,
                                       shader_name,
                                       m_blend,
                                       m_double_sided,
                                       m_base_color_factor,
                                       m_metallic_factor,
                                       m_roughness_factor,
                                       m_normal_scale,
                                       m_occlusion_strength,
                                       m_emissive_factor,
                                       m_base_color_texture_file,
                                       m_metallic_roughness_texture_file,
                                       m_normal_texture_file,
                                       m_occlusion_texture_file,
                                       m_emissive_texture_file,
                                       _specularAntiAliasingVariance,
                                       _specularAntiAliasingThreshold)

} // namespace MoYu