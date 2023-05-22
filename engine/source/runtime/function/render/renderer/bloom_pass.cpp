#include "runtime/function/render/renderer/bloom_pass.h"
#include "runtime/resource/config_manager/config_manager.h"
#include "runtime/function/render/rhi/rhi_core.h"

#include <cassert>

namespace MoYu
{

	void BloomPass::initialize(const BloomInitInfo& init_info)
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
            m_aBloomUAV1Desc[0] = CreateTex2DDesc("Bloom Buffer 1a", kBloomWidth, kBloomHeight, DXGI_FORMAT_R32G32B32A32_FLOAT)
            m_aBloomUAV1Desc[1] = CreateTex2DDesc("Bloom Buffer 1b", kBloomWidth, kBloomHeight, DXGI_FORMAT_R32G32B32A32_FLOAT)
            m_aBloomUAV2Desc[0] = CreateTex2DDesc("Bloom Buffer 2a", kBloomWidth / 2, kBloomHeight / 2, DXGI_FORMAT_R32G32B32A32_FLOAT)
            m_aBloomUAV2Desc[1] = CreateTex2DDesc("Bloom Buffer 2b", kBloomWidth / 2, kBloomHeight / 2, DXGI_FORMAT_R32G32B32A32_FLOAT)
            m_aBloomUAV3Desc[0] = CreateTex2DDesc("Bloom Buffer 3a", kBloomWidth / 4, kBloomHeight / 4, DXGI_FORMAT_R32G32B32A32_FLOAT)
            m_aBloomUAV3Desc[1] = CreateTex2DDesc("Bloom Buffer 3b", kBloomWidth / 4, kBloomHeight / 4, DXGI_FORMAT_R32G32B32A32_FLOAT)
            m_aBloomUAV4Desc[0] = CreateTex2DDesc("Bloom Buffer 4a", kBloomWidth / 8, kBloomHeight / 8, DXGI_FORMAT_R32G32B32A32_FLOAT)
            m_aBloomUAV4Desc[1] = CreateTex2DDesc("Bloom Buffer 4b", kBloomWidth / 8, kBloomHeight / 8, DXGI_FORMAT_R32G32B32A32_FLOAT)
            m_aBloomUAV5Desc[0] = CreateTex2DDesc("Bloom Buffer 5a", kBloomWidth / 16, kBloomHeight / 16, DXGI_FORMAT_R32G32B32A32_FLOAT)
            m_aBloomUAV5Desc[1] = CreateTex2DDesc("Bloom Buffer 5b", kBloomWidth / 16, kBloomHeight / 16, DXGI_FORMAT_R32G32B32A32_FLOAT)
            // clang-format on
        }

		ShaderCompiler*       m_ShaderCompiler = init_info.m_ShaderCompiler;
        std::filesystem::path m_ShaderRootPath = init_info.m_ShaderRootPath;

        BloomExtractAndDownsampleHdrCS = 
			m_ShaderCompiler->CompileShader(RHI_SHADER_TYPE::Compute, m_ShaderRootPath / "hlsl/BloomExtractAndDownsampleHdrCS.hlsl", ShaderCompileOptions(L"main"));
        DownsampleBloom4CS = m_ShaderCompiler->CompileShader(RHI_SHADER_TYPE::Compute, m_ShaderRootPath / "hlsl/DownsampleBloomAllCS.hlsl", ShaderCompileOptions(L"main"));
        DownsampleBloom2CS = m_ShaderCompiler->CompileShader(RHI_SHADER_TYPE::Compute, m_ShaderRootPath / "hlsl/DownsampleBloomCS.hlsl", ShaderCompileOptions(L"main"));
        BlurCS = m_ShaderCompiler->CompileShader(RHI_SHADER_TYPE::Compute, m_ShaderRootPath / "hlsl/BlurCS.hlsl", ShaderCompileOptions(L"main"));
        UpsampleAndBlurCS = m_ShaderCompiler->CompileShader(RHI_SHADER_TYPE::Compute, m_ShaderRootPath / "hlsl/UpsampleAndBlurCS.hlsl", ShaderCompileOptions(L"main"));

        {
            RHI::RootSignatureDesc rootSigDesc =
                RHI::RootSignatureDesc()
                    .Add32BitConstants<0, 0>(3)
                    .AddConstantBufferView<1, 0>()
                    .AddStaticSampler<0, 0>(D3D12_FILTER::D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_CLAMP, 8)
                    .AllowResourceDescriptorHeapIndexing()
                    .AllowSampleDescriptorHeapIndexing();

            pBloomExtractAndDownsampleHdrCSSignature = std::make_shared<RHI::D3D12RootSignature>(m_Device, rootSigDesc);
        }
        {
            RHI::RootSignatureDesc rootSigDesc =
                RHI::RootSignatureDesc()
                    .Add32BitConstants<0, 0>(2)
                    .AddConstantBufferView<1, 0>()
                    .AddStaticSampler<0, 0>(D3D12_FILTER::D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_CLAMP, 8)
                    .AllowResourceDescriptorHeapIndexing()
                    .AllowSampleDescriptorHeapIndexing();

            pDownsampleBloom4CSSignature = std::make_shared<RHI::D3D12RootSignature>(m_Device, rootSigDesc);
            pDownsampleBloom2CSSignature = pDownsampleBloom4CSSignature;
        }
        {
            RHI::RootSignatureDesc rootSigDesc =
                RHI::RootSignatureDesc()
                    .Add32BitConstants<0, 0>(2)
                    .AddConstantBufferView<1, 0>()
                    .AllowResourceDescriptorHeapIndexing()
                    .AllowSampleDescriptorHeapIndexing();

            pBlurCSSignature = std::make_shared<RHI::D3D12RootSignature>(m_Device, rootSigDesc);
        }
        {
            RHI::RootSignatureDesc rootSigDesc =
                RHI::RootSignatureDesc()
                    .Add32BitConstants<0, 0>(3)
                    .AddConstantBufferView<1, 0>()
                    .AddStaticSampler<10, 0>(D3D12_FILTER::D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_CLAMP, 8)
                    .AllowResourceDescriptorHeapIndexing()
                    .AllowSampleDescriptorHeapIndexing();

            pUpsampleAndBlurCSSignature = std::make_shared<RHI::D3D12RootSignature>(m_Device, rootSigDesc);
        }

        {
            struct PsoStream
            {
                PipelineStateStreamRootSignature RootSignature;
                PipelineStateStreamCS            CS;
            } psoStream;
            psoStream.RootSignature = PipelineStateStreamRootSignature(pBloomExtractAndDownsampleHdrCSSignature.get());
            psoStream.CS            = &BloomExtractAndDownsampleHdrCS;
            PipelineStateStreamDesc psoDesc = {sizeof(PsoStream), &psoStream};

            pBloomExtractAndDownsampleHdrCSPSO =
                std::make_shared<RHI::D3D12PipelineState>(m_Device, L"BloomExtractAndDownsampleHdrPSO", psoDesc);
        }
        {
            struct PsoStream
            {
                PipelineStateStreamRootSignature RootSignature;
                PipelineStateStreamCS            CS;
            } psoStream;
            psoStream.RootSignature         = PipelineStateStreamRootSignature(pDownsampleBloom4CSSignature.get());
            psoStream.CS                    = &DownsampleBloom4CS;
            PipelineStateStreamDesc psoDesc = {sizeof(PsoStream), &psoStream};

            pDownsampleBloom4CSPSO =
                std::make_shared<RHI::D3D12PipelineState>(m_Device, L"DownsampleBloom4CSPSO", psoDesc);
        }
        {
            struct PsoStream
            {
                PipelineStateStreamRootSignature RootSignature;
                PipelineStateStreamCS            CS;
            } psoStream;
            psoStream.RootSignature         = PipelineStateStreamRootSignature(pDownsampleBloom2CSSignature.get());
            psoStream.CS                    = &DownsampleBloom2CS;
            PipelineStateStreamDesc psoDesc = {sizeof(PsoStream), &psoStream};

            pDownsampleBloom2CSPSO =
                std::make_shared<RHI::D3D12PipelineState>(m_Device, L"DownsampleBloom2CSPSO", psoDesc);
        }
        {
            struct PsoStream
            {
                PipelineStateStreamRootSignature RootSignature;
                PipelineStateStreamCS            CS;
            } psoStream;
            psoStream.RootSignature         = PipelineStateStreamRootSignature(pBlurCSSignature.get());
            psoStream.CS                    = &BlurCS;
            PipelineStateStreamDesc psoDesc = {sizeof(PsoStream), &psoStream};

            pBlurCSPSO = std::make_shared<RHI::D3D12PipelineState>(m_Device, L"BlurCSPSO", psoDesc);
        }
        {
            struct PsoStream
            {
                PipelineStateStreamRootSignature RootSignature;
                PipelineStateStreamCS            CS;
            } psoStream;
            psoStream.RootSignature         = PipelineStateStreamRootSignature(pUpsampleAndBlurCSSignature.get());
            psoStream.CS                    = &UpsampleAndBlurCS;
            PipelineStateStreamDesc psoDesc = {sizeof(PsoStream), &psoStream};

            pUpsampleAndBlurCSPSO =
                std::make_shared<RHI::D3D12PipelineState>(m_Device, L"UpsampleAndBlurCSPSO", psoDesc);
        }

	}

	void BloomPass::update(RHI::RenderGraph& graph, DrawInputParameters& passInput, DrawOutputParameters& passOutput)
	{
        int kBloomWidth  = m_LumaLRDesc.Width;
        int kBloomHeight = m_LumaLRDesc.Height;

        RHI::RgResourceHandle m_LumaLRHandle = graph.Create<RHI::D3D12Texture>(m_LumaLRDesc);
        RHI::RgResourceHandle m_aBloomUAV1Handle[2] = {graph.Create<RHI::D3D12Texture>(m_aBloomUAV1Desc[0]),
                                                       graph.Create<RHI::D3D12Texture>(m_aBloomUAV1Desc[1])};
        RHI::RgResourceHandle m_aBloomUAV2Handle[2] = {graph.Create<RHI::D3D12Texture>(m_aBloomUAV2Desc[0]),
                                                       graph.Create<RHI::D3D12Texture>(m_aBloomUAV2Desc[1])};
        RHI::RgResourceHandle m_aBloomUAV3Handle[2] = {graph.Create<RHI::D3D12Texture>(m_aBloomUAV3Desc[0]),
                                                       graph.Create<RHI::D3D12Texture>(m_aBloomUAV3Desc[1])};
        RHI::RgResourceHandle m_aBloomUAV4Handle[2] = {graph.Create<RHI::D3D12Texture>(m_aBloomUAV4Desc[0]),
                                                       graph.Create<RHI::D3D12Texture>(m_aBloomUAV4Desc[1])};
        RHI::RgResourceHandle m_aBloomUAV5Handle[2] = {graph.Create<RHI::D3D12Texture>(m_aBloomUAV5Desc[0]),
                                                       graph.Create<RHI::D3D12Texture>(m_aBloomUAV5Desc[1])};

        auto inputSceneColorHandle = passInput.inputSceneColorHandle;
        auto inputExposureHandle   = passInput.inputExposureHandle;

        RHI::RenderPass& generateBloomPass = graph.AddRenderPass("GenerateBloom");

        generateBloomPass.Read(passInput.inputSceneColorHandle);
        generateBloomPass.Read(passInput.inputExposureHandle);
        generateBloomPass.Write(m_aBloomUAV1Handle[0]);
        generateBloomPass.Write(m_LumaLRHandle);

        generateBloomPass.Execute([=](RHI::RenderGraphRegistry* registry, RHI::D3D12CommandContext* context) {
            RHI::D3D12ComputeContext* computeContext = context->GetComputeContext();

            RHI::D3D12Texture* pInputSceneColor = registry->GetD3D12Texture(inputSceneColorHandle);
            RHI::D3D12ShaderResourceView*  pInputSceneColorSRV = pInputSceneColor->GetDefaultSRV().get();
            RHI::D3D12Buffer*              pInputExposure = registry->GetD3D12Buffer(inputExposureHandle);
            RHI::D3D12ShaderResourceView*  pInputExposureSRV = pInputExposure->GetDefaultSRV().get();
            RHI::D3D12Texture*             pOutputBloom1     = registry->GetD3D12Texture(m_aBloomUAV1Handle[0]);
            RHI::D3D12UnorderedAccessView* pOutputBloom1UAV  = pOutputBloom1->GetDefaultUAV().get();
            RHI::D3D12Texture*             pLumaLRHandle     = registry->GetD3D12Texture(m_LumaLRHandle);
            RHI::D3D12UnorderedAccessView* pOutputLumaLRUAV  = pLumaLRHandle->GetDefaultUAV().get();

            computeContext->SetRootSignature(pBloomExtractAndDownsampleHdrCSSignature.get());
            computeContext->SetPipelineState(pBloomExtractAndDownsampleHdrCSPSO.get());

            float BloomThreshold = EngineConfig::g_BloomConfig.m_BloomThreshold;

            computeContext->SetConstants(0, 1.0f / kBloomWidth, 1.0f / kBloomHeight, (float)BloomThreshold);

            __declspec(align(16)) struct
            {
                uint32_t m_SourceTexIndex;
                uint32_t m_ExposureIndex;
                uint32_t m_BloomResultIndex;
                uint32_t m_LumaResultIndex;
            } _DescriptorIndex = {
                pInputSceneColorSRV->GetIndex(),
                pInputExposureSRV->GetIndex(),
                pOutputBloom1UAV->GetIndex(),
                pOutputLumaLRUAV->GetIndex(),
            };

            computeContext->SetDynamicConstantBufferView(1, sizeof(_DescriptorIndex), &_DescriptorIndex);

            computeContext->Dispatch2D(kBloomWidth, kBloomHeight);
        });

        // The difference between high and low quality bloom is that high quality sums 5 octaves with a 2x frequency
        // scale, and the low quality
        // sums 3 octaves with a 4x frequency scale.
        if (EngineConfig::g_BloomConfig.m_HighQualityBloom)
        {
            RHI::RenderPass& downsampleBloomPass = graph.AddRenderPass("DownSampleBloom");

            downsampleBloomPass.Read(m_aBloomUAV1Handle[0]);
            downsampleBloomPass.Write(m_aBloomUAV2Handle[0]);
            downsampleBloomPass.Write(m_aBloomUAV3Handle[0]);
            downsampleBloomPass.Write(m_aBloomUAV4Handle[0]);
            downsampleBloomPass.Write(m_aBloomUAV5Handle[0]);

            downsampleBloomPass.Execute([=](RHI::RenderGraphRegistry* registry, RHI::D3D12CommandContext* context) {
                RHI::D3D12ComputeContext* computeContext = context->GetComputeContext();

                RHI::D3D12Texture*            paBloomUAV1Tex0    = registry->GetD3D12Texture(m_aBloomUAV1Handle[0]);
                RHI::D3D12ShaderResourceView* paBloomUAV1Tex0SRV = paBloomUAV1Tex0->GetDefaultSRV().get();

                RHI::D3D12Texture*             paBloomUAV2Tex0   = registry->GetD3D12Texture(m_aBloomUAV2Handle[0]);
                RHI::D3D12UnorderedAccessView* paBloomUAV2Tex0UAV = paBloomUAV2Tex0->GetDefaultUAV().get();
                RHI::D3D12Texture*             paBloomUAV3Tex0    = registry->GetD3D12Texture(m_aBloomUAV3Handle[0]);
                RHI::D3D12UnorderedAccessView* paBloomUAV3Tex0UAV = paBloomUAV3Tex0->GetDefaultUAV().get();
                RHI::D3D12Texture*             paBloomUAV4Tex0    = registry->GetD3D12Texture(m_aBloomUAV4Handle[0]);
                RHI::D3D12UnorderedAccessView* paBloomUAV4Tex0UAV = paBloomUAV4Tex0->GetDefaultUAV().get();
                RHI::D3D12Texture*             paBloomUAV5Tex0    = registry->GetD3D12Texture(m_aBloomUAV5Handle[0]);
                RHI::D3D12UnorderedAccessView* paBloomUAV5Tex0UAV = paBloomUAV5Tex0->GetDefaultUAV().get();

                computeContext->SetRootSignature(pDownsampleBloom4CSSignature.get());
                computeContext->SetPipelineState(pDownsampleBloom4CSPSO.get()); 
                
                computeContext->SetConstants(0, 1.0f / kBloomWidth, 1.0f / kBloomHeight);

                __declspec(align(16)) struct
                {
                    uint32_t m_BloomBufIndex;
                    uint32_t m_Result1Index;
                    uint32_t m_Result2Index;
                    uint32_t m_Result3Index;
                    uint32_t m_Result4Index;
                } _DescriptorIndex = {
                    paBloomUAV1Tex0SRV->GetIndex(),
                    paBloomUAV2Tex0UAV->GetIndex(),
                    paBloomUAV3Tex0UAV->GetIndex(),
                    paBloomUAV4Tex0UAV->GetIndex(),
                    paBloomUAV5Tex0UAV->GetIndex(),
                };

                computeContext->SetDynamicConstantBufferView(1, sizeof(_DescriptorIndex), &_DescriptorIndex);

                computeContext->Dispatch2D(kBloomWidth / 2, kBloomHeight / 2);
            });
            
            float upsampleBlendFactor = EngineConfig::g_BloomConfig.m_BloomUpsampleFactor;
            
            // Blur then upsample and blur four times
            BlurBuffer(graph, m_aBloomUAV5Handle, m_aBloomUAV5Handle[0], 1.0f);
            BlurBuffer(graph, m_aBloomUAV4Handle, m_aBloomUAV5Handle[1], upsampleBlendFactor);
            BlurBuffer(graph, m_aBloomUAV3Handle, m_aBloomUAV4Handle[1], upsampleBlendFactor);
            BlurBuffer(graph, m_aBloomUAV2Handle, m_aBloomUAV3Handle[1], upsampleBlendFactor);
            BlurBuffer(graph, m_aBloomUAV1Handle, m_aBloomUAV2Handle[1], upsampleBlendFactor);
        }
        else
        {
            RHI::RenderPass& downsampleBloomPass = graph.AddRenderPass("DownSampleBloom");

            downsampleBloomPass.Read(m_aBloomUAV1Handle[0]);
            downsampleBloomPass.Write(m_aBloomUAV3Handle[0]);
            downsampleBloomPass.Write(m_aBloomUAV5Handle[0]);

            downsampleBloomPass.Execute([=](RHI::RenderGraphRegistry* registry, RHI::D3D12CommandContext* context) {
                RHI::D3D12ComputeContext* computeContext = context->GetComputeContext();

                RHI::D3D12Texture*            paBloomUAV1Tex0    = registry->GetD3D12Texture(m_aBloomUAV1Handle[0]);
                RHI::D3D12ShaderResourceView* paBloomUAV1Tex0SRV = paBloomUAV1Tex0->GetDefaultSRV().get();

                RHI::D3D12Texture*             paBloomUAV3Tex0    = registry->GetD3D12Texture(m_aBloomUAV3Handle[0]);
                RHI::D3D12UnorderedAccessView* paBloomUAV3Tex0UAV = paBloomUAV3Tex0->GetDefaultUAV().get();
                RHI::D3D12Texture*             paBloomUAV5Tex0    = registry->GetD3D12Texture(m_aBloomUAV5Handle[0]);
                RHI::D3D12UnorderedAccessView* paBloomUAV5Tex0UAV = paBloomUAV5Tex0->GetDefaultUAV().get();

                computeContext->SetRootSignature(pDownsampleBloom4CSSignature.get());
                computeContext->SetPipelineState(pDownsampleBloom4CSPSO.get());

                computeContext->SetConstants(0, 1.0f / kBloomWidth, 1.0f / kBloomHeight);

                __declspec(align(16)) struct
                {
                    uint32_t m_BloomBufIndex;
                    uint32_t m_Result1Index;
                    uint32_t m_Result2Index;
                } _DescriptorIndex = {
                    paBloomUAV1Tex0SRV->GetIndex(),
                    paBloomUAV3Tex0UAV->GetIndex(),
                    paBloomUAV5Tex0UAV->GetIndex(),
                };

                computeContext->SetDynamicConstantBufferView(1, sizeof(_DescriptorIndex), &_DescriptorIndex);

                computeContext->Dispatch2D(kBloomWidth / 2, kBloomHeight / 2);
            });

            float upsampleBlendFactor = EngineConfig::g_BloomConfig.m_BloomUpsampleFactor * 2.0f / 3.0f;

            // Blur then upsample and blur two times
            BlurBuffer(graph, m_aBloomUAV5Handle, m_aBloomUAV5Handle[0], 1.0f);
            BlurBuffer(graph, m_aBloomUAV3Handle, m_aBloomUAV5Handle[1], upsampleBlendFactor);
            BlurBuffer(graph, m_aBloomUAV1Handle, m_aBloomUAV3Handle[1], upsampleBlendFactor);
        }
        
        passOutput.outputLumaLRHandle = m_LumaLRHandle;
        passOutput.outputBloomHandle  = m_aBloomUAV1Handle[1];
	}

    void BloomPass::BlurBuffer(RHI::RenderGraph& graph, RHI::RgResourceHandle buffer[2], RHI::RgResourceHandle& lowerResBuf, float upsampleBlendFactor)
    {
        RHI::RenderPass& upsampleBloomPass = graph.AddRenderPass("UpSample");

        if (buffer[0] != lowerResBuf)
        {
            RHI::RgResourceHandle srcHandle = buffer[0];
            RHI::RgResourceHandle dstHandle = buffer[1];

            upsampleBloomPass.Read(buffer[0]);
            upsampleBloomPass.Read(lowerResBuf);
            upsampleBloomPass.Write(buffer[1]);

            upsampleBloomPass.Execute([=](RHI::RenderGraphRegistry* registry, RHI::D3D12CommandContext* context) {
                RHI::D3D12ComputeContext* computeContext = context->GetComputeContext();

                RHI::D3D12Texture*             buffer0Tex        = registry->GetD3D12Texture(srcHandle);
                RHI::D3D12ShaderResourceView*  buffer0TexSRV     = buffer0Tex->GetDefaultSRV().get();
                RHI::D3D12Texture*             lowerResBufTex    = registry->GetD3D12Texture(lowerResBuf);
                RHI::D3D12ShaderResourceView*  lowerResBufTexSRV = lowerResBufTex->GetDefaultSRV().get();
                RHI::D3D12Texture*             buffer1Tex        = registry->GetD3D12Texture(dstHandle);
                RHI::D3D12UnorderedAccessView* buffer1TexUAV     = buffer1Tex->GetDefaultUAV().get();

                uint32_t bufferWidth  = buffer0Tex->GetWidth();
                uint32_t bufferHeight = buffer0Tex->GetHeight();

                computeContext->SetRootSignature(pUpsampleAndBlurCSSignature.get());
                computeContext->SetPipelineState(pUpsampleAndBlurCSPSO.get());

                computeContext->SetConstants(0, 1.0f / bufferWidth, 1.0f / bufferHeight, upsampleBlendFactor);

                __declspec(align(16)) struct
                {
                    uint32_t m_HigherResBufIndex;
                    uint32_t m_LowerResBufIndex;
                    uint32_t m_ResultIndex;
                } _DescriptorIndex = {
                    buffer0TexSRV->GetIndex(),
                    lowerResBufTexSRV->GetIndex(),
                    buffer1TexUAV->GetIndex(),
                };

                computeContext->SetDynamicConstantBufferView(1, sizeof(_DescriptorIndex), &_DescriptorIndex);

                computeContext->Dispatch2D(bufferWidth, bufferHeight);
            });
        }
        else
        {
            RHI::RgResourceHandle srcHandle = buffer[0];
            RHI::RgResourceHandle dstHandle = buffer[1];

            upsampleBloomPass.Read(buffer[0]);
            upsampleBloomPass.Write(buffer[1]);

            upsampleBloomPass.Execute([=](RHI::RenderGraphRegistry* registry, RHI::D3D12CommandContext* context) {
                RHI::D3D12ComputeContext* computeContext = context->GetComputeContext();

                RHI::D3D12Texture*             buffer0Tex        = registry->GetD3D12Texture(srcHandle);
                RHI::D3D12ShaderResourceView*  buffer0TexSRV     = buffer0Tex->GetDefaultSRV().get();
                RHI::D3D12Texture*             buffer1Tex        = registry->GetD3D12Texture(dstHandle);
                RHI::D3D12UnorderedAccessView* buffer1TexUAV     = buffer1Tex->GetDefaultUAV().get();

                uint32_t bufferWidth  = buffer0Tex->GetWidth();
                uint32_t bufferHeight = buffer0Tex->GetHeight();

                computeContext->SetRootSignature(pBlurCSSignature.get());
                computeContext->SetPipelineState(pBlurCSPSO.get());

                computeContext->SetConstants(0, 1.0f / bufferWidth, 1.0f / bufferHeight);

                __declspec(align(16)) struct
                {
                    uint32_t m_InputBufIndex;
                    uint32_t m_ResultIndex;
                } _DescriptorIndex = {
                    buffer0TexSRV->GetIndex(),
                    buffer1TexUAV->GetIndex(),
                };

                computeContext->SetDynamicConstantBufferView(1, sizeof(_DescriptorIndex), &_DescriptorIndex);

                computeContext->Dispatch2D(bufferWidth, bufferHeight);
            });
        }
    }

	void BloomPass::destroy()
	{

        pBloomExtractAndDownsampleHdrCSSignature = nullptr;
        pBloomExtractAndDownsampleHdrCSPSO       = nullptr;

        pDownsampleBloom4CSSignature = nullptr;
        pDownsampleBloom4CSPSO       = nullptr;

        pDownsampleBloom2CSSignature = nullptr;
        pDownsampleBloom2CSPSO       = nullptr;

        pBlurCSSignature = nullptr;
        pBlurCSPSO       = nullptr;

        pUpsampleAndBlurCSSignature = nullptr;
        pUpsampleAndBlurCSPSO       = nullptr;
	}

}
