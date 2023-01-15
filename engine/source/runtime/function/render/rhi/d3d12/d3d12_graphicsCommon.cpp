#include "d3d12_graphicsCommon.h"

#include "shaders/CompiledShaders/GenerateMipsGammaCS.h"
#include "shaders/CompiledShaders/GenerateMipsGammaOddCS.h"
#include "shaders/CompiledShaders/GenerateMipsGammaOddXCS.h"
#include "shaders/CompiledShaders/GenerateMipsGammaOddYCS.h"
#include "shaders/CompiledShaders/GenerateMipsLinearCS.h"
#include "shaders/CompiledShaders/GenerateMipsLinearOddCS.h"
#include "shaders/CompiledShaders/GenerateMipsLinearOddXCS.h"
#include "shaders/CompiledShaders/GenerateMipsLinearOddYCS.h"

#include "shaders/CompiledShaders/DownsampleDepthPS.h"
#include "shaders/CompiledShaders/ScreenQuadCommonVS.h"

namespace RHI
{
    D3D12Device* p_Parent;

    std::shared_ptr<D3D12Texture> PDefaultTextures[kNumDefaultTextures];
    D3D12Texture* GetDefaultTexture(eDefaultTexture texID)
    {
        ASSERT(texID < kNumDefaultTextures);
        return PDefaultTextures[texID].get();
    }

    SamplerDesc SamplerLinearWrapDesc;
    SamplerDesc SamplerAnisoWrapDesc;
    SamplerDesc SamplerShadowDesc;
    SamplerDesc SamplerLinearClampDesc;
    SamplerDesc SamplerVolumeWrapDesc;
    SamplerDesc SamplerPointClampDesc;
    SamplerDesc SamplerPointBorderDesc;
    SamplerDesc SamplerLinearBorderDesc;

    D3D12_CPU_DESCRIPTOR_HANDLE SamplerLinearWrap;
    D3D12_CPU_DESCRIPTOR_HANDLE SamplerAnisoWrap;
    D3D12_CPU_DESCRIPTOR_HANDLE SamplerShadow;
    D3D12_CPU_DESCRIPTOR_HANDLE SamplerLinearClamp;
    D3D12_CPU_DESCRIPTOR_HANDLE SamplerVolumeWrap;
    D3D12_CPU_DESCRIPTOR_HANDLE SamplerPointClamp;
    D3D12_CPU_DESCRIPTOR_HANDLE SamplerPointBorder;
    D3D12_CPU_DESCRIPTOR_HANDLE SamplerLinearBorder;

    D3D12_RASTERIZER_DESC RasterizerDefault; // Counter-clockwise
    D3D12_RASTERIZER_DESC RasterizerDefaultMsaa;
    D3D12_RASTERIZER_DESC RasterizerDefaultCw; // Clockwise winding
    D3D12_RASTERIZER_DESC RasterizerDefaultCwMsaa;
    D3D12_RASTERIZER_DESC RasterizerTwoSided;
    D3D12_RASTERIZER_DESC RasterizerTwoSidedMsaa;
    D3D12_RASTERIZER_DESC RasterizerShadow;
    D3D12_RASTERIZER_DESC RasterizerShadowCW;
    D3D12_RASTERIZER_DESC RasterizerShadowTwoSided;

    D3D12_BLEND_DESC BlendNoColorWrite;
    D3D12_BLEND_DESC BlendDisable;
    D3D12_BLEND_DESC BlendPreMultiplied;
    D3D12_BLEND_DESC BlendTraditional;
    D3D12_BLEND_DESC BlendAdditive;
    D3D12_BLEND_DESC BlendTraditionalAdditive;

    D3D12_DEPTH_STENCIL_DESC DepthStateDisabled;
    D3D12_DEPTH_STENCIL_DESC DepthStateReadWrite;
    D3D12_DEPTH_STENCIL_DESC DepthStateReadOnly;
    D3D12_DEPTH_STENCIL_DESC DepthStateReadOnlyReversed;
    D3D12_DEPTH_STENCIL_DESC DepthStateTestEqual;

    D3D12CommandSignature* pDispatchIndirectCommandSignature;
    D3D12CommandSignature* pDrawIndirectCommandSignature;

    D3D12RootSignature* g_pCommonRS;

    D3D12PipelineState* g_pGenerateMipsLinearPSO[4];
    
    D3D12PipelineState* g_pGenerateMipsGammaPSO[4];
    
