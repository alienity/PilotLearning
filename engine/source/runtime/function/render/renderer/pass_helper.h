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

    struct MipGenInBuffer
    {
        glm::uint   SrcMipLevel;  // Texture level of source mip
        glm::uint   NumMipLevels; // Number of OutMips to write: [1, 4]
        glm::float2 TexelSize;    // 1.0 / OutMip1.Dimensions

        glm::uint SrcIndex;
        glm::uint OutMip1Index;
        glm::uint OutMip2Index;
        glm::uint OutMip3Index;
        glm::uint OutMip4Index;
    };


    struct GBufferOutput : public PassOutput
    {
        GBufferOutput()
        {
            albedoHandle.Invalidate();
            worldNormalHandle.Invalidate();
            worldTangentHandle.Invalidate();
            matNormalHandle.Invalidate();
            emissiveHandle.Invalidate();
            metallic_Roughness_Reflectance_AO_Handle.Invalidate();
            clearCoat_ClearCoatRoughness_Anisotropy_Handle.Invalidate();
            depthHandle.Invalidate();
        }

        RHI::RgResourceHandle albedoHandle;
        RHI::RgResourceHandle worldNormalHandle;
        RHI::RgResourceHandle worldTangentHandle;
        RHI::RgResourceHandle matNormalHandle;
        RHI::RgResourceHandle emissiveHandle;
        RHI::RgResourceHandle metallic_Roughness_Reflectance_AO_Handle;
        RHI::RgResourceHandle clearCoat_ClearCoatRoughness_Anisotropy_Handle;
        RHI::RgResourceHandle depthHandle;
    };


}

