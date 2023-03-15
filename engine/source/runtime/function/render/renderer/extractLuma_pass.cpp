#include "runtime/function/render/renderer/extractLuma_pass.h"
#include "runtime/resource/config_manager/config_manager.h"
#include "runtime/function/render/rhi/rhi_core.h"

#include <cassert>

namespace Pilot
{
    void ExtractLumaPass::initialize(const ExtractLumaInitInfo& init_info)
    {
        {
            // We can generate a bloom buffer up to 1/4 smaller in each dimension without undersampling.  If only
            // downsizing by 1/2 or less, a faster
            // shader can be used which only does one bilinear sample.

#define CreateTex2DDesc(name, width, height, format) \
    RHI::RgTextureDesc(name) \
        .SetType(RHI::RgTextureType::Texture2D) \
        .SetFormat(format) \
        .SetExtent(width, height) \
        .SetAllowUnorderedAccess(true);\

            // Divisible by 128 so that after dividing by 16, we still have multiples of 8x8 tiles.  The bloom
            // dimensions must be at least 1/4 native resolution to avoid undersampling.
            // uint32_t kBloomWidth = bufferWidth > 2560 ? Math::AlignUp(bufferWidth / 4, 128) : 640;
            // uint32_t kBloomHeight = bufferHeight > 1440 ? Math::AlignUp(bufferHeight / 4, 128) : 384;
            uint32_t kBloomWidth  = init_info.m_ColorTexDesc.Width > 2560 ? 1280 : 640;
            uint32_t kBloomHeight = init_info.m_ColorTexDesc.Height > 1440 ? 768 : 384;

            // These bloom buffer dimensions were chosen for their impressive divisibility by 128 and because they are
            // roughly 16:9. The blurring algorithm is exactly 9 pixels by 9 pixels, so if the aspect ratio of each
            // pixel is not square, the blur will be oval in appearance rather than circular.  Coincidentally, they are
            // close to 1/2 of a 720p buffer and 1/3 of 1080p.  This is a common size for a bloom buffer on consoles.
            ASSERT(kBloomWidth % 16 == 0 && kBloomHeight % 16 == 0);

            // clang-format off
            m_LumaLRDesc = CreateTex2DDesc("LumaLR", kBloomWidth, kBloomHeight, DXGI_FORMAT_R8_UINT)
            // clang-format on
        }

		ShaderCompiler*       m_ShaderCompiler = init_info.m_ShaderCompiler;
        std::filesystem::path m_ShaderRootPath = init_info.m_ShaderRootPath;

        ExtractLumaCS = m_ShaderCompiler->CompileShader(
            RHI_SHADER_TYPE::Compute, m_ShaderRootPath / "hlsl/ExtractLumaCS.hlsl", ShaderCompileOptions(L"main"));

        {
            RHI::RootSignatureDesc rootSigDesc =
                RHI::RootSignatureDesc()
                    .Add32BitConstants<0, 0>(2)
                    .AddConstantBufferView<1, 0>()
                    .AddStaticSampler<0, 0>(D3D12_FILTER::D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_CLAMP, 8)
                    .AllowResourceDescriptorHeapIndexing()
                    .AllowSampleDescriptorHeapIndexing();

            pExtractLumaCSSignature = std::make_shared<RHI::D3D12RootSignature>(m_Device, rootSigDesc);
        }

        {
            struct PsoStream
            {
                PipelineStateStreamRootSignature RootSignature;
                PipelineStateStreamCS            CS;
            } psoStream;
            psoStream.RootSignature         = PipelineStateStreamRootSignature(pExtractLumaCSSignature.get());
            psoStream.CS                    = &ExtractLumaCS;
            PipelineStateStreamDesc psoDesc = {sizeof(PsoStream), &psoStream};

            pExtractLumaCSPSO = std::make_shared<RHI::D3D12PipelineState>(m_Device, L"ExtractLumaCSPSO", psoDesc);
        }

    }

    void ExtractLumaPass::update(RHI::RenderGraph& graph, DrawInputParameters& passInput, DrawOutputParameters& passOutput)
    {
        DrawInputParameters  drawPassInput  = passInput;
        DrawOutputParameters drawPassOutput = passOutput;

        int kBloomWidth  = m_LumaLRDesc.Width;
        int kBloomHeight = m_LumaLRDesc.Height;

        RHI::RgResourceHandle m_LumaLRHandle = graph.Create<RHI::D3D12Texture>(m_LumaLRDesc);
        
        drawPassOutput.outputLumaLRHandle = m_LumaLRHandle;

        RHI::RenderPass& generateBloomPass = graph.AddRenderPass("ExtractLumaPass");

        generateBloomPass.Read(drawPassInput.inputSceneColorHandle);
        generateBloomPass.Read(drawPassInput.inputExposureHandle);
        generateBloomPass.Write(m_LumaLRHandle);

        generateBloomPass.Execute([=](RHI::RenderGraphRegistry* registry, RHI::D3D12CommandContext* context) {
            RHI::D3D12ComputeContext* computeContext = context->GetComputeContext();

            RHI::D3D12Texture* pInputSceneColor = registry->GetD3D12Texture(drawPassInput.inputSceneColorHandle);
            RHI::D3D12ShaderResourceView*  pInputSceneColorSRV = pInputSceneColor->GetDefaultSRV().get();
            RHI::D3D12Buffer*              pInputExposure = registry->GetD3D12Buffer(drawPassInput.inputExposureHandle);
            RHI::D3D12ShaderResourceView*  pInputExposureSRV = pInputExposure->GetDefaultSRV().get();
            RHI::D3D12Texture*             pLumaLRHandle     = registry->GetD3D12Texture(m_LumaLRHandle);
            RHI::D3D12UnorderedAccessView* pOutputLumaLRUAV  = pLumaLRHandle->GetDefaultUAV().get();

            computeContext->SetRootSignature(pExtractLumaCSSignature.get());
            computeContext->SetPipelineState(pExtractLumaCSPSO.get());

            float BloomThreshold = EngineConfig::g_BloomConfig.m_BloomThreshold;

            computeContext->SetConstants(0, 1.0f / kBloomWidth, 1.0f / kBloomHeight);

            __declspec(align(16)) struct
            {
                uint32_t m_SourceTexIndex;
                uint32_t m_ExposureIndex;
                uint32_t m_LumaResultIndex;
            } _DescriptorIndex = {
                pInputSceneColorSRV->GetIndex(),
                pInputExposureSRV->GetIndex(),
                pOutputLumaLRUAV->GetIndex(),
            };

            computeContext->SetDynamicConstantBufferView(1, sizeof(_DescriptorIndex), &_DescriptorIndex);

            computeContext->Dispatch2D(kBloomWidth, kBloomHeight);
        });
    }

    void ExtractLumaPass::destroy()
    {
        pExtractLumaCSSignature = nullptr;
        pExtractLumaCSPSO       = nullptr;
    }

}
