#include "runtime/function/render/renderer/postprocess_passes.h"
#include "runtime/function/render/rhi/d3d12/d3d12_graphicsCommon.h"
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
        if (EngineConfig::g_BloomConfig.m_BloomEnable)
        {
            BloomPass::BloomInitInfo bloomPassInit;
            bloomPassInit.m_ColorTexDesc  = colorTexDesc;
            bloomPassInit.m_ShaderCompiler = init_info.m_ShaderCompiler;
            bloomPassInit.m_ShaderRootPath = g_runtime_global_context.m_config_manager->getShaderFolder();

            mBloomPass = std::make_shared<BloomPass>();
            mBloomPass->setCommonInfo(renderPassCommonInfo);
            mBloomPass->initialize(bloomPassInit);
        }
        else
        {
            ExtractLumaPass::ExtractLumaInitInfo extractLumaPassInit;
            extractLumaPassInit.m_ColorTexDesc = colorTexDesc;
            extractLumaPassInit.m_ShaderCompiler = init_info.m_ShaderCompiler;
            extractLumaPassInit.m_ShaderRootPath = g_runtime_global_context.m_config_manager->getShaderFolder();

            mExtractLumaPass = std::make_shared<ExtractLumaPass>();
            mExtractLumaPass->setCommonInfo(renderPassCommonInfo);
            mExtractLumaPass->initialize(extractLumaPassInit);
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
#define Create2DTex(width, height, format, name) \
    RHI::D3D12Texture::Create2D(m_Device->GetLinkedDevice(), \
                                width, \
                                height, \
                                1, \
                                format, \
                                RHI::RHISurfaceCreateRandomWrite, \
                                1, \
                                name, \
                                D3D12_RESOURCE_STATE_COMMON)

            m_LumaBuffer = Create2DTex(colorTexDesc.Width, colorTexDesc.Height, DXGI_FORMAT_R8_UINT, L"LuminanceColor");

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

            m_ExposureBuffer = RHI::D3D12Buffer::Create(m_Device->GetLinkedDevice(),
                                                        RHI::RHIBufferTarget::RHIBufferRandomReadWrite,
                                                        8,
                                                        4,
                                                        L"Exposure",
                                                        RHI::RHIBufferMode::RHIBufferModeImmutable,
                                                        D3D12_RESOURCE_STATE_GENERIC_READ,
                                                        (BYTE*)initExposure,
                                                        sizeof(initExposure));
        }

    }

    void PostprocessPasses::update(RHI::RenderGraph& graph, PostprocessInputParameters& passInput, PostprocessOutputParameters& passOutput)
    {
        //PostprocessInputParameters*  drawPassInput  = &passInput;
        //PostprocessOutputParameters* drawPassOutput = &passOutput;

        RHI::RgResourceHandle inputSceneColorHandle = passInput.inputSceneColorHandle;

        RHI::RgResourceHandle postTargetColorHandle0 = graph.Create<RHI::D3D12Texture>(colorTexDesc);
        RHI::RgResourceHandle postTargetColorHandle1 = graph.Create<RHI::D3D12Texture>(colorTexDesc);

        //initializeResolveTarget(graph, drawPassOutput);

        RHI::RgResourceHandle exposureBufferHandle = graph.Import(m_ExposureBuffer.get());
        RHI::RgResourceHandle lumaBufferHandle     = graph.Import(m_LumaBuffer.get());

        RHI::RgResourceHandle outputTargetHandle = inputSceneColorHandle;

        // resolve rendertarget
        if (EngineConfig::g_AntialiasingMode == EngineConfig::MSAA)
        {
            MSAAResolvePass::DrawInputParameters  mMSAAResolveIntputParams;
            MSAAResolvePass::DrawOutputParameters mMSAAResolveOutputParams;

            mMSAAResolveIntputParams.renderTargetColorHandle  = inputSceneColorHandle;
            mMSAAResolveOutputParams.resolveTargetColorHandle = postTargetColorHandle0;
            
            mResolvePass->update(graph, mMSAAResolveIntputParams, mMSAAResolveOutputParams);

            outputTargetHandle = mMSAAResolveOutputParams.resolveTargetColorHandle;
        }
        else if (EngineConfig::g_AntialiasingMode == EngineConfig::FXAA)
        {
            FXAAPass::DrawInputParameters  mFXAAIntputParams;
            FXAAPass::DrawOutputParameters mFXAAOutputParams;

            mFXAAIntputParams.inputLumaColorHandle  = lumaBufferHandle;
            mFXAAIntputParams.inputSceneColorHandle = inputSceneColorHandle;
            mFXAAOutputParams.outputColorHandle     = postTargetColorHandle0;

            mFXAAPass->update(graph, mFXAAIntputParams, mFXAAOutputParams);

            outputTargetHandle = mFXAAOutputParams.outputColorHandle;
        }
        else
        {
            //RHI::RgResourceHandle mTmpTargetColorHandle = drawPassOutput->postTargetColorHandle0;

            Blit(graph, inputSceneColorHandle, postTargetColorHandle0);

            outputTargetHandle = postTargetColorHandle0;
        }
        
        {
            RHI::RgResourceHandle m_BllomHandle;  // Output Bloom Texture
            RHI::RgResourceHandle m_LumaLRHandle; // LumaLR Texture

            if (EngineConfig::g_BloomConfig.m_BloomEnable)
            {
                // Bloom
                BloomPass::DrawInputParameters  mBloomIntputParams;
                BloomPass::DrawOutputParameters mBloomOutputParams;

                mBloomIntputParams.inputExposureHandle   = exposureBufferHandle;
                mBloomIntputParams.inputSceneColorHandle = outputTargetHandle;

                mBloomPass->update(graph, mBloomIntputParams, mBloomOutputParams);

                m_BllomHandle  = mBloomOutputParams.outputBloomHandle;
                m_LumaLRHandle = mBloomOutputParams.outputLumaLRHandle;
            }
            else if (EngineConfig::g_HDRConfig.m_EnableAdaptiveExposure)
            {
                // ExtractLuma
                ExtractLumaPass::DrawInputParameters  mExtractLumaIntputParams;
                ExtractLumaPass::DrawOutputParameters mExtractLumaOutputParams;

                mExtractLumaIntputParams.inputExposureHandle   = exposureBufferHandle;
                mExtractLumaIntputParams.inputSceneColorHandle = outputTargetHandle;

                mExtractLumaPass->update(graph, mExtractLumaIntputParams, mExtractLumaOutputParams);

                m_BllomHandle  = graph.Import(RHI::GetDefaultTexture(RHI::kBlackOpaque2D));
                m_LumaLRHandle = mExtractLumaOutputParams.outputLumaLRHandle;
            }

            // ToneMapping
            HDRToneMappingPass::DrawInputParameters  mToneMappingIntputParams;
            HDRToneMappingPass::DrawOutputParameters mToneMappingOutputParams;
            
            mToneMappingIntputParams.inputBloomHandle        = m_BllomHandle;
            mToneMappingIntputParams.inputExposureHandle     = exposureBufferHandle;
            mToneMappingIntputParams.inputSceneColorHandle   = outputTargetHandle;
            mToneMappingOutputParams.outputPostEffectsHandle = postTargetColorHandle1;

            mHDRToneMappingPass->update(graph, mToneMappingIntputParams, mToneMappingOutputParams);

            outputTargetHandle = mToneMappingOutputParams.outputPostEffectsHandle;
            
            // Exposure -- Do this last so that the bright pass uses the same exposure as tone mapping
            ExposurePass::DrawInputParameters  mExposureIntputParams;
            ExposurePass::DrawOutputParameters mExposureOutputParams;

            mExposureIntputParams.inputLumaLRHandle = m_LumaLRHandle;

            //exposureBufferHandle.Version += 1; // 因为后续的pass输出与前面的pass输入重合，所以需要version来区分
            mExposureOutputParams.exposureHandle = exposureBufferHandle;

            mExposurePass->update(graph, mExposureIntputParams, mExposureOutputParams);
            
        }
        
        passOutput.outputColorHandle = outputTargetHandle;
    }

    void PostprocessPasses::destroy() {}

    //bool PostprocessPasses::initializeResolveTarget(RHI::RenderGraph& graph, PostprocessOutputParameters* drawPassOutput)
    //{
    //    if (!drawPassOutput->postTargetColorHandle0.IsValid())
    //    {
    //        drawPassOutput->postTargetColorHandle0 = graph.Create<RHI::D3D12Texture>(colorTexDesc);
    //    }
    //    if (!drawPassOutput->postTargetColorHandle1.IsValid())
    //    {
    //        drawPassOutput->postTargetColorHandle1 = graph.Create<RHI::D3D12Texture>(colorTexDesc);
    //    }
    //    return true;
    //}

    void PostprocessPasses::Blit(RHI::RenderGraph& graph, RHI::RgResourceHandle& inputHandle, RHI::RgResourceHandle& outputHandle)
    {
        RHI::RenderPass& blitPass = graph.AddRenderPass("BlitPass");

        blitPass.Read(inputHandle, true);
        blitPass.Write(outputHandle, true);

        RHI::RgResourceHandle Blit_inputHandle = inputHandle;
        RHI::RgResourceHandle Blit_outputHandle = outputHandle;

        blitPass.Execute([=](RHI::RenderGraphRegistry* registry, RHI::D3D12CommandContext* context) {
            // RHI::D3D12ComputeContext* computeContext = context->GetComputeContext();

            RHI::D3D12Texture* pInputColor = registry->GetD3D12Texture(Blit_inputHandle);
            RHI::D3D12Texture* pTempColor  = registry->GetD3D12Texture(Blit_outputHandle);

            context->TransitionBarrier(pInputColor, D3D12_RESOURCE_STATE_COPY_SOURCE);
            context->TransitionBarrier(pTempColor, D3D12_RESOURCE_STATE_COPY_DEST);
            context->FlushResourceBarriers();

            context->CopySubresource(pTempColor, 0, pInputColor, 0);
        });
    }

}