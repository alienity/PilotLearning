#include "runtime/function/render/renderer/ssr_pass.h"
#include "runtime/resource/config_manager/config_manager.h"
#include "runtime/function/render/rhi/rhi_core.h"
#include "runtime/function/render/jitter_helper.h"
#include "runtime/function/render/render_helper.h"

#include <cassert>

namespace MoYu
{
#define RegGetBuf(h) registry->GetD3D12Buffer(h)
#define RegGetBufCounter(h) registry->GetD3D12Buffer(h)->GetCounterBuffer().get()
#define RegGetTex(h) registry->GetD3D12Texture(h)
#define RegGetBufDefCBVIdx(h) registry->GetD3D12Buffer(h)->GetDefaultCBV()->GetIndex()
#define RegGetBufDefSRVIdx(h) registry->GetD3D12Buffer(h)->GetDefaultSRV()->GetIndex()
#define RegGetBufDefUAVIdx(h) registry->GetD3D12Buffer(h)->GetDefaultUAV()->GetIndex()
#define RegGetBufCounterSRVIdx(h) registry->GetD3D12Buffer(h)->GetCounterBuffer()->GetDefaultSRV()->GetIndex()
#define RegGetTexDefSRVIdx(h) registry->GetD3D12Texture(h)->GetDefaultSRV()->GetIndex()
#define RegGetTexDefUAVIdx(h) registry->GetD3D12Texture(h)->GetDefaultUAV()->GetIndex()

	void SSRPass::initialize(const SSRInitInfo& init_info)
	{
        colorTexDesc = init_info.m_ColorTexDesc;

        ShaderCompiler*       m_ShaderCompiler = init_info.m_ShaderCompiler;
        std::filesystem::path m_ShaderRootPath = init_info.m_ShaderRootPath;

        {
            SSRRaycastCS =
                m_ShaderCompiler->CompileShader(RHI_SHADER_TYPE::Compute,
                                                m_ShaderRootPath / "hlsl/SSRRaycastCS.hlsl",
                                                ShaderCompileOptions(L"CSRaycast"));

            RHI::RootSignatureDesc rootSigDesc =
                RHI::RootSignatureDesc()
                    .Add32BitConstants<0, 0>(12)
                    .AddStaticSampler<10, 0>(D3D12_FILTER::D3D12_FILTER_ANISOTROPIC,
                                             D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_WRAP,
                                             8)
                    .AllowInputLayout()
                    .AllowResourceDescriptorHeapIndexing()
                    .AllowSampleDescriptorHeapIndexing();

            pSSRRaycastSignature = std::make_shared<RHI::D3D12RootSignature>(m_Device, rootSigDesc);

            struct PsoStream
            {
                PipelineStateStreamRootSignature RootSignature;
                PipelineStateStreamCS            CS;
            } psoStream;
            psoStream.RootSignature         = PipelineStateStreamRootSignature(pSSRRaycastSignature.get());
            psoStream.CS                    = &SSRRaycastCS;
            PipelineStateStreamDesc psoDesc = {sizeof(PsoStream), &psoStream};

            pSSRRaycastPSO = std::make_shared<RHI::D3D12PipelineState>(m_Device, L"SSRRaycastPSO", psoDesc);
            
        }

        {
            SSRResolveCS = m_ShaderCompiler->CompileShader(RHI_SHADER_TYPE::Compute,
                                                           m_ShaderRootPath / "hlsl/SSRResolveCS.hlsl",
                                                           ShaderCompileOptions(L"CSResolve"));

            RHI::RootSignatureDesc rootSigDesc =
                RHI::RootSignatureDesc()
                    .Add32BitConstants<0, 0>(12)
                    .AddStaticSampler<10, 0>(D3D12_FILTER::D3D12_FILTER_ANISOTROPIC,
                                             D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_WRAP,
                                             8)
                    .AllowInputLayout()
                    .AllowResourceDescriptorHeapIndexing()
                    .AllowSampleDescriptorHeapIndexing();

            pSSRResolveSignature = std::make_shared<RHI::D3D12RootSignature>(m_Device, rootSigDesc);

            struct PsoStream
            {
                PipelineStateStreamRootSignature RootSignature;
                PipelineStateStreamCS            CS;
            } psoStream;
            psoStream.RootSignature         = PipelineStateStreamRootSignature(pSSRResolveSignature.get());
            psoStream.CS                    = &SSRResolveCS;
            PipelineStateStreamDesc psoDesc = {sizeof(PsoStream), &psoStream};

            pSSRResolvePSO = std::make_shared<RHI::D3D12PipelineState>(m_Device, L"SSRResolvePSO", psoDesc);
        }

        {
            SSRTemporalCS = m_ShaderCompiler->CompileShader(RHI_SHADER_TYPE::Compute,
                                                           m_ShaderRootPath / "hlsl/SSRTemporalCS.hlsl",
                                                           ShaderCompileOptions(L"CSTemporal"));

            RHI::RootSignatureDesc rootSigDesc =
                RHI::RootSignatureDesc()
                    .Add32BitConstants<0, 0>(12)
                    .AddStaticSampler<10, 0>(D3D12_FILTER::D3D12_FILTER_ANISOTROPIC,
                                             D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_WRAP,
                                             8)
                    .AllowInputLayout()
                    .AllowResourceDescriptorHeapIndexing()
                    .AllowSampleDescriptorHeapIndexing();

            pSSRTemporalSignature = std::make_shared<RHI::D3D12RootSignature>(m_Device, rootSigDesc);

            struct PsoStream
            {
                PipelineStateStreamRootSignature RootSignature;
                PipelineStateStreamCS            CS;
            } psoStream;
            psoStream.RootSignature         = PipelineStateStreamRootSignature(pSSRTemporalSignature.get());
            psoStream.CS                    = &SSRTemporalCS;
            PipelineStateStreamDesc psoDesc = {sizeof(PsoStream), &psoStream};

            pSSRTemporalPSO = std::make_shared<RHI::D3D12PipelineState>(m_Device, L"SSRTemporalPSO", psoDesc);
        }


    }

