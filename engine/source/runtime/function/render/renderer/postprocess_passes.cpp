#include "runtime/function/render/renderer/postprocess_passes.h"

#include "runtime/resource/config_manager/config_manager.h"

#include "runtime/function/render/rhi/rhi_core.h"

namespace Pilot
{

	void PostprocessPasses::initialize(const PostprocessInitInfo& init_info)
    {
        colorTexDesc = init_info.colorTexDesc;
        colorTexDesc.SetAllowRenderTarget(false);

        RenderPassCommonInfo renderPassCommonInfo = {m_RenderGraphAllocator, m_RenderGraphRegistry, m_Device, m_WindowSystem};

        {
            mLuminanceColor = RHI::D3D12Texture::Create2D(m_Device->GetLinkedDevice(),
                                                          colorTexDesc.Width,
                                                          colorTexDesc.Height,
                                                          1,
                                                          DXGI_FORMAT_R8_UINT,
                                                          RHI::RHISurfaceCreateRandomWrite,
                                                          1,
                                                          L"LuminanceColor",
                                                          D3D12_RESOURCE_STATE_COMMON);

        }
        {
            MSAAResolvePass::MSAAResolveInitInfo resolvePassInit;
            resolvePassInit.colorTexDesc = colorTexDesc;

            mResolvePass = std::make_shared<MSAAResolvePass>();
            mResolvePass->setCommonInfo(renderPassCommonInfo);
            mResolvePass->initialize(resolvePassInit);
        }
        {
            FXAAPass::FXAAInitInfo fxaaPassInit;
            fxaaPassInit.m_ColorTexDesc   = colorTexDesc;
            fxaaPassInit.m_FXAAConfig     = EngineConfig::g_FXAAConfig;
            fxaaPassInit.m_ShaderCompiler = init_info.m_ShaderCompiler;
            fxaaPassInit.m_ShaderRootPath = g_runtime_global_context.m_config_manager->getShaderFolder();

            mFXAAPass = std::make_shared<FXAAPass>();
            mFXAAPass->setCommonInfo(renderPassCommonInfo);
            mFXAAPass->initialize(fxaaPassInit);
        }
        {
            HDRToneMappingPass::HDRToneMappingInitInfo toneMappingInit;
            toneMappingInit.m_ColorTexDesc = colorTexDesc;
            toneMappingInit.m_HDRConfig    = EngineConfig::g_HDRConfig;
            toneMappingInit.m_ShaderCompiler = init_info.m_ShaderCompiler;
            toneMappingInit.m_ShaderRootPath = g_runtime_global_context.m_config_manager->getShaderFolder();

            mHDRToneMappingPass = std::make_shared<HDRToneMappingPass>();
            mHDRToneMappingPass->setCommonInfo(renderPassCommonInfo);
            mHDRToneMappingPass->initialize(toneMappingInit);
        }
        {
            ExposurePass::ExposureInitInfo exposureInit;
            exposureInit.m_ColorTexDesc   = colorTexDesc;
            exposureInit.m_HDRConfig      = EngineConfig::g_HDRConfig;
            exposureInit.m_ShaderCompiler = init_info.m_ShaderCompiler;
            exposureInit.m_ShaderRootPath = g_runtime_global_context.m_config_manager->getShaderFolder();

            mExposurePass = std::make_shared<ExposurePass>();
            mExposurePass->setCommonInfo(renderPassCommonInfo);
            mExposurePass->initialize(exposureInit);
        }

        {
            __declspec(align(16)) float initExposure[] = {
                EngineConfig::g_HDRConfig.m_Exposure,
                1.0f / EngineConfig::g_HDRConfig.m_Exposure,
                EngineConfig::g_HDRConfig.m_Exposure,
                0.0f,
                EngineConfig::kInitialMinLog,
                EngineConfig::kInitialMaxLog,
                EngineConfig::kInitialMaxLog - EngineConfig::kInitialMinLog,
                1.0f / (EngineConfig::kInitialMaxLog - EngineConfig::kInitialMinLog)};

            mExposureBuffer = RHI::D3D12Buffer::Create(m_Device->GetLinkedDevice(),
                                                       RHI::RHIBufferTarget::RHIBufferTargetNone,
                                                       8,
                                                       4,
                                                       L"Exposure",
                                                       RHI::RHIBufferMode::RHIBufferModeDynamic,
                                                       D3D12_RESOURCE_STATE_GENERIC_READ,
                                                       (BYTE*)initExposure,
                                                       sizeof(initExposure));
        }

    }

