#include "runtime/function/render/renderer/gtao_pass.h"
#include "runtime/resource/config_manager/config_manager.h"
#include "runtime/function/render/rhi/rhi_core.h"
#include "runtime/function/render/renderer/pass_helper.h"
#include "runtime/function/render/renderer/renderer_config.h"

#include <cassert>

namespace MoYu
{

	void GTAOPass::initialize(const AOInitInfo& init_info)
	{
        colorTexDesc = init_info.colorTexDesc;
        depthTexDesc = init_info.depthTexDesc;

        ShaderCompiler*       m_ShaderCompiler = init_info.m_ShaderCompiler;
        std::filesystem::path m_ShaderRootPath = init_info.m_ShaderRootPath;

        DepthPrefilterCS = m_ShaderCompiler->CompileShader(
            RHI_SHADER_TYPE::Compute, m_ShaderRootPath / "hlsl/vaGTAOPrefilter.hlsl", ShaderCompileOptions(L"CSPrefilterDepths16x16"));

        GTAOCS = m_ShaderCompiler->CompileShader(
            RHI_SHADER_TYPE::Compute, m_ShaderRootPath / "hlsl/vaGTAOMain.hlsl", ShaderCompileOptions(L"CSGTAOHigh"));

        DenoiseCS = m_ShaderCompiler->CompileShader(
            RHI_SHADER_TYPE::Compute, m_ShaderRootPath / "hlsl/vaGTAODenoise.hlsl", ShaderCompileOptions(L"CSDenoisePass"));

        {
            RHI::RootSignatureDesc rootSigDesc =
                RHI::RootSignatureDesc()
                    .Add32BitConstants<0, 0>(16)
                    .AddStaticSampler<10, 0>(D3D12_FILTER::D3D12_FILTER_MIN_MAG_MIP_POINT,
                                             D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
                                             1)
                    .AllowInputLayout()
                    .AllowResourceDescriptorHeapIndexing()
                    .AllowSampleDescriptorHeapIndexing();

            pDepthPrefilterSignature = std::make_shared<RHI::D3D12RootSignature>(m_Device, rootSigDesc);
        }
        {
            RHI::RootSignatureDesc rootSigDesc =
                RHI::RootSignatureDesc()
                    .Add32BitConstants<0, 0>(16)
                    .AddStaticSampler<10, 0>(D3D12_FILTER::D3D12_FILTER_MIN_MAG_MIP_POINT,
                                             D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
                                             1)
                    .AllowInputLayout()
                    .AllowResourceDescriptorHeapIndexing()
                    .AllowSampleDescriptorHeapIndexing();

            pGTAOSignature = std::make_shared<RHI::D3D12RootSignature>(m_Device, rootSigDesc);
        }
        
        {
            RHI::RootSignatureDesc rootSigDesc =
                RHI::RootSignatureDesc()
                    .Add32BitConstants<0, 0>(4)
                    .AddConstantBufferView<1, 0>()
                    .AddStaticSampler<10, 0>(D3D12_FILTER::D3D12_FILTER_ANISOTROPIC,
                                             D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_WRAP,
                                             8)
                    .AddStaticSampler<11, 0>(D3D12_FILTER::D3D12_FILTER_COMPARISON_ANISOTROPIC,
                                             D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
                                             8,
                                             D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_GREATER_EQUAL,
                                             D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK)
                    .AllowInputLayout()
                    .AllowResourceDescriptorHeapIndexing()
                    .AllowSampleDescriptorHeapIndexing();

            pDenoiseSignature = std::make_shared<RHI::D3D12RootSignature>(m_Device, rootSigDesc);
        }

        {
            struct PsoStream
            {
                PipelineStateStreamRootSignature RootSignature;
                PipelineStateStreamCS            CS;
            } psoStream;
            psoStream.RootSignature         = PipelineStateStreamRootSignature(pDepthPrefilterSignature.get());
            psoStream.CS                    = &DepthPrefilterCS;
            PipelineStateStreamDesc psoDesc = {sizeof(PsoStream), &psoStream};

            pDepthPrefilterPSO = std::make_shared<RHI::D3D12PipelineState>(m_Device, L"GTAOPrefilterPSO", psoDesc);
        }
        {
            struct PsoStream
            {
                PipelineStateStreamRootSignature RootSignature;
                PipelineStateStreamCS            CS;
            } psoStream;
            psoStream.RootSignature         = PipelineStateStreamRootSignature(pGTAOSignature.get());
            psoStream.CS                    = &GTAOCS;
            PipelineStateStreamDesc psoDesc = {sizeof(PsoStream), &psoStream};

            pGTAOPSO = std::make_shared<RHI::D3D12PipelineState>(m_Device, L"GTAOPSO", psoDesc);
        }
        {
            struct PsoStream
            {
                PipelineStateStreamRootSignature RootSignature;
                PipelineStateStreamCS            CS;
            } psoStream;
            psoStream.RootSignature         = PipelineStateStreamRootSignature(pDenoiseSignature.get());
            psoStream.CS                    = &DenoiseCS;
            PipelineStateStreamDesc psoDesc = {sizeof(PsoStream), &psoStream};

            pDenoisePSO = std::make_shared<RHI::D3D12PipelineState>(m_Device, L"DenoisePSO", psoDesc);
        }

        if (pGTAOConstants == nullptr)
        {
            pGTAOConstants = RHI::D3D12Buffer::Create(m_Device->GetLinkedDevice(),
                                                      RHI::RHIBufferTargetNone,
                                                      1,
                                                      MoYu::AlignUp(sizeof(XeGTAO::GTAOConstants), 256),
                                                      L"GTAOConstants",
                                                      RHI::RHIBufferModeDynamic,
                                                      D3D12_RESOURCE_STATE_GENERIC_READ);
        }



    }