    D3D12PipelineState* g_pDownsampleDepthPSO;




    
    void InitializeCommonState(D3D12Device* pParent)
    {
        p_Parent = pParent;

        SamplerLinearWrapDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        SamplerLinearWrap            = SamplerLinearWrapDesc.CreateDescriptor(p_Parent->GetLinkedDevice());

        SamplerAnisoWrapDesc.MaxAnisotropy = 4;
        SamplerAnisoWrap                   = SamplerAnisoWrapDesc.CreateDescriptor(p_Parent->GetLinkedDevice());

        SamplerShadowDesc.Filter         = D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
        SamplerShadowDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_GREATER_EQUAL;
        SamplerShadowDesc.SetTextureAddressMode(D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
        SamplerShadow = SamplerShadowDesc.CreateDescriptor(p_Parent->GetLinkedDevice());

        SamplerLinearClampDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        SamplerLinearClampDesc.SetTextureAddressMode(D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
        SamplerLinearClamp = SamplerLinearClampDesc.CreateDescriptor(p_Parent->GetLinkedDevice());

        SamplerVolumeWrapDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
        SamplerVolumeWrap            = SamplerVolumeWrapDesc.CreateDescriptor(p_Parent->GetLinkedDevice());

        SamplerPointClampDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
        SamplerPointClampDesc.SetTextureAddressMode(D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
        SamplerPointClamp = SamplerPointClampDesc.CreateDescriptor(p_Parent->GetLinkedDevice());

        SamplerLinearBorderDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        SamplerLinearBorderDesc.SetTextureAddressMode(D3D12_TEXTURE_ADDRESS_MODE_BORDER);
        SamplerLinearBorderDesc.SetBorderColor(Pilot::Color(0.0f, 0.0f, 0.0f, 0.0f));
        SamplerLinearBorder = SamplerLinearBorderDesc.CreateDescriptor(p_Parent->GetLinkedDevice());

        SamplerPointBorderDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
        SamplerPointBorderDesc.SetTextureAddressMode(D3D12_TEXTURE_ADDRESS_MODE_BORDER);
        SamplerPointBorderDesc.SetBorderColor(Pilot::Color(0.0f, 0.0f, 0.0f, 0.0f));
        SamplerPointBorder = SamplerPointBorderDesc.CreateDescriptor(p_Parent->GetLinkedDevice());

        // Default rasterizer states
        RasterizerDefault.FillMode              = D3D12_FILL_MODE_SOLID;
        RasterizerDefault.CullMode              = D3D12_CULL_MODE_BACK;
        RasterizerDefault.FrontCounterClockwise = TRUE;
        RasterizerDefault.DepthBias             = D3D12_DEFAULT_DEPTH_BIAS;
        RasterizerDefault.DepthBiasClamp        = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
        RasterizerDefault.SlopeScaledDepthBias  = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
        RasterizerDefault.DepthClipEnable       = TRUE;
        RasterizerDefault.MultisampleEnable     = FALSE;
        RasterizerDefault.AntialiasedLineEnable = FALSE;
        RasterizerDefault.ForcedSampleCount     = 0;
        RasterizerDefault.ConservativeRaster    = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

RasterizerDefaultMsaa                   = RasterizerDefault;
        RasterizerDefaultMsaa.MultisampleEnable = TRUE;

        RasterizerDefaultCw                       = RasterizerDefault;
        RasterizerDefaultCw.FrontCounterClockwise = FALSE;

        RasterizerDefaultCwMsaa                   = RasterizerDefaultCw;
        RasterizerDefaultCwMsaa.MultisampleEnable = TRUE;

        RasterizerTwoSided          = RasterizerDefault;
        RasterizerTwoSided.CullMode = D3D12_CULL_MODE_NONE;

        RasterizerTwoSidedMsaa                   = RasterizerTwoSided;
        RasterizerTwoSidedMsaa.MultisampleEnable = TRUE;

        // Shadows need their own rasterizer state so we can reverse the winding of faces
        RasterizerShadow = RasterizerDefault;
        // RasterizerShadow.CullMode = D3D12_CULL_FRONT;  // Hacked here rather than fixing the content
        RasterizerShadow.SlopeScaledDepthBias = -1.5f;
        RasterizerShadow.DepthBias            = -100;

        RasterizerShadowTwoSided          = RasterizerShadow;
        RasterizerShadowTwoSided.CullMode = D3D12_CULL_MODE_NONE;

        RasterizerShadowCW                       = RasterizerShadow;
        RasterizerShadowCW.FrontCounterClockwise = FALSE;

        DepthStateDisabled.DepthEnable                  = FALSE;
        DepthStateDisabled.DepthWriteMask               = D3D12_DEPTH_WRITE_MASK_ZERO;
        DepthStateDisabled.DepthFunc                    = D3D12_COMPARISON_FUNC_ALWAYS;
        DepthStateDisabled.StencilEnable                = FALSE;
        DepthStateDisabled.StencilReadMask              = D3D12_DEFAULT_STENCIL_READ_MASK;
        DepthStateDisabled.StencilWriteMask             = D3D12_DEFAULT_STENCIL_WRITE_MASK;
        DepthStateDisabled.FrontFace.StencilFunc        = D3D12_COMPARISON_FUNC_ALWAYS;
        DepthStateDisabled.FrontFace.StencilPassOp      = D3D12_STENCIL_OP_KEEP;
        DepthStateDisabled.FrontFace.StencilFailOp      = D3D12_STENCIL_OP_KEEP;
        DepthStateDisabled.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
        DepthStateDisabled.BackFace                     = DepthStateDisabled.FrontFace;

        DepthStateReadWrite                = DepthStateDisabled;
        DepthStateReadWrite.DepthEnable    = TRUE;
        DepthStateReadWrite.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
        DepthStateReadWrite.DepthFunc      = D3D12_COMPARISON_FUNC_GREATER_EQUAL;

        DepthStateReadOnly                = DepthStateReadWrite;
        DepthStateReadOnly.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;

        DepthStateReadOnlyReversed           = DepthStateReadOnly;
        DepthStateReadOnlyReversed.DepthFunc = D3D12_COMPARISON_FUNC_LESS;

        DepthStateTestEqual           = DepthStateReadOnly;
        DepthStateTestEqual.DepthFunc = D3D12_COMPARISON_FUNC_EQUAL;

        D3D12_BLEND_DESC alphaBlend                      = {};
        alphaBlend.IndependentBlendEnable                = FALSE;
        alphaBlend.RenderTarget[0].BlendEnable           = FALSE;
        alphaBlend.RenderTarget[0].SrcBlend              = D3D12_BLEND_SRC_ALPHA;
        alphaBlend.RenderTarget[0].DestBlend             = D3D12_BLEND_INV_SRC_ALPHA;
        alphaBlend.RenderTarget[0].BlendOp               = D3D12_BLEND_OP_ADD;
        alphaBlend.RenderTarget[0].SrcBlendAlpha         = D3D12_BLEND_ONE;
        alphaBlend.RenderTarget[0].DestBlendAlpha        = D3D12_BLEND_INV_SRC_ALPHA;
        alphaBlend.RenderTarget[0].BlendOpAlpha          = D3D12_BLEND_OP_ADD;
        alphaBlend.RenderTarget[0].RenderTargetWriteMask = 0;
        BlendNoColorWrite                                = alphaBlend;

        alphaBlend.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
        BlendDisable                                     = alphaBlend;

        alphaBlend.RenderTarget[0].BlendEnable = TRUE;
        BlendTraditional                       = alphaBlend;

        alphaBlend.RenderTarget[0].SrcBlend = D3D12_BLEND_ONE;
        BlendPreMultiplied                  = alphaBlend;

        alphaBlend.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
        BlendAdditive                        = alphaBlend;

        alphaBlend.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
        BlendTraditionalAdditive            = alphaBlend;

        RHI::CommandSignatureDesc mDispatchIndirectDesc(1);
        mDispatchIndirectDesc.AddDispatch();
        pDispatchIndirectCommandSignature = new D3D12CommandSignature(pParent, mDispatchIndirectDesc);

        RHI::CommandSignatureDesc mDrawIndirectDesc(1);
        mDrawIndirectDesc.AddDraw();
        pDrawIndirectCommandSignature = new D3D12CommandSignature(pParent, mDrawIndirectDesc);

        RHI::RootSignatureDesc mCommonRSDesc;
        mCommonRSDesc.Add32BitConstants<0, 0>(4);
        mCommonRSDesc.AddDescriptorTable(RHI::D3D12DescriptorTable(1).AddSRVRange<1, 0>(10, D3D12_DESCRIPTOR_RANGE_FLAG_NONE, 0));
        mCommonRSDesc.AddDescriptorTable(RHI::D3D12DescriptorTable(1).AddUAVRange<2, 0>(10, D3D12_DESCRIPTOR_RANGE_FLAG_NONE, 0));
        mCommonRSDesc.AddConstantBufferView<1, 0>();
        mCommonRSDesc.AddStaticSampler<0, 0>(SamplerLinearClampDesc);
        mCommonRSDesc.AddStaticSampler<1, 0>(SamplerPointBorderDesc);
        mCommonRSDesc.AddStaticSampler<2, 0>(SamplerLinearBorderDesc);

        g_pCommonRS = new D3D12RootSignature(pParent, mCommonRSDesc);
        
        for (size_t i = 0; i < 4; i++)
        {
            PipelineStateStreamDesc genMipPSODesc;

            g_pGenerateMipsLinearPSO[i] = new D3D12PipelineState(pParent, L"Generate Mips Linear CS", );
        }

    }

    void DestroyCommonState(void)
    {
        delete pDispatchIndirectCommandSignature;
        delete pDrawIndirectCommandSignature;

        delete g_pCommonRS;

        delete[] g_pGenerateMipsLinearPSO;
        delete[] g_pGenerateMipsGammaPSO;
        delete g_pDownsampleDepthPSO;


    }

}