    void SSRPass::prepareMetaData(std::shared_ptr<RenderResource> render_resource)
    {
        HLSL::FrameUniforms* _frameUniforms = &(render_resource->m_FrameUniforms);

        glm::float2 costMapSize = glm::float2(colorTexDesc.Width, colorTexDesc.Height);
        glm::float2 raycastSize = glm::float2(colorTexDesc.Width, colorTexDesc.Height);
        glm::float2 resolveSize = glm::float2(colorTexDesc.Width, colorTexDesc.Height);

        glm::float2 jitterSample = GenerateRandomOffset();

        std::shared_ptr<RHI::D3D12Texture> pBlueNoiseUniMap = m_render_scene->m_bluenoise_map.m_bluenoise_64x64_uni;
        float noiseWidth = pBlueNoiseUniMap->GetWidth();
        float noiseHeight = pBlueNoiseUniMap->GetHeight();

        const int kMaxLods = 12;
        int lodCount = glm::log2((float)glm::min(colorTexDesc.Width, colorTexDesc.Height));
        lodCount = glm::min(lodCount, kMaxLods);

        HLSL::SSRUniform ssrUniform = {};

        ssrUniform.ScreenSize = glm::float4(colorTexDesc.Width, colorTexDesc.Height, 1.0f / colorTexDesc.Width, 1.0f / colorTexDesc.Height);
        ssrUniform.RayCastSize = glm::float4(raycastSize.x, raycastSize.y, 1.0f / raycastSize.x, 1.0f / raycastSize.y);
        ssrUniform.ResolveSize = glm::float4(resolveSize.x, resolveSize.y, 1.0f / resolveSize.x, 1.0f / resolveSize.y);
        ssrUniform.JitterSizeAndOffset = glm::float4((float)colorTexDesc.Width / (float)noiseWidth,
                                                     (float)colorTexDesc.Height / (float)noiseHeight,
                                                     jitterSample.x,
                                                     jitterSample.y);

        ssrUniform.SmoothnessRange = 1.0f;
        ssrUniform.BRDFBias        = 0.7f;
        ssrUniform.TResponseMin    = 0.85f;
        ssrUniform.TResponseMax    = 1.0f;
        ssrUniform.EdgeFactor      = 0.25f;
        ssrUniform.Thickness       = 0.2f;
        ssrUniform.NumSteps        = 70;
        ssrUniform.MaxMipMap       = lodCount;

        // SSR Uniform
        _frameUniforms->ssrUniform = ssrUniform;

    }

    void SSRPass::update(RHI::RenderGraph& graph, DrawInputParameters& passInput, DrawOutputParameters& passOutput)
    {
        




    }

    void SSRPass::destroy()
    {
        




    }

}
