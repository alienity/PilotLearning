#pragma once
#include "d3d12_core.h"
#include "d3d12_linkedDevice.h"
#include "d3d12_resource.h"

namespace RHI
{
    class SamplerDesc;
    class D3D12CommandSignature;
    class D3D12RootSignature;
    class D3D12PipelineState;

    void InitializeCommonState(D3D12LinkedDevice* pParent);
    void InitializeCommonResource(D3D12LinkedDevice* pParent);
    void DestroyCommonState();
    void DestroyCommonResource();

    extern SamplerDesc SamplerLinearWrapDesc;
    extern SamplerDesc SamplerAnisoWrapDesc;
    extern SamplerDesc SamplerShadowDesc;
    extern SamplerDesc SamplerLinearClampDesc;
    extern SamplerDesc SamplerVolumeWrapDesc;
    extern SamplerDesc SamplerPointClampDesc;
    extern SamplerDesc SamplerPointBorderDesc;
    extern SamplerDesc SamplerLinearBorderDesc;

    extern D3D12_CPU_DESCRIPTOR_HANDLE SamplerLinearWrap;
    extern D3D12_CPU_DESCRIPTOR_HANDLE SamplerAnisoWrap;
    extern D3D12_CPU_DESCRIPTOR_HANDLE SamplerShadow;
    extern D3D12_CPU_DESCRIPTOR_HANDLE SamplerLinearClamp;
    extern D3D12_CPU_DESCRIPTOR_HANDLE SamplerVolumeWrap;
    extern D3D12_CPU_DESCRIPTOR_HANDLE SamplerPointClamp;
    extern D3D12_CPU_DESCRIPTOR_HANDLE SamplerPointBorder;
    extern D3D12_CPU_DESCRIPTOR_HANDLE SamplerLinearBorder;

    extern D3D12_RASTERIZER_DESC RasterizerDefault;
    extern D3D12_RASTERIZER_DESC RasterizerDefaultMsaa;
    extern D3D12_RASTERIZER_DESC RasterizerDefaultCw;
    extern D3D12_RASTERIZER_DESC RasterizerDefaultCwMsaa;
    extern D3D12_RASTERIZER_DESC RasterizerTwoSided;
    extern D3D12_RASTERIZER_DESC RasterizerTwoSidedMsaa;
    extern D3D12_RASTERIZER_DESC RasterizerShadow;
    extern D3D12_RASTERIZER_DESC RasterizerShadowCW;
    extern D3D12_RASTERIZER_DESC RasterizerShadowTwoSided;

    extern D3D12_BLEND_DESC BlendNoColorWrite;        // XXX
    extern D3D12_BLEND_DESC BlendDisable;             // 1, 0
    extern D3D12_BLEND_DESC BlendPreMultiplied;       // 1, 1-SrcA
    extern D3D12_BLEND_DESC BlendTraditional;         // SrcA, 1-SrcA
    extern D3D12_BLEND_DESC BlendAdditive;            // 1, 1
    extern D3D12_BLEND_DESC BlendTraditionalAdditive; // SrcA, 1

    extern D3D12_DEPTH_STENCIL_DESC DepthStateDisabled;
    extern D3D12_DEPTH_STENCIL_DESC DepthStateReadWrite;
    extern D3D12_DEPTH_STENCIL_DESC DepthStateReadOnly;
    extern D3D12_DEPTH_STENCIL_DESC DepthStateReadOnlyReversed;
    extern D3D12_DEPTH_STENCIL_DESC DepthStateTestEqual;

    extern D3D12CommandSignature* pDispatchIndirectCommandSignature;
    extern D3D12CommandSignature* pDrawIndirectCommandSignature;

    enum eDefaultTexture
    {
        kMagenta2D, // Useful for indicating missing textures
        kBlackOpaque2D,
        kBlackTransparent2D,
        kWhiteOpaque2D,
        kWhiteTransparent2D,
        kDefaultNormalMap,
        kBlackCubeMap,

        kNumDefaultTextures
    };
    D3D12Texture* GetDefaultTexture(eDefaultTexture texID);

    extern D3D12RootSignature* g_pCommonRS;
    extern D3D12PipelineState* g_pGenerateMipsLinearPSO[4];
    extern D3D12PipelineState* g_pGenerateMipsGammaPSO[4];
    extern D3D12PipelineState* g_pDownsampleDepthPSO;
}