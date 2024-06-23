#pragma once

#include "runtime/function/render/render_pass.h"
#include "runtime/function/global/global_context.h"
#include "runtime/resource/config_manager/config_manager.h"

namespace MoYu
{
#define GImport(g, b) g.Import(b)
#define RegGetBuf(h) registry->GetD3D12Buffer(h)
#define RegGetBufCounter(h) registry->GetD3D12Buffer(h)->GetCounterBuffer().get()
#define RegGetTex(h) registry->GetD3D12Texture(h)
#define RegGetBufDefCBVIdx(h) registry->GetD3D12Buffer(h)->GetDefaultCBV()->GetIndex()
#define RegGetBufDefSRVIdx(h) registry->GetD3D12Buffer(h)->GetDefaultSRV()->GetIndex()
#define RegGetBufDefUAVIdx(h) registry->GetD3D12Buffer(h)->GetDefaultUAV()->GetIndex()
#define RegGetBufCounterSRVIdx(h) registry->GetD3D12Buffer(h)->GetCounterBuffer()->GetDefaultSRV()->GetIndex()
#define RegGetTexDefSRVIdx(h) registry->GetD3D12Texture(h)->GetDefaultSRV()->GetIndex()
#define RegGetTexDefUAVIdx(h) registry->GetD3D12Texture(h)->GetDefaultUAV()->GetIndex()

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
            gbuffer0Handle.Invalidate();
            gbuffer1Handle.Invalidate();
            gbuffer2Handle.Invalidate();
            gbuffer3Handle.Invalidate();
            depthHandle.Invalidate();
        }

        RHI::RgResourceHandle gbuffer0Handle;
        RHI::RgResourceHandle gbuffer1Handle;
        RHI::RgResourceHandle gbuffer2Handle;
        RHI::RgResourceHandle gbuffer3Handle;
        RHI::RgResourceHandle depthHandle;
    };

    enum StencilUsage
    {
        Clear = 0,

        SubsurfaceScattering = (1 << 2),     //  SSS, Split Lighting
    };

}

