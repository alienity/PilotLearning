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

    inline uint32_t CreateHandleIndexFunc(RHI::RenderGraphRegistry* registry, RHI::RgResourceHandle InHandle)
    {
        RHI::D3D12Texture* TempTex = registry->GetD3D12Texture(InHandle);
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
        srvDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MostDetailedMip = 0;
        srvDesc.Texture2D.MipLevels = 1;
        srvDesc.Texture2D.PlaneSlice = 0;
        srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
        std::shared_ptr<RHI::D3D12ShaderResourceView> InTexSRV = TempTex->CreateSRV(srvDesc);
        uint32_t InTexSRVIndex = InTexSRV->GetIndex();
        return InTexSRVIndex;
    };

    enum class LightVolumeType
    {
        Cone,
        Sphere,
        Box,
        Count
    };

    enum LightCategory
    {
        Punctual,
        Area,
        Env,
        Decal,
        Count
    };

    enum class LightFeatureFlags
    {
        // Light bit mask must match LightDefinitions.s_LightFeatureMaskFlags value
        Punctual = 1 << 12,
        Area = 1 << 13,
        Directional = 1 << 14,
        Env = 1 << 15,
        Sky = 1 << 16,
        SSRefraction = 1 << 17,
        SSReflection = 1 << 18,
        // If adding more light be sure to not overflow LightDefinitions.s_LightFeatureMaskFlags
    };

    struct LightDefinitions
    {
        float s_ViewportScaleZ = 1.0f;
        int s_UseLeftHandCameraSpace = 1;

        int s_TileSizeFptl = 16;
        int s_TileSizeClustered = 32;
        int s_TileSizeBigTile = 64;

        // Tile indexing constants for indirect dispatch deferred pass : [2 bits for eye index | 15 bits for tileX | 15 bits for tileY]
        int s_TileIndexMask = 0x7FFF;
        int s_TileIndexShiftX = 0;
        int s_TileIndexShiftY = 15;
        int s_TileIndexShiftEye = 30;

        // feature variants
        int s_NumFeatureVariants = 29;

        // Following define the maximum number of bits use in each feature category.
        glm::uint s_LightFeatureMaskFlags = 0xFFF000;
        glm::uint s_LightFeatureMaskFlagsOpaque = 0xFFF000 & ~((glm::uint)LightFeatureFlags::SSRefraction); // Opaque don't support screen space refraction
        glm::uint s_LightFeatureMaskFlagsTransparent = 0xFFF000 & ~((glm::uint)LightFeatureFlags::SSReflection); // Transparent don't support screen space reflection
        glm::uint s_MaterialFeatureMaskFlags = 0x000FFF;   // don't use all bits just to be safe from signed and/or float conversions :/

        // Screen space shadow flags
        glm::uint s_RayTracedScreenSpaceShadowFlag = 0x1000;
        glm::uint s_ScreenSpaceColorShadowFlag = 0x100;
        glm::uint s_InvalidScreenSpaceShadow = 0xff;
        glm::uint s_ScreenSpaceShadowIndexMask = 0xff;

        //Contact shadow bit definitions
        int s_ContactShadowFadeBits = 8;
        int s_ContactShadowMaskBits = 32 - s_ContactShadowFadeBits;
        int s_ContactShadowFadeMask = (1 << s_ContactShadowFadeBits) - 1;
        int s_ContactShadowMaskMask = (1 << s_ContactShadowMaskBits) - 1;

    };

}

