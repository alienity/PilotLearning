#include "runtime/function/render/renderer/postprocess_passes.h"

#include "runtime/resource/config_manager/config_manager.h"

#include "runtime/function/render/rhi/rhi_core.h"

namespace Pilot
{

	void PostprocessPasses::initialize(const PostprocessInitInfo& init_info)
    {
        colorTexDesc = init_info.colorTexDesc;

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


    }

    void PostprocessPasses::update(RHI::RenderGraph& graph, PostprocessInputParameters& passInput, PostprocessOutputParameters& passOutput)
    {
        PostprocessInputParameters*  drawPassInput  = &passInput;
        PostprocessOutputParameters* drawPassOutput = &passOutput;

        RHI::RgResourceHandle targetHandle = drawPassInput->renderTargetColorHandle;

        // resolve rendertarget
        if (EngineConfig::g_AntialiasingMode == EngineConfig::MSAA)
        {
            initializeResolveTarget(graph, drawPassOutput);

            MSAAResolvePass::DrawInputParameters  mMSAAResolveIntputParams;
            MSAAResolvePass::DrawOutputParameters mMSAAResolveOutputParams;

            mMSAAResolveIntputParams.renderTargetColorHandle = targetHandle;
            mMSAAResolveOutputParams.resolveTargetColorHandle = drawPassOutput->postTargetColorHandle;
            
            mResolvePass->update(graph, mMSAAResolveIntputParams, mMSAAResolveOutputParams);

            //targetHandle = mMSAAResolveOutputParams.resolveTargetColorHandle;
        }
        else if (EngineConfig::g_AntialiasingMode == EngineConfig::FXAA)
        {
            initializeResolveTarget(graph, drawPassOutput);

            FXAAPass::DrawInputParameters  mFXAAIntputParams;
            FXAAPass::DrawOutputParameters mFXAAOutputParams;

            mFXAAIntputParams.inputColorHandle  = targetHandle;
            mFXAAOutputParams.targetColorHandle = drawPassOutput->postTargetColorHandle;

            mFXAAPass->update(graph, mFXAAIntputParams, mFXAAOutputParams);

            //targetHandle = mFXAAOutputParams.targetColorHandle;
        }
        else
        {
            drawPassOutput->postTargetColorHandle = targetHandle;
        }
    }

    void PostprocessPasses::destroy() {}

    bool PostprocessPasses::initializeResolveTarget(RHI::RenderGraph& graph, PostprocessOutputParameters* drawPassOutput)
    {
        if (!drawPassOutput->postTargetColorHandle.IsValid())
        {
            drawPassOutput->postTargetColorHandle = graph.Create<RHI::D3D12Texture>(colorTexDesc);
        }
        return true;
    }

}