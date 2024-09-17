#include "runtime/function/render/renderer/indirect_lightloop_pass.h"
#include "runtime/resource/config_manager/config_manager.h"
#include "runtime/function/render/rhi/rhi_core.h"
#include "runtime/function/render/renderer/pass_helper.h"

#include <cassert>

namespace MoYu
{

	void IndirectLightLoopPass::initialize(const DrawPassInitInfo& init_info)
	{
        colorTexDesc = init_info.colorTexDesc;

        ShaderCompiler*       m_ShaderCompiler = init_info.m_ShaderCompiler;
        std::filesystem::path m_ShaderRootPath = init_info.m_ShaderRootPath;

        indirectLightLoopCS = m_ShaderCompiler->CompileShader(
            RHI_SHADER_TYPE::Compute, m_ShaderRootPath / "pipeline/Runtime/Lighting/LightLoop/DeferredLightLoop.hlsl", ShaderCompileOptions(L"SHADE_OPAQUE_ENTRY"));

        {
            RHI::RootSignatureDesc rootSigDesc =
                RHI::RootSignatureDesc()
                    .Add32BitConstants<0, 0>(16)
                    .AddStaticSampler<10, 0>(D3D12_FILTER::D3D12_FILTER_MIN_MAG_MIP_POINT, D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_CLAMP, 8)
                    .AddStaticSampler<11, 0>(D3D12_FILTER::D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT, D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_CLAMP, 8)
                    .AddStaticSampler<12, 0>(D3D12_FILTER::D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT, D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_WRAP, 8)
                    .AddStaticSampler<13, 0>(D3D12_FILTER::D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_CLAMP, 8)
                    .AddStaticSampler<14, 0>(D3D12_FILTER::D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_WRAP, 8)
                    .AddStaticSampler<15, 0>(D3D12_FILTER::D3D12_FILTER_COMPARISON_ANISOTROPIC,
                                             D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
                                             8,
                                             D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_GREATER_EQUAL,
                                             D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK)
                    .AllowInputLayout()
                    .AllowResourceDescriptorHeapIndexing()
                    .AllowSampleDescriptorHeapIndexing();

            pIndirectLightLoopSignature = std::make_shared<RHI::D3D12RootSignature>(m_Device, rootSigDesc);
        }

        {
            struct PsoStream
            {
                PipelineStateStreamRootSignature RootSignature;
                PipelineStateStreamCS            CS;
            } psoStream;
            psoStream.RootSignature         = PipelineStateStreamRootSignature(pIndirectLightLoopSignature.get());
            psoStream.CS                    = &indirectLightLoopCS;
            PipelineStateStreamDesc psoDesc = {sizeof(PsoStream), &psoStream};

            pIndirectLightLoopPSO =
                std::make_shared<RHI::D3D12PipelineState>(m_Device, L"LightLoopPSO", psoDesc);
        }

	}

    void IndirectLightLoopPass::update(RHI::RenderGraph& graph, DrawInputParameters& passInput, DrawOutputParameters& passOutput)
    {
        bool needClearRenderTarget = initializeRenderTarget(graph, &passOutput);

        RHI::RenderPass& drawpass = graph.AddRenderPass("LightLoopPass");

        drawpass.Read(passInput.perframeBufferHandle, false, RHIResourceState::RHI_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
        drawpass.Read(passInput.gbuffer0Handle, false, RHIResourceState::RHI_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        drawpass.Read(passInput.gbuffer1Handle, false, RHIResourceState::RHI_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        drawpass.Read(passInput.gbuffer2Handle, false, RHIResourceState::RHI_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        drawpass.Read(passInput.gbuffer3Handle, false, RHIResourceState::RHI_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        drawpass.Read(passInput.gbufferDepthHandle, false, RHIResourceState::RHI_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        drawpass.Read(passInput.ssgiHandle, false, RHIResourceState::RHI_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        drawpass.Read(passInput.ssrHandle, false, RHIResourceState::RHI_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        drawpass.Read(passInput.mAOHandle, false, RHIResourceState::RHI_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

        drawpass.Read(passInput.directionalCascadeShadowmapHandle, false, RHIResourceState::RHI_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        for (int i = 0; i < passInput.spotShadowmapHandles.size(); i++)
        {
            drawpass.Read(passInput.spotShadowmapHandles[i], false, RHIResourceState::RHI_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        }
        drawpass.Write(passOutput.specularLightinghandle, false, RHIResourceState::RHI_RESOURCE_STATE_UNORDERED_ACCESS);

        RHI::RgResourceHandle perframeBufferHandle = passInput.perframeBufferHandle;
        RHI::RgResourceHandle gbuffer0Handle = passInput.gbuffer0Handle;
        RHI::RgResourceHandle gbuffer1Handle = passInput.gbuffer1Handle;
        RHI::RgResourceHandle gbuffer2Handle = passInput.gbuffer2Handle;
        RHI::RgResourceHandle gbuffer3Handle = passInput.gbuffer3Handle;
        
        RHI::RgResourceHandle specularLightinghandle = passOutput.specularLightinghandle;

        drawpass.Execute([=](RHI::RenderGraphRegistry* registry, RHI::D3D12CommandContext* context) {

            RHI::D3D12ComputeContext* pContext = context->GetComputeContext();

            RHI::D3D12Texture* pRenderTargetColor = registry->GetD3D12Texture(specularLightinghandle);

            pContext->ClearUAV(pRenderTargetColor);

            pContext->SetRootSignature(pIndirectLightLoopSignature.get());
            pContext->SetPipelineState(pIndirectLightLoopPSO.get());

            struct RootIndexBuffer
            {
                uint32_t frameUniformIndex;
                uint32_t inGBuffer0Index;
                uint32_t inGBuffer1Index;
                uint32_t inGBuffer2Index;
                uint32_t inGBuffer3Index;
                uint32_t specularLightingUAVIndex; // float4
            };

            RootIndexBuffer rootIndexBuffer = {RegGetBufDefCBVIdx(perframeBufferHandle),
                                               RegGetTexDefSRVIdx(gbuffer0Handle),
                                               RegGetTexDefSRVIdx(gbuffer1Handle),
                                               RegGetTexDefSRVIdx(gbuffer2Handle),
                                               RegGetTexDefSRVIdx(gbuffer3Handle),
                                               RegGetTexDefUAVIdx(specularLightinghandle)};

            pContext->SetConstantArray(0, sizeof(RootIndexBuffer) / sizeof(UINT), &rootIndexBuffer);

            pContext->Dispatch2D(colorTexDesc.Width, colorTexDesc.Height, 8, 8);

        });
    }


    void IndirectLightLoopPass::destroy()
    {


    }

    bool IndirectLightLoopPass::initializeRenderTarget(RHI::RenderGraph& graph, DrawOutputParameters* drawPassOutput)
    {
        bool needClearRenderTarget = false;
        if (!drawPassOutput->specularLightinghandle.IsValid())
        {
            needClearRenderTarget = true;

            RHI::RgTextureDesc targetColorDesc = colorTexDesc;
            targetColorDesc.SetAllowUnorderedAccess(true);
            targetColorDesc.SetAllowRenderTarget(true);
            targetColorDesc.Name = "LightLoopColor";

            drawPassOutput->specularLightinghandle = graph.Create<RHI::D3D12Texture>(targetColorDesc);
        }
        return needClearRenderTarget;
    }

}