    void GTAOPass::updateConstantBuffer(std::shared_ptr<RenderResource> render_resource)
    {
        int viewportWidth = m_render_camera->m_pixelWidth;
        int viewportHeight = m_render_camera->m_pixelHeight;
        glm::float4x4 proj = m_render_camera->getUnJitterPersProjMatrix();
        float projMatrix[16] = {0};
        for (int i = 0; i < 4; i++)
        {
            for (int j = 0; j < 4; j++)
            {
                projMatrix[i * 4 + j] = proj[j][i];
            }
        }
        GTAOUpdateConstants(gtaoConstants, viewportWidth, viewportHeight, gtaoSettings, projMatrix, frameCounter++);
        frameCounter %= 64;

        memcpy(pGTAOConstants->GetCpuVirtualAddress<XeGTAO::GTAOConstants>(),
               &gtaoConstants,
               sizeof(XeGTAO::GTAOConstants));
    }

    void GTAOPass::update(RHI::RenderGraph& graph, DrawInputParameters& passInput, DrawOutputParameters& passOutput)
    {
        bool needClearRenderTarget = initializeRenderTarget(graph, &passOutput);

        RHI::RgResourceHandle perframeBufferHandle = passInput.perframeBufferHandle;
        RHI::RgResourceHandle normalHandle = passInput.worldNormalHandle;
        RHI::RgResourceHandle depthHandle  = passInput.depthHandle;
        RHI::RgResourceHandle viewSpaceDepthHandle  = passOutput.outputViewDepthHandle;
        RHI::RgResourceHandle outAOHandle  = passOutput.outputAOHandle;

        RHI::RgResourceHandle gtaoConstantsHandle = graph.Import<RHI::D3D12Buffer>(pGTAOConstants.get());

        RHI::RenderPass& prefilterpass = graph.AddRenderPass("GTAOPrefilterPass");

        prefilterpass.Read(gtaoConstantsHandle, false, RHIResourceState::RHI_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
        prefilterpass.Read(passInput.depthHandle, false, RHIResourceState::RHI_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        prefilterpass.Write(viewSpaceDepthHandle, false, RHIResourceState::RHI_RESOURCE_STATE_UNORDERED_ACCESS);

        prefilterpass.Execute([=](RHI::RenderGraphRegistry* registry, RHI::D3D12CommandContext* context) {
            
            RHI::D3D12Buffer*  _GTAOBuffer = RegGetBuf(gtaoConstantsHandle);
            RHI::D3D12Texture* _SrcDepthTexture = RegGetTex(depthHandle);
            RHI::D3D12Texture* _DstViewSpaceDepthTexture = RegGetTex(viewSpaceDepthHandle);

            RHI::D3D12ComputeContext* pContext = context->GetComputeContext();

            RHI::D3D12ConstantBufferView* gtaoCBV = _GTAOBuffer->GetDefaultCBV().get();
            RHI::D3D12ShaderResourceView* depthSRV = _SrcDepthTexture->GetDefaultSRV().get();
            
            #define ViewSpaceDepthUAVIndex(idx) \
    _DstViewSpaceDepthTexture->CreateUAV(RHI::D3D12UnorderedAccessView::GetDesc(_DstViewSpaceDepthTexture, 0, idx)) \
        ->GetIndex()

            __declspec(align(16)) struct
            {
                uint32_t gitao_consts_index;
                uint32_t srcRawDepth_index;
                uint32_t outWorkingDepthMIP0_index;
                uint32_t outWorkingDepthMIP1_index;
                uint32_t outWorkingDepthMIP2_index;
                uint32_t outWorkingDepthMIP3_index;
                uint32_t outWorkingDepthMIP4_index;
            } GTAOConstantBuffer = {gtaoCBV->GetIndex(),
                                    depthSRV->GetIndex(),
                                    ViewSpaceDepthUAVIndex(0),
                                    ViewSpaceDepthUAVIndex(1),
                                    ViewSpaceDepthUAVIndex(2),
                                    ViewSpaceDepthUAVIndex(3),
                                    ViewSpaceDepthUAVIndex(4)};

            pContext->SetRootSignature(pDepthPrefilterSignature.get());
            pContext->SetPipelineState(pDepthPrefilterPSO.get());
            pContext->SetConstantArray(0, 0, &GTAOConstantBuffer);
            
            //pContext->Dispatch2D(depthTexDesc.Width, depthTexDesc.Height, 8, 8);
        });

        //====================================================================================================

        //RHI::RenderPass& mainpass = graph.AddRenderPass("GTAOMainPass");


    }

    void GTAOPass::destroy()
    {
        pGTAOConstants = nullptr;
    }

    bool GTAOPass::initializeRenderTarget(RHI::RenderGraph& graph, DrawOutputParameters* drawPassOutput)
    {
        bool needClearRenderTarget = false;
        if (!drawPassOutput->outputAOHandle.IsValid())
        {
            needClearRenderTarget = true;

            RHI::RgTextureDesc _targetAODesc = colorTexDesc;

            _targetAODesc.Name = "AOTex";
            _targetAODesc.SetFormat(DXGI_FORMAT_R32G32B32A32_FLOAT);
            _targetAODesc.SetAllowUnorderedAccess(true);
            
            drawPassOutput->outputAOHandle = graph.Create<RHI::D3D12Texture>(_targetAODesc);
        }
        if (!drawPassOutput->outputViewDepthHandle.IsValid())
        {
            RHI::RgTextureDesc _viewDepthPyramidDesc = depthTexDesc;

            _viewDepthPyramidDesc.Name = "ViewDepthPyramid";
            _viewDepthPyramidDesc.SetFormat(DXGI_FORMAT_R16_FLOAT);
            _viewDepthPyramidDesc.SetAllowUnorderedAccess(true);
            _viewDepthPyramidDesc.SetAllowDepthStencil(false);
            _viewDepthPyramidDesc.SetMipLevels(5);

            drawPassOutput->outputViewDepthHandle = graph.Create<RHI::D3D12Texture>(_viewDepthPyramidDesc);
        }

        return false;
    }

}
