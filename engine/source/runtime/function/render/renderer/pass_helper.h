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
        SceneCommonIdentifier m_identifier{ UndefCommonIdentifier };
        glm::float2 m_atlas_size;
        glm::float2 m_shadowmap_size;
        uint32_t m_casccade;
        std::shared_ptr<RHI::D3D12Texture> p_LightCascadeShadowmap;

        void Reset() 
        {
            p_LightCascadeShadowmap = nullptr;
            m_identifier = UndefCommonIdentifier;
            m_atlas_size = MYFloat2::One;
            m_shadowmap_size = MYFloat2::One;
            m_casccade = 4;
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

            historyNormalBufferHandle.Invalidate();
        }

        RHI::RgResourceHandle gbuffer0Handle;
        RHI::RgResourceHandle gbuffer1Handle;
        RHI::RgResourceHandle gbuffer2Handle;
        RHI::RgResourceHandle gbuffer3Handle;
        RHI::RgResourceHandle depthHandle;

        RHI::RgResourceHandle historyNormalBufferHandle;
    };

    enum StencilUsage
    {
        Clear = 0,

        // Note: first bit is free and can still be used by both phases.

        // --- Following bits are used before transparent rendering ---
        IsUnlit = (1 << 0), // Unlit materials (shader and shader graph) except for the shadow matte
        RequiresDeferredLighting = (1 << 1),
        SubsurfaceScattering = (1 << 2),     //  SSS, Split Lighting
        TraceReflectionRay = (1 << 3),     //  SSR or RTR - slot is reuse in transparent
        Decals = (1 << 4),     //  Use to tag when an Opaque Decal is render into DBuffer
        ObjectMotionVector = (1 << 5),     //  Animated object (for motion blur, SSR, SSAO, TAA)

        // --- Stencil  is cleared after opaque rendering has finished ---

        // --- Following bits are used exclusively for what happens after opaque ---
        ExcludeFromTUAndAA = (1 << 1),    // Disable Temporal Upscaling and Antialiasing for certain objects
        DistortionVectors = (1 << 2),    // Distortion pass - reset after distortion pass, shared with SMAA
        SMAA = (1 << 2),    // Subpixel Morphological Antialiasing
        // Reserved TraceReflectionRay = (1 << 3) for transparent SSR or RTR
        WaterSurface = (1 << 4), // Reserved for water surface usage
        AfterOpaqueReservedBits = 0x38,        // Reserved for future usage

        // --- Following are user bits, we don't touch them inside HDRP and is up to the user to handle them ---
        UserBit0 = (1 << 6),
        UserBit1 = (1 << 7),

        HDRPReservedBits = 255 & ~(UserBit0 | UserBit1),
    };

}

