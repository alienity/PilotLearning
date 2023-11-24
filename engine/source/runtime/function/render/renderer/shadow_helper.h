#pragma once

#include "runtime/function/render/render_pass.h"
#include "runtime/function/global/global_context.h"
#include "runtime/resource/config_manager/config_manager.h"

namespace MoYu
{
    struct DirectionShadowmapStruct
    {
        SceneCommonIdentifier m_identifier {UndefCommonIdentifier};
        glm::float2 m_shadowmap_bounds;
        glm::float2 m_shadowmap_size;
        uint32_t m_casccade;

        std::shared_ptr<RHI::D3D12Texture> p_LightShadowmap;

        void Reset() 
        {
            m_identifier       = UndefCommonIdentifier;
            p_LightShadowmap   = nullptr;
            m_shadowmap_bounds = MYFloat2::One;
            m_shadowmap_size   = MYFloat2::One;
            m_casccade         = 4;
        }
    };

    struct SpotShadowmapStruct
    {
        SceneCommonIdentifier m_identifier {UndefCommonIdentifier};
        uint32_t m_spot_index;
        glm::float2  m_shadowmap_size;
        std::shared_ptr<RHI::D3D12Texture> p_LightShadowmap;
        
        void Reset()
        {
            m_identifier     = UndefCommonIdentifier;
            p_LightShadowmap = nullptr;
        }
    };

}