    void PostprocessPasses::update(RHI::RenderGraph& graph, PostprocessInputParameters& passInput, PostprocessOutputParameters& passOutput)
    {
        PostprocessInputParameters*  drawPassInput  = &passInput;
        PostprocessOutputParameters* drawPassOutput = &passOutput;

        RHI::RgResourceHandle inputSceneColorHandle = drawPassInput->inputSceneColorHandle;

        initializeResolveTarget(graph, drawPassOutput);

        RHI::RgResourceHandle outputTargetHandle = inputSceneColorHandle;

        RHI::RgResourceHandle exposureBufferHandle = graph.Import(mExposureBuffer.get());
        RHI::RgResourceHandle luminanceColorHandle = graph.Import(mLuminanceColor.get());

        // resolve rendertarget
        if (EngineConfig::g_AntialiasingMode == EngineConfig::MSAA)
        {
            MSAAResolvePass::DrawInputParameters  mMSAAResolveIntputParams;
            MSAAResolvePass::DrawOutputParameters mMSAAResolveOutputParams;

            mMSAAResolveIntputParams.renderTargetColorHandle  = inputSceneColorHandle;
            mMSAAResolveOutputParams.resolveTargetColorHandle = drawPassOutput->postTargetColorHandle0;
            
            mResolvePass->update(graph, mMSAAResolveIntputParams, mMSAAResolveOutputParams);

            outputTargetHandle = mMSAAResolveOutputParams.resolveTargetColorHandle;
        }
        else if (EngineConfig::g_AntialiasingMode == EngineConfig::FXAA)
        {
            FXAAPass::DrawInputParameters  mFXAAIntputParams;
            FXAAPass::DrawOutputParameters mFXAAOutputParams;

            mFXAAIntputParams.inputLumaColorHandle  = luminanceColorHandle;
            mFXAAIntputParams.inputSceneColorHandle = inputSceneColorHandle;
            mFXAAOutputParams.outputColorHandle     = drawPassOutput->postTargetColorHandle0;

            mFXAAPass->update(graph, mFXAAIntputParams, mFXAAOutputParams);

            outputTargetHandle = mFXAAOutputParams.outputColorHandle;
        }
        else
        {
            RHI::RgResourceHandle mTmpTargetColorHandle = drawPassOutput->postTargetColorHandle0;

            RHI::RenderPass& blitPass = graph.AddRenderPass("BlitPass");

            blitPass.Read(inputSceneColorHandle, true);
            blitPass.Write(mTmpTargetColorHandle, true);

            blitPass.Execute([=](RHI::RenderGraphRegistry* registry, RHI::D3D12CommandContext* context) {
                // RHI::D3D12ComputeContext* computeContext = context->GetComputeContext();

                RHI::D3D12Texture* pInputColor = registry->GetD3D12Texture(inputSceneColorHandle);
                RHI::D3D12Texture* pTempColor  = registry->GetD3D12Texture(mTmpTargetColorHandle);

                context->TransitionBarrier(pInputColor, D3D12_RESOURCE_STATE_COPY_SOURCE);
                context->TransitionBarrier(pTempColor, D3D12_RESOURCE_STATE_COPY_DEST);
                context->FlushResourceBarriers();

                context->CopySubresource(pTempColor, 0, pInputColor, 0);
            });

            outputTargetHandle = mTmpTargetColorHandle;
        }

        {
            // ToneMapping
            HDRToneMappingPass::DrawInputParameters  mToneMappingIntputParams;
            HDRToneMappingPass::DrawOutputParameters mToneMappingOutputParams;
            
            mToneMappingIntputParams.inputSceneColorHandle   = drawPassOutput->postTargetColorHandle0;
            mToneMappingOutputParams.outputPostEffectsHandle = drawPassOutput->postTargetColorHandle1;

            mHDRToneMappingPass->update(graph, mToneMappingIntputParams, mToneMappingOutputParams);

            outputTargetHandle = mToneMappingOutputParams.outputPostEffectsHandle;

            // Exposure
            ExposurePass::DrawInputParameters  mExposureIntputParams;
            ExposurePass::DrawOutputParameters mExposureOutputParams;

            mExposureIntputParams.inputLumaLRHandle = mToneMappingOutputParams.outputLumaHandle;
            mExposureOutputParams.exposureHandle    = exposureBufferHandle;

            // Do this last so that the bright pass uses the same exposure as tone mapping
            mExposurePass->update(graph, mExposureIntputParams, mExposureOutputParams);
        }

        passOutput.outputColorHandle = outputTargetHandle;
    }

    void PostprocessPasses::destroy() {}

    bool PostprocessPasses::initializeResolveTarget(RHI::RenderGraph& graph, PostprocessOutputParameters* drawPassOutput)
    {
        if (!drawPassOutput->postTargetColorHandle0.IsValid())
        {
            drawPassOutput->postTargetColorHandle0 = graph.Create<RHI::D3D12Texture>(colorTexDesc);
        }
        if (!drawPassOutput->postTargetColorHandle1.IsValid())
        {
            drawPassOutput->postTargetColorHandle1 = graph.Create<RHI::D3D12Texture>(colorTexDesc);
        }
        return true;
    }

}