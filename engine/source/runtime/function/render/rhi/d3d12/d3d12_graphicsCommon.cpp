#include "d3d12_graphicsCommon.h"
#include "d3d12_device.h"
#include "d3d12_linkedDevice.h"
#include "d3d12_samplerManager.h"
#include "d3d12_commandSignature.h"
#include "d3d12_rootSignature.h"
#include "d3d12_pipelineState.h"

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

    D3D12RootSignature* g_pBindlessRS;
    D3D12RootSignature* g_pCommonRS;

    D3D12PipelineState* g_pGenerateMipsLinearPSO[4];
    D3D12PipelineState* g_pGenerateMipsGammaPSO[4];
    D3D12PipelineState* g_pDownsampleDepthPSO;




    
    void InitializeCommonState(D3D12LinkedDevice* pParent)
    {
        SamplerLinearWrapDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        SamplerLinearWrap            = SamplerLinearWrapDesc.CreateDescriptor(pParent);

        SamplerAnisoWrapDesc.MaxAnisotropy = 4;
        SamplerAnisoWrap                   = SamplerAnisoWrapDesc.CreateDescriptor(pParent);

        SamplerShadowDesc.Filter         = D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
        SamplerShadowDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_GREATER_EQUAL;
        SamplerShadowDesc.SetTextureAddressMode(D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
        SamplerShadow = SamplerShadowDesc.CreateDescriptor(pParent);

        SamplerLinearClampDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        SamplerLinearClampDesc.SetTextureAddressMode(D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
        SamplerLinearClamp = SamplerLinearClampDesc.CreateDescriptor(pParent);

        SamplerVolumeWrapDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
        SamplerVolumeWrap            = SamplerVolumeWrapDesc.CreateDescriptor(pParent);

        SamplerPointClampDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
        SamplerPointClampDesc.SetTextureAddressMode(D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
        SamplerPointClamp = SamplerPointClampDesc.CreateDescriptor(pParent);

        SamplerLinearBorderDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        SamplerLinearBorderDesc.SetTextureAddressMode(D3D12_TEXTURE_ADDRESS_MODE_BORDER);
        SamplerLinearBorderDesc.SetBorderColor(Pilot::Color(0.0f, 0.0f, 0.0f, 0.0f));
        SamplerLinearBorder = SamplerLinearBorderDesc.CreateDescriptor(pParent);

        SamplerPointBorderDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
        SamplerPointBorderDesc.SetTextureAddressMode(D3D12_TEXTURE_ADDRESS_MODE_BORDER);
        SamplerPointBorderDesc.SetBorderColor(Pilot::Color(0.0f, 0.0f, 0.0f, 0.0f));
        SamplerPointBorder = SamplerPointBorderDesc.CreateDescriptor(pParent);

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
        pDispatchIndirectCommandSignature = new D3D12CommandSignature(pParent->GetParentDevice(), mDispatchIndirectDesc);

        RHI::CommandSignatureDesc mDrawIndirectDesc(1);
        mDrawIndirectDesc.AddDraw();
        pDrawIndirectCommandSignature = new D3D12CommandSignature(pParent->GetParentDevice(), mDrawIndirectDesc);
        
        CD3DX12_STATIC_SAMPLER_DESC _s0Sampler = CD3DX12_STATIC_SAMPLER_DESC();
        _s0Sampler.Filter   = D3D12_FILTER_MIN_MAG_MIP_POINT;
        _s0Sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        _s0Sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        _s0Sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;

        CD3DX12_STATIC_SAMPLER_DESC _s1Sampler = CD3DX12_STATIC_SAMPLER_DESC();
        _s1Sampler.Filter   = D3D12_FILTER_MIN_MAG_MIP_POINT;
        _s1Sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        _s1Sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        _s1Sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;

        CD3DX12_STATIC_SAMPLER_DESC _s2Sampler = CD3DX12_STATIC_SAMPLER_DESC();
        _s2Sampler.Filter   = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        _s2Sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        _s2Sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        _s2Sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;

        CD3DX12_STATIC_SAMPLER_DESC _s3Sampler = CD3DX12_STATIC_SAMPLER_DESC();
        _s3Sampler.Filter   = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        _s3Sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        _s3Sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        _s3Sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;

        CD3DX12_STATIC_SAMPLER_DESC _s4Sampler = CD3DX12_STATIC_SAMPLER_DESC();
        _s4Sampler.Filter   = D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT;
        _s4Sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        _s4Sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        _s4Sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;

        CD3DX12_STATIC_SAMPLER_DESC _s5Sampler = CD3DX12_STATIC_SAMPLER_DESC();
        _s5Sampler.Filter   = D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT;
        _s5Sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        _s5Sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        _s5Sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;

        CD3DX12_STATIC_SAMPLER_DESC _s6Sampler = CD3DX12_STATIC_SAMPLER_DESC();
        _s6Sampler.Filter   = D3D12_FILTER_MIN_MAG_POINT_MIP_LINEAR;
        _s6Sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        _s6Sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        _s6Sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;

        CD3DX12_STATIC_SAMPLER_DESC _s7Sampler = CD3DX12_STATIC_SAMPLER_DESC();
        _s7Sampler.Filter   = D3D12_FILTER_MIN_MAG_POINT_MIP_LINEAR;
        _s7Sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        _s7Sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        _s7Sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;

        CD3DX12_STATIC_SAMPLER_DESC _s8Sampler = CD3DX12_STATIC_SAMPLER_DESC();
        _s8Sampler.Filter        = D3D12_FILTER_ANISOTROPIC;
        _s8Sampler.MaxAnisotropy = 16;

        CD3DX12_STATIC_SAMPLER_DESC _s9Sampler = CD3DX12_STATIC_SAMPLER_DESC();
        _s9Sampler.Filter      = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        _s9Sampler.AddressU    = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
        _s9Sampler.AddressV    = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
        _s9Sampler.AddressW    = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
        _s9Sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;

        RHI::RootSignatureDesc mBindlessRSDesc;
        mBindlessRSDesc.AllowInputLayout();
        mBindlessRSDesc.AllowResourceDescriptorHeapIndexing();
        mBindlessRSDesc.AllowSampleDescriptorHeapIndexing();
        mBindlessRSDesc.Add32BitConstants<0, 0>(64);
        mBindlessRSDesc.AddStaticSampler<0, 0>(_s0Sampler);
        mBindlessRSDesc.AddStaticSampler<1, 0>(_s1Sampler);
        mBindlessRSDesc.AddStaticSampler<2, 0>(_s2Sampler);
        mBindlessRSDesc.AddStaticSampler<3, 0>(_s3Sampler);
        mBindlessRSDesc.AddStaticSampler<4, 0>(_s4Sampler);
        mBindlessRSDesc.AddStaticSampler<5, 0>(_s5Sampler);
        mBindlessRSDesc.AddStaticSampler<6, 0>(_s6Sampler);
        mBindlessRSDesc.AddStaticSampler<7, 0>(_s7Sampler);
        mBindlessRSDesc.AddStaticSampler<8, 0>(_s8Sampler);
        mBindlessRSDesc.AddStaticSampler<9, 0>(_s9Sampler);

        g_pBindlessRS = new D3D12RootSignature(pParent->GetParentDevice(), mBindlessRSDesc);

        RHI::RootSignatureDesc mCommonRSDesc;
        mCommonRSDesc.Add32BitConstants<0, 0>(4);
        mCommonRSDesc.AddDescriptorTable(RHI::D3D12DescriptorTable(1).AddSRVRange<0, 0>(10, D3D12_DESCRIPTOR_RANGE_FLAG_NONE, 0));
        mCommonRSDesc.AddDescriptorTable(RHI::D3D12DescriptorTable(1).AddUAVRange<0, 0>(10, D3D12_DESCRIPTOR_RANGE_FLAG_NONE, 0));
        mCommonRSDesc.AddConstantBufferView<1, 0>();
        mCommonRSDesc.AddStaticSampler<0, 0>(SamplerLinearClampDesc);
        mCommonRSDesc.AddStaticSampler<1, 0>(SamplerPointBorderDesc);
        mCommonRSDesc.AddStaticSampler<2, 0>(SamplerLinearBorderDesc);

        g_pCommonRS = new D3D12RootSignature(pParent->GetParentDevice(), mCommonRSDesc);
        
#define CreatePSO(ObjName, Index, PSOName, ShaderByteCode) \
    D3D12_COMPUTE_PIPELINE_STATE_DESC ObjName##Index##PSODesc = {};\
    ObjName##Index##PSODesc.pRootSignature = g_pCommonRS->GetApiHandle();\
    ObjName##Index##PSODesc.CS             = D3D12_SHADER_BYTECODE {ShaderByteCode, sizeof(ShaderByteCode)};\
    ObjName##Index##PSODesc.NodeMask       = pParent->GetParentDevice()->GetAllNodeMask();\
    ObjName##Index##PSODesc.CachedPSO      = D3D12_CACHED_PIPELINE_STATE();\
    ObjName##Index##PSODesc.Flags          = D3D12_PIPELINE_STATE_FLAG_NONE;\
    ObjName[Index] = new D3D12PipelineState(pParent->GetParentDevice(), PSOName, ObjName##Index##PSODesc);

        CreatePSO(g_pGenerateMipsLinearPSO, 0, L"Generate Mips Linear CS", g_pGenerateMipsLinearCS)
        CreatePSO(g_pGenerateMipsLinearPSO, 1, L"Generate Mips Linear Odd X CS", g_pGenerateMipsLinearOddXCS)
        CreatePSO(g_pGenerateMipsLinearPSO, 2, L"Generate Mips Linear Odd Y CS", g_pGenerateMipsLinearOddYCS)
        CreatePSO(g_pGenerateMipsLinearPSO, 3, L"Generate Mips Linear Odd CS", g_pGenerateMipsLinearOddCS)
        CreatePSO(g_pGenerateMipsGammaPSO, 0, L"Generate Mips Gamma CS", g_pGenerateMipsGammaCS)
        CreatePSO(g_pGenerateMipsGammaPSO, 1, L"Generate Mips Gamma Odd X CS", g_pGenerateMipsGammaOddXCS)
        CreatePSO(g_pGenerateMipsGammaPSO, 2, L"Generate Mips Gamma Odd Y CS", g_pGenerateMipsGammaOddYCS)
        CreatePSO(g_pGenerateMipsGammaPSO, 3, L"Generate Mips Gamma Odd CS", g_pGenerateMipsGammaOddCS)

        D3D12_GRAPHICS_PIPELINE_STATE_DESC mDownsampleDepthDesc = {};
        mDownsampleDepthDesc.pRootSignature                 = g_pCommonRS->GetApiHandle();
        mDownsampleDepthDesc.RasterizerState                = RasterizerTwoSided;
        mDownsampleDepthDesc.BlendState                     = BlendDisable;
        mDownsampleDepthDesc.DepthStencilState              = DepthStateReadWrite;
        mDownsampleDepthDesc.SampleMask                     = 0xFFFFFFFF;
        mDownsampleDepthDesc.InputLayout.NumElements        = 0;
        mDownsampleDepthDesc.InputLayout.pInputElementDescs = nullptr;
        mDownsampleDepthDesc.PrimitiveTopologyType          = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        mDownsampleDepthDesc.VS = D3D12_SHADER_BYTECODE {g_pScreenQuadCommonVS, sizeof(g_pScreenQuadCommonVS)};
        mDownsampleDepthDesc.PS = D3D12_SHADER_BYTECODE {g_pDownsampleDepthPS, sizeof(g_pDownsampleDepthPS)};
        mDownsampleDepthDesc.NumRenderTargets   = 0;
        mDownsampleDepthDesc.DSVFormat          = DXGI_FORMAT_D32_FLOAT;
        mDownsampleDepthDesc.SampleDesc.Count   = 1;
        mDownsampleDepthDesc.SampleDesc.Quality = 0;
        mDownsampleDepthDesc.NodeMask           = pParent->GetParentDevice()->GetAllNodeMask();
        mDownsampleDepthDesc.CachedPSO          = D3D12_CACHED_PIPELINE_STATE();
        mDownsampleDepthDesc.Flags              = D3D12_PIPELINE_STATE_FLAG_NONE;
        g_pDownsampleDepthPSO = new D3D12PipelineState(pParent->GetParentDevice(), L"DownsampleDepth PSO", mDownsampleDepthDesc);
    }

    std::shared_ptr<D3D12Texture> PDefaultTextures[kNumDefaultTextures];
    D3D12Texture*                 GetDefaultTexture(eDefaultTexture texID)
    {
        ASSERT(texID < kNumDefaultTextures);
        return PDefaultTextures[texID].get();
    }

    void InitializeCommonResource(D3D12LinkedDevice* pParent)
    {
#define CreateSubData(data) D3D12_SUBRESOURCE_DATA {&data, 4, 4}

#define Create2DTex(name, data) \
    D3D12Texture::Create2D(pParent, \
                           1, \
                           1, \
                           1, \
                           DXGI_FORMAT_R8G8B8A8_UNORM, \
                           RHISurfaceCreateFlags::RHISurfaceCreateFlagNone, \
                           1, \
                           name, \
                           D3D12_RESOURCE_STATE_COMMON, \
                           std::nullopt, \
                           CreateSubData(data))

        #define Create2DCubeTex(name, subdata) \
    D3D12Texture::CreateCubeMap(pParent, \
                                1, \
                                1, \
                                1, \
                                DXGI_FORMAT_R8G8B8A8_UNORM, \
                                RHISurfaceCreateFlags::RHISurfaceCreateFlagNone, \
                                1, \
                                name, \
                                D3D12_RESOURCE_STATE_COMMON, \
                                std::nullopt, \
                                std::vector<D3D12_SUBRESOURCE_DATA> {CreateSubData(subdata), \
                                                                     CreateSubData(subdata), \
                                                                     CreateSubData(subdata), \
                                                                     CreateSubData(subdata), \
                                                                     CreateSubData(subdata), \
                                                                     CreateSubData(subdata)})

        // clang-format off
        uint32_t MagentaPixel = 0xFFFF00FF;
        PDefaultTextures[eDefaultTexture::kMagenta2D] = Create2DTex(L"DefaultMagenta2D", MagentaPixel);
        uint32_t BlackOpaqueTexel = 0xFF000000;
        PDefaultTextures[eDefaultTexture::kBlackOpaque2D] = Create2DTex(L"DefaultBlackOpaque2D", BlackOpaqueTexel);
        uint32_t BlackTransparentTexel = 0x00000000;
        PDefaultTextures[eDefaultTexture::kBlackTransparent2D] = Create2DTex(L"DefaultBlackTransparent2D", BlackTransparentTexel);
        uint32_t WhiteOpaqueTexel = 0xFFFFFFFF;
        PDefaultTextures[eDefaultTexture::kWhiteOpaque2D] = Create2DTex(L"DefaultWhiteOpaque2D", WhiteOpaqueTexel);
        uint32_t WhiteTransparentTexel = 0x00FFFFFF;
        PDefaultTextures[eDefaultTexture::kWhiteTransparent2D] = Create2DTex(L"DefaultWhiteTransparent2D", WhiteOpaqueTexel);
        uint32_t FlatNormalTexel = 0x00FFFFFF;
        PDefaultTextures[eDefaultTexture::kDefaultNormalMap] = Create2DTex(L"DefaultFlatNormal2D", FlatNormalTexel);
        uint32_t BlackCubeTexels = 0x00000000;
        PDefaultTextures[eDefaultTexture::kBlackCubeMap] = Create2DCubeTex(L"BlackCubeTex", BlackCubeTexels);
        // clang-format on

    }

    void DestroyCommonState(void)
    {
#define SAFERELEASE(a) delete a; a = nullptr;

        SAFERELEASE(pDispatchIndirectCommandSignature)
        SAFERELEASE(pDrawIndirectCommandSignature)

        SAFERELEASE(g_pCommonRS)

        SAFERELEASE(g_pGenerateMipsLinearPSO[0])
        SAFERELEASE(g_pGenerateMipsLinearPSO[1])
        SAFERELEASE(g_pGenerateMipsLinearPSO[2])
        SAFERELEASE(g_pGenerateMipsLinearPSO[3])
            
        SAFERELEASE(g_pGenerateMipsGammaPSO[0])
        SAFERELEASE(g_pGenerateMipsGammaPSO[1])
        SAFERELEASE(g_pGenerateMipsGammaPSO[2])
        SAFERELEASE(g_pGenerateMipsGammaPSO[3])

        SAFERELEASE(g_pDownsampleDepthPSO)
    }

    void DestroyCommonResource()
    {
        PDefaultTextures[eDefaultTexture::kMagenta2D]          = nullptr;
        PDefaultTextures[eDefaultTexture::kBlackOpaque2D]      = nullptr;
        PDefaultTextures[eDefaultTexture::kBlackTransparent2D] = nullptr;
        PDefaultTextures[eDefaultTexture::kWhiteOpaque2D]      = nullptr;
        PDefaultTextures[eDefaultTexture::kWhiteTransparent2D] = nullptr;
        PDefaultTextures[eDefaultTexture::kDefaultNormalMap]   = nullptr;
        PDefaultTextures[eDefaultTexture::kBlackCubeMap]       = nullptr;
    }

}

