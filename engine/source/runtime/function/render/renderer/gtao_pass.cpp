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
            RHI_SHADER_TYPE::Compute, m_ShaderRootPath / "pipeline/Runtime/Tools/GTAO/vaGTAOPrefilter.hlsl", ShaderCompileOptions(L"CSPrefilterDepths16x16"));

        GTAOCS = m_ShaderCompiler->CompileShader(
            RHI_SHADER_TYPE::Compute, m_ShaderRootPath / "pipeline/Runtime/Tools/GTAO/vaGTAOMain.hlsl", ShaderCompileOptions(L"CSGTAOHigh"));

        DenoiseCS = m_ShaderCompiler->CompileShader(
            RHI_SHADER_TYPE::Compute, m_ShaderRootPath / "pipeline/Runtime/Tools/GTAO/vaGTAODenoise.hlsl", ShaderCompileOptions(L"CSDenoisePass"));

        {
            RHI::RootSignatureDesc rootSigDesc =
                RHI::RootSignatureDesc()
                    .Add32BitConstants<0, 0>(16)
                    .AddStaticSampler<10, 0>(D3D12_FILTER::D3D12_FILTER_MIN_MAG_MIP_POINT,
                                             D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
                                             4)
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
                                             4)
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

    void GTAOPass::prepareMatBuffer(std::shared_ptr<RenderResource> render_resource)
    {
        // GTAO UniformBuffer
        float aoClearColor[4] = { 1,1,1,1 };

        RHI::RHIRenderSurfaceBaseDesc rtDesc{};
        rtDesc.width = colorTexDesc.Width;
        rtDesc.height = colorTexDesc.Height;
        rtDesc.depthOrArray = 1;
        rtDesc.samples = 1;
        rtDesc.mipCount = 1;
        rtDesc.flags = RHI::RHISurfaceCreateRandomWrite;
        rtDesc.dim = RHI::RHITextureDimension::RHITexDim2D;
        rtDesc.graphicsFormat = DXGI_FORMAT_R32_FLOAT;
        rtDesc.colorSurface = true;
        rtDesc.backBuffer = false;
        pGTAORenderTexture = render_resource->CreateTransientTexture(rtDesc, L"GTAOTexture", D3D12_RESOURCE_STATE_COMMON);

        HLSL::FrameUniforms* _frameUniforms = &render_resource->m_FrameUniforms;

        _frameUniforms->baseUniform._AmbientOcclusionTextureIndex = pGTAORenderTexture->GetDefaultSRV()->GetIndex();

        HLSL::AOUniform aoUniform{};
        aoUniform._AmbientOcclusionParam = glm::float4(0, 0, 0, 1);
        aoUniform._SpecularOcclusionBlend = 1;

        _frameUniforms->aoUniform = aoUniform;


        // GTAO Parameters
        int viewportWidth = m_render_camera->m_pixelWidth;
        int viewportHeight = m_render_camera->m_pixelHeight;
        glm::float4x4 proj = m_render_camera->getProjMatrix(true);
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

        RHI::RgResourceHandle finalAOHandle = graph.Import<RHI::D3D12Texture>(pGTAORenderTexture.get());

        RHI::RgResourceHandle perframeBufferHandle = passInput.perframeBufferHandle;
        RHI::RgResourceHandle packedNormalHandle = passInput.packedNormalHandle;
        RHI::RgResourceHandle depthHandle  = passInput.depthHandle;
        //RHI::RgResourceHandle finalAOHandle  = passOutput.outputAOHandle;

        RHI::RgResourceHandle workingViewDepthHandle = passOutput.workingViewDepthHandle;
        RHI::RgResourceHandle workingEdgeHandle = passOutput.workingEdgeHandle;
        RHI::RgResourceHandle workingAOHandle   = passOutput.workingAOHandle;

        RHI::RgResourceHandle gtaoConstantsHandle = graph.Import<RHI::D3D12Buffer>(pGTAOConstants.get());

        RHI::RenderPass& prefilterpass = graph.AddRenderPass("GTAOPrefilterPass");

        prefilterpass.Read(gtaoConstantsHandle, false, RHIResourceState::RHI_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
        prefilterpass.Read(depthHandle, false, RHIResourceState::RHI_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        prefilterpass.Write(workingViewDepthHandle, false, RHIResourceState::RHI_RESOURCE_STATE_UNORDERED_ACCESS);

        prefilterpass.Execute([=](RHI::RenderGraphRegistry* registry, RHI::D3D12CommandContext* context) {
            
            RHI::D3D12Buffer*  _GTAOBuffer = RegGetBuf(gtaoConstantsHandle);
            RHI::D3D12Texture* _SrcDepthTexture = RegGetTex(depthHandle);
            RHI::D3D12Texture* _DstViewSpaceDepthTexture = RegGetTex(workingViewDepthHandle);

            RHI::D3D12ComputeContext* pContext = context->GetComputeContext();

            RHI::D3D12ConstantBufferView* gtaoCBV = _GTAOBuffer->GetDefaultCBV().get();
            RHI::D3D12ShaderResourceView* depthSRV = _SrcDepthTexture->GetDefaultSRV().get();
            
            #define ViewSpaceDepthUAVDesc(idx) RHI::D3D12UnorderedAccessView::GetDesc(_DstViewSpaceDepthTexture, 0, idx)
            #define ViewSpaceDepthUAVIndex(idx) _DstViewSpaceDepthTexture->CreateUAV(ViewSpaceDepthUAVDesc(idx))->GetIndex()

            __declspec(align(16)) struct
            {
                uint32_t gitao_consts_index;
                uint32_t srcRawDepth_index;
                uint32_t outWorkingDepthMIP0_index;
                uint32_t outWorkingDepthMIP1_index;
                uint32_t outWorkingDepthMIP2_index;
                uint32_t outWorkingDepthMIP3_index;
                uint32_t outWorkingDepthMIP4_index;
            } gtaoConstantBuffer = {gtaoCBV->GetIndex(),
                                    depthSRV->GetIndex(),
                                    ViewSpaceDepthUAVIndex(0),
                                    ViewSpaceDepthUAVIndex(1),
                                    ViewSpaceDepthUAVIndex(2),
                                    ViewSpaceDepthUAVIndex(3),
                                    ViewSpaceDepthUAVIndex(4)};

            pContext->SetRootSignature(pDepthPrefilterSignature.get());
            pContext->SetPipelineState(pDepthPrefilterPSO.get());
            pContext->SetConstantArray(0, sizeof(gtaoConstantBuffer) / sizeof(uint32_t), &gtaoConstantBuffer);
            
            uint32_t dispatchWidth = depthTexDesc.Width;
            uint32_t dispatchHeight = depthTexDesc.Height;

            pContext->Dispatch((dispatchWidth + 16 - 1) / 16, (dispatchHeight + 16 - 1) / 16, 1);
        });
        
        //====================================================================================================
         
        RHI::RenderPass& mainpass = graph.AddRenderPass("GTAOMainPass");

        mainpass.Read(perframeBufferHandle, false, RHIResourceState::RHI_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
        mainpass.Read(gtaoConstantsHandle, false, RHIResourceState::RHI_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
        mainpass.Read(workingViewDepthHandle, false, RHIResourceState::RHI_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        mainpass.Read(packedNormalHandle, false, RHIResourceState::RHI_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        mainpass.Write(workingEdgeHandle, false, RHIResourceState::RHI_RESOURCE_STATE_UNORDERED_ACCESS);
        mainpass.Write(workingAOHandle, false, RHIResourceState::RHI_RESOURCE_STATE_UNORDERED_ACCESS);

        mainpass.Execute([=](RHI::RenderGraphRegistry* registry, RHI::D3D12CommandContext* context) {
            RHI::D3D12Buffer*  _PerFrameBuffer        = RegGetBuf(perframeBufferHandle);
            RHI::D3D12Buffer*  _GTAOBuffer            = RegGetBuf(gtaoConstantsHandle);
            RHI::D3D12Texture* _ViewSpaceDepthTexture = RegGetTex(workingViewDepthHandle);
            RHI::D3D12Texture* _PackedNormalTexture   = RegGetTex(packedNormalHandle);
            RHI::D3D12Texture* _EdgeTexture           = RegGetTex(workingEdgeHandle);
            RHI::D3D12Texture* _GTAOTexture           = RegGetTex(workingAOHandle);

            RHI::D3D12ComputeContext* pContext = context->GetComputeContext();

            RHI::D3D12ConstantBufferView*  perframeCBV     = _PerFrameBuffer->GetDefaultCBV().get();
            RHI::D3D12ConstantBufferView*  gtaoCBV         = _GTAOBuffer->GetDefaultCBV().get();
            RHI::D3D12ShaderResourceView*  viewDepthSRV    = _ViewSpaceDepthTexture->GetDefaultSRV().get();
            RHI::D3D12ShaderResourceView*  packedNormalSRV = _PackedNormalTexture->GetDefaultSRV().get();
            RHI::D3D12UnorderedAccessView* edgeUAV         = _EdgeTexture->GetDefaultUAV().get();
            RHI::D3D12UnorderedAccessView* gtaoUAV         = _GTAOTexture->GetDefaultUAV().get();

            __declspec(align(16)) struct
            {
                uint32_t perframe_consts_index;
                uint32_t gitao_consts_index;
                uint32_t workingDepth_index;
                uint32_t packedNormal_index;
                uint32_t outWorkingAOTerm_index;
                uint32_t outWorkingEdges_index;
            } gtaoConstantBuffer = {perframeCBV->GetIndex(),
                                    gtaoCBV->GetIndex(),
                                    viewDepthSRV->GetIndex(),
                                    packedNormalSRV->GetIndex(),
                                    gtaoUAV->GetIndex(),
                                    edgeUAV->GetIndex()};

            pContext->SetRootSignature(pGTAOSignature.get());
            pContext->SetPipelineState(pGTAOPSO.get());
            pContext->SetConstantArray(0, sizeof(gtaoConstantBuffer) / sizeof(uint32_t), &gtaoConstantBuffer);

            pContext->Dispatch2D(colorTexDesc.Width, colorTexDesc.Height, 8, 8);
        });
         
        //====================================================================================================
        
        RHI::RenderPass& denoisepass = graph.AddRenderPass("GTAODenoisePass");

        denoisepass.Read(gtaoConstantsHandle, false, RHIResourceState::RHI_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
        denoisepass.Read(workingAOHandle, false, RHIResourceState::RHI_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        denoisepass.Write(workingEdgeHandle, false, RHIResourceState::RHI_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        denoisepass.Write(finalAOHandle, false, RHIResourceState::RHI_RESOURCE_STATE_UNORDERED_ACCESS);

        denoisepass.Execute([=](RHI::RenderGraphRegistry* registry, RHI::D3D12CommandContext* context) {
            RHI::D3D12Buffer*  _GTAOBuffer         = RegGetBuf(gtaoConstantsHandle);
            RHI::D3D12Texture* _WorkingAOTexture   = RegGetTex(workingAOHandle);
            RHI::D3D12Texture* _WorkingEdgeTexture = RegGetTex(workingEdgeHandle);
            RHI::D3D12Texture* _FinalGTAOTexture   = RegGetTex(finalAOHandle);

            RHI::D3D12ComputeContext* pContext = context->GetComputeContext();

            RHI::D3D12ConstantBufferView*  gtaoCBV        = _GTAOBuffer->GetDefaultCBV().get();
            RHI::D3D12ShaderResourceView*  workingAOSRV   = _WorkingAOTexture->GetDefaultSRV().get();
            RHI::D3D12ShaderResourceView*  workingEdgeSRV = _WorkingEdgeTexture->GetDefaultSRV().get();
            RHI::D3D12UnorderedAccessView* finalAOUAV     = _FinalGTAOTexture->GetDefaultUAV().get();

            __declspec(align(16)) struct
            {
                uint32_t gitao_consts_index;
                uint32_t g_srcWorkingAOTerm_index;
                uint32_t g_srcWorkingEdges_index;
                uint32_t g_outFinalAOTerm_index;  
            } gtaoConstantBuffer = {gtaoCBV->GetIndex(),
                                    workingAOSRV->GetIndex(),
                                    workingEdgeSRV->GetIndex(),
                                    finalAOUAV->GetIndex()};

            pContext->SetRootSignature(pDenoiseSignature.get());
            pContext->SetPipelineState(pDenoisePSO.get());
            pContext->SetConstantArray(0, sizeof(gtaoConstantBuffer) / sizeof(uint32_t), &gtaoConstantBuffer);

            pContext->Dispatch2D(colorTexDesc.Width, colorTexDesc.Height, 8, 8);
        });
        
        passOutput.workingViewDepthHandle = workingViewDepthHandle;
        passOutput.workingEdgeHandle     = workingEdgeHandle;
        passOutput.workingAOHandle       = workingAOHandle;

        passOutput.outputAOHandle        = finalAOHandle;
    }

    void GTAOPass::destroy()
    {
        pGTAOConstants = nullptr;
    }

    bool GTAOPass::initializeRenderTarget(RHI::RenderGraph& graph, DrawOutputParameters* drawPassOutput)
    {
        bool needClearRenderTarget = false;
        //if (!drawPassOutput->outputAOHandle.IsValid())
        //{
        //    needClearRenderTarget = true;
        //    RHI::RgTextureDesc _targetAODesc = colorTexDesc;
        //    _targetAODesc.Name = "FinalGTAO";
        //    _targetAODesc.SetFormat(DXGI_FORMAT_R32_UINT);
        //    _targetAODesc.SetAllowUnorderedAccess(true);
        //    drawPassOutput->outputAOHandle = graph.Create<RHI::D3D12Texture>(_targetAODesc);
        //}
        if (!drawPassOutput->workingViewDepthHandle.IsValid())
        {
            RHI::RgTextureDesc _viewDepthPyramidDesc = depthTexDesc;

            _viewDepthPyramidDesc.Name = "ViewDepthPyramid";
            _viewDepthPyramidDesc.SetFormat(DXGI_FORMAT_R16_FLOAT);
            _viewDepthPyramidDesc.SetAllowUnorderedAccess(true);
            _viewDepthPyramidDesc.SetAllowDepthStencil(false);
            _viewDepthPyramidDesc.SetMipLevels(5);

            drawPassOutput->workingViewDepthHandle = graph.Create<RHI::D3D12Texture>(_viewDepthPyramidDesc);
        }
        if (!drawPassOutput->workingEdgeHandle.IsValid())
        {
            workingEdgeDesc      = colorTexDesc;
            workingEdgeDesc.Name = "WorkingEdge";
            workingEdgeDesc.SetFormat(DXGI_FORMAT_R32_FLOAT);
            workingEdgeDesc.SetAllowUnorderedAccess(true);
            workingEdgeDesc.SetAllowDepthStencil(false);

            drawPassOutput->workingEdgeHandle = graph.Create<RHI::D3D12Texture>(workingEdgeDesc);
        }
        if (!drawPassOutput->workingAOHandle.IsValid())
        {
            workingAODesc      = colorTexDesc;
            workingAODesc.Name = "WorkingGTAO";
            workingAODesc.SetFormat(DXGI_FORMAT_R32_UINT);
            workingAODesc.SetAllowUnorderedAccess(true);

            drawPassOutput->workingAOHandle = graph.Create<RHI::D3D12Texture>(workingAODesc);
        }

        return false;
    }

}
