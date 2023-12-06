#include "renderer_deferred.h"

#include "runtime/function/render/renderer/renderer_registry.h"
#include "runtime/function/global/global_context.h"
#include "runtime/resource/config_manager/config_manager.h"

#include "runtime/resource/basic_geometry/mesh_tools.h"
#include "runtime/resource/basic_geometry/icosphere_mesh.h"
#include "runtime/resource/basic_geometry/cube_mesh.h"

namespace MoYu
{
    DeferredRenderer::DeferredRenderer(RendererInitParams& renderInitParams) :
        Renderer(renderInitParams)
    {}

    void DeferredRenderer::Initialize()
    {
        RHI::D3D12Texture* pBackBufferResource = pSwapChain->GetCurrentBackBufferResource();
        
        CD3DX12_RESOURCE_DESC backDesc = pBackBufferResource->GetDesc();

        backBufferWidth   = backDesc.Width;
        backBufferHeight  = backDesc.Height;
        backBufferFormat  = backDesc.Format;
        depthBufferFormat = DXGI_FORMAT_D32_FLOAT;

        pipleineColorFormat = DXGI_FORMAT_R32G32B32A32_FLOAT;
        pipleineDepthFormat = DXGI_FORMAT_D32_FLOAT;

        SetViewPort(0, 0, backBufferWidth, backBufferHeight);

        std::shared_ptr<ConfigManager> config_manager = g_runtime_global_context.m_config_manager;
        const std::filesystem::path&   shaderRootPath = config_manager->getShaderFolder();

        // initialize global objects
        Shaders::Compile(pCompiler, shaderRootPath);
        RootSignatures::Compile(pDevice);
        CommandSignatures::Compile(pDevice);
        PipelineStates::Compile(pipleineColorFormat, pipleineDepthFormat, backBufferFormat, depthBufferFormat, pDevice);

        InitGlobalBuffer();
        InitPass();
    }

    void DeferredRenderer::InitGlobalBuffer()
    {
        const FLOAT         renderTargetClearColor[4] = {0, 0, 0, 0};
        CD3DX12_CLEAR_VALUE renderTargetClearValue    = CD3DX12_CLEAR_VALUE(backBufferFormat, renderTargetClearColor);

        p_RenderTargetTex = RHI::D3D12Texture::Create2D(pDevice->GetLinkedDevice(),
                                                        viewport.width,
                                                        viewport.height,
                                                        1,
                                                        backBufferFormat,
                                                        RHI::RHISurfaceCreateRenderTarget,
                                                        1,
                                                        L"RenderTargetTexture",
                                                        D3D12_RESOURCE_STATE_COMMON,
                                                        renderTargetClearValue);
    }

    void DeferredRenderer::InitPass()
    {
        RenderPassCommonInfo renderPassCommonInfo = {&renderGraphAllocator, &renderGraphRegistry, pDevice, pWindowSystem};

        int sampleCount = EngineConfig::g_AntialiasingMode == EngineConfig::MSAA ? EngineConfig::g_MASSConfig.m_MSAASampleCount : 1;

        // Prepare common resources
        RHI::RgTextureDesc colorTexDesc = RHI::RgTextureDesc("ColorBuffer")
                                              .SetFormat(pipleineColorFormat)
                                              .SetExtent(viewport.width, viewport.height)
                                              .SetAllowRenderTarget()
                                              .SetSampleCount(sampleCount)
                                              .SetClearValue(RHI::RgClearValue(0, 0, 0, 0));
        RHI::RgTextureDesc depthTexDesc = RHI::RgTextureDesc("DepthBuffer")
                                              .SetFormat(pipleineDepthFormat)
                                              .SetExtent(viewport.width, viewport.height)
                                              .SetAllowDepthStencil()
                                              .SetSampleCount(sampleCount)
                                              .SetClearValue(RHI::RgClearValue(0.0f, 0xff));

        // Tool pass
        {
            ToolPass::ToolPassInitInfo toolInitInfo;
            toolInitInfo.m_ShaderCompiler = pCompiler;

            mToolPass = std::make_shared<ToolPass>();
            mToolPass->setCommonInfo(renderPassCommonInfo);
            mToolPass->initialize(toolInitInfo);
        }

        // Cull pass
        {
            MoYu::IndirectCullPass::IndirectCullInitInfo indirectCullInit;
            indirectCullInit.albedoTexDesc    = colorTexDesc;
            indirectCullInit.depthTexDesc     = depthTexDesc;
            indirectCullInit.m_ShaderCompiler = pCompiler;
            indirectCullInit.m_ShaderRootPath = g_runtime_global_context.m_config_manager->getShaderFolder();

            mIndirectCullPass = std::make_shared<IndirectCullPass>();
            mIndirectCullPass->setCommonInfo(renderPassCommonInfo);
            mIndirectCullPass->initialize(indirectCullInit);
        }
        // Terrain Cull pass
        {
            TerrainCullInitInfo drawPassInit;
            drawPassInit.colorTexDesc     = colorTexDesc;
            drawPassInit.m_ShaderCompiler = pCompiler;
            drawPassInit.m_ShaderRootPath = g_runtime_global_context.m_config_manager->getShaderFolder();

            mTerrainCullPass = std::make_shared<IndirectTerrainCullPass>();
            mTerrainCullPass->setCommonInfo(renderPassCommonInfo);
            mTerrainCullPass->initialize(drawPassInit);
        }
        // GBuffer pass
        {
            IndirectGBufferPass::DrawPassInitInfo drawPassInit;
            drawPassInit.albedoTexDesc = colorTexDesc;
            drawPassInit.depthTexDesc = depthTexDesc;
            drawPassInit.m_ShaderCompiler = pCompiler;
            drawPassInit.m_ShaderRootPath = g_runtime_global_context.m_config_manager->getShaderFolder();

            mIndirectGBufferPass = std::make_shared<IndirectGBufferPass>();
            mIndirectGBufferPass->setCommonInfo(renderPassCommonInfo);
            mIndirectGBufferPass->initialize(drawPassInit);
        }
        // Terrain GBuffer pass
        {
            //mIndirectTerrainGBufferPass
            IndirectTerrainGBufferPass::DrawPassInitInfo drawPassInit;
            drawPassInit.albedoDesc = mIndirectGBufferPass->albedoDesc;
            drawPassInit.depthDesc  = mIndirectGBufferPass->depthDesc;
            drawPassInit.worldNormalDesc = mIndirectGBufferPass->worldNormalDesc;
            drawPassInit.worldTangentDesc = mIndirectGBufferPass->worldTangentDesc;
            drawPassInit.matNormalDesc    = mIndirectGBufferPass->matNormalDesc;
            drawPassInit.motionVectorDesc = mIndirectGBufferPass->motionVectorDesc;
            drawPassInit.metallic_Roughness_Reflectance_AO_Desc = mIndirectGBufferPass->metallic_Roughness_Reflectance_AO_Desc;
            drawPassInit.clearCoat_ClearCoatRoughness_AnisotropyDesc = mIndirectGBufferPass->clearCoat_ClearCoatRoughness_AnisotropyDesc;
            
            drawPassInit.m_ShaderCompiler = pCompiler;
            drawPassInit.m_ShaderRootPath = g_runtime_global_context.m_config_manager->getShaderFolder();

            mIndirectTerrainGBufferPass = std::make_shared<IndirectTerrainGBufferPass>();
            mIndirectTerrainGBufferPass->setCommonInfo(renderPassCommonInfo);
            mIndirectTerrainGBufferPass->initialize(drawPassInit);
        }
        // Depth Pyramid
        {
            DepthPyramidPass::DrawPassInitInfo depthDrawPassInit;
            depthDrawPassInit.albedoTexDesc = mIndirectGBufferPass->albedoDesc;
            depthDrawPassInit.depthTexDesc  = mIndirectGBufferPass->depthDesc;

            depthDrawPassInit.m_ShaderCompiler = pCompiler;
            depthDrawPassInit.m_ShaderRootPath = g_runtime_global_context.m_config_manager->getShaderFolder();

            mDepthPyramidPass = std::make_shared<DepthPyramidPass>();
            mDepthPyramidPass->setCommonInfo(renderPassCommonInfo);
            mDepthPyramidPass->initialize(depthDrawPassInit);
        }
        // LightLoop pass
        {
            IndirectLightLoopPass::DrawPassInitInfo drawPassInit;
            drawPassInit.colorTexDesc     = colorTexDesc;
            drawPassInit.m_ShaderCompiler = pCompiler;
            drawPassInit.m_ShaderRootPath = g_runtime_global_context.m_config_manager->getShaderFolder();

            mIndirectLightLoopPass = std::make_shared<IndirectLightLoopPass>();
            mIndirectLightLoopPass->setCommonInfo(renderPassCommonInfo);
            mIndirectLightLoopPass->initialize(drawPassInit);
        }
        // Opaque drawing pass
        {
            IndirectDrawPass::DrawPassInitInfo drawPassInit;
            drawPassInit.colorTexDesc = colorTexDesc;
            drawPassInit.depthTexDesc = depthTexDesc;

            mIndirectOpaqueDrawPass = std::make_shared<IndirectDrawPass>();
            mIndirectOpaqueDrawPass->setCommonInfo(renderPassCommonInfo);
            mIndirectOpaqueDrawPass->initialize(drawPassInit);
        }
        // Skybox pass
        {
            SkyBoxPass::SkyBoxInitInfo drawPassInit;
            drawPassInit.colorTexDesc = colorTexDesc;
            drawPassInit.depthTexDesc = depthTexDesc;

            mSkyBoxPass = std::make_shared<SkyBoxPass>();
            mSkyBoxPass->setCommonInfo(renderPassCommonInfo);
            mSkyBoxPass->initialize(drawPassInit);
        }
        // Transparent drawing pass
        {
            IndirectDrawTransparentPass::DrawPassInitInfo drawPassInit;
            drawPassInit.colorTexDesc = colorTexDesc;
            drawPassInit.depthTexDesc = depthTexDesc;

            mIndirectTransparentDrawPass = std::make_shared<IndirectDrawTransparentPass>();
            mIndirectTransparentDrawPass->setCommonInfo(renderPassCommonInfo);
            mIndirectTransparentDrawPass->initialize(drawPassInit);
        }
        // shadow pass
        {
            IndirectShadowPass::ShadowPassInitInfo shadowPassInit;

            mIndirectShadowPass = std::make_shared<IndirectShadowPass>();
            mIndirectShadowPass->setCommonInfo(renderPassCommonInfo);
            mIndirectShadowPass->initialize(shadowPassInit);
        }
        // terrain shadow pass
        {
            IndirectTerrainShadowPass::ShadowPassInitInfo terrainShadowPassInit;
            terrainShadowPassInit.m_ShaderCompiler = pCompiler;
            terrainShadowPassInit.m_ShaderRootPath = g_runtime_global_context.m_config_manager->getShaderFolder();

            mIndirectTerrainShadowPass = std::make_shared<IndirectTerrainShadowPass>();
            mIndirectTerrainShadowPass->setCommonInfo(renderPassCommonInfo);
            mIndirectTerrainShadowPass->initialize(terrainShadowPassInit); 
        }
        // ao pass
        {
            AOPass::AOInitInfo aoPassInit;
            aoPassInit.colorTexDesc       = colorTexDesc;
            aoPassInit.m_ShaderCompiler   = pCompiler;
            aoPassInit.m_ShaderRootPath   = g_runtime_global_context.m_config_manager->getShaderFolder();

            mAOPass = std::make_shared<AOPass>();
            mAOPass->setCommonInfo(renderPassCommonInfo);
            mAOPass->initialize(aoPassInit); 
        }
        // postprocess pass
        {
            RHI::RgTextureDesc resolveColorTexDesc = colorTexDesc;
            resolveColorTexDesc.SetAllowUnorderedAccess();
            resolveColorTexDesc.SetSampleCount(1);

            PostprocessPasses::PostprocessInitInfo postInitInfo;
            postInitInfo.colorTexDesc = resolveColorTexDesc;
            postInitInfo.m_ShaderCompiler = pCompiler;

            mPostprocessPasses = std::make_shared<PostprocessPasses>();
            mPostprocessPasses->setCommonInfo(renderPassCommonInfo);
            mPostprocessPasses->initialize(postInitInfo);
        }
        // UI drawing pass
        {
            mDisplayPass = std::make_shared<DisplayPass>();
        }
    }

    void DeferredRenderer::InitializeUIRenderBackend(WindowUI* window_ui)
    {
        RenderPassCommonInfo renderPassCommonInfo = {
            &renderGraphAllocator, &renderGraphRegistry, pDevice, pWindowSystem};

        mUIPass = std::make_shared<UIPass>();
        mUIPass->setCommonInfo(renderPassCommonInfo);
        UIPass::UIPassInitInfo uiPassInitInfo;
        uiPassInitInfo.window_ui = window_ui;
        mUIPass->initialize(uiPassInitInfo);

        window_ui->setGameView(p_RenderTargetTex->GetDefaultSRV()->GetGpuHandle(), backBufferWidth, backBufferHeight);
    }

    void DeferredRenderer::PreparePassData(std::shared_ptr<RenderResource> render_resource)
    {
        render_resource->updateFrameUniforms(RenderPass::m_render_scene, RenderPass::m_render_camera);

        mIndirectCullPass->prepareMeshData(render_resource);
        mIndirectShadowPass->prepareShadowmaps(render_resource);
        mIndirectTerrainShadowPass->prepareShadowmaps(render_resource);
        mSkyBoxPass->prepareMeshData(render_resource);
        mTerrainCullPass->prepareMeshData(render_resource);
        mIndirectTerrainGBufferPass->prepareMatBuffer(render_resource);
        mDepthPyramidPass->prepareMeshData(render_resource);

        mPostprocessPasses->PreparePassData(render_resource);

        mIndirectCullPass->inflatePerframeBuffer(render_resource);
    }

    DeferredRenderer::~DeferredRenderer()
    {
        mToolPass                    = nullptr;
        mUIPass                      = nullptr;
        mIndirectCullPass            = nullptr;
        mTerrainCullPass             = nullptr;
        mIndirectShadowPass          = nullptr;
        mIndirectTerrainShadowPass   = nullptr;
        mIndirectGBufferPass         = nullptr;
        mIndirectTerrainGBufferPass  = nullptr;
        mDepthPyramidPass            = nullptr;
        mIndirectLightLoopPass       = nullptr;
        mIndirectOpaqueDrawPass      = nullptr;
        mAOPass                      = nullptr;
        mSkyBoxPass                  = nullptr;
        mIndirectTransparentDrawPass = nullptr;
        mPostprocessPasses           = nullptr;
        mDisplayPass                 = nullptr;

        // Release global objects
        RootSignatures::Release();
        CommandSignatures::Release();
        PipelineStates::Release();
    }

    void DeferredRenderer::OnRender(RHI::D3D12CommandContext* context)
    {


        //IndirectCullPass::IndirectCullOutput indirectCullOutput;
        //mIndirectCullPass->cullMeshs(context, &renderGraphRegistry, indirectCullOutput);

        RHI::RenderGraph graph(renderGraphAllocator, renderGraphRegistry);
        // backbuffer output
        RHI::D3D12Texture* pBackBufferResource = pSwapChain->GetCurrentBackBufferResource();
        RHI::RgResourceHandle backBufColorHandle = graph.Import(pBackBufferResource);
        // game view output
        RHI::RgResourceHandle renderTargetColorHandle = graph.Import(p_RenderTargetTex.get());
        
        /**/
        //=================================================================================
        // 应该再给graph添加一个signal同步，目前先这样
        IndirectCullPass::IndirectCullOutput indirectCullOutput;
        mIndirectCullPass->update(graph, indirectCullOutput);
        //=================================================================================
        
        //=================================================================================
        // Terrain剪裁Pass
        // Terrain使用上一帧depth剪裁Pass
        RHI::RgResourceHandle lastFrameMinDepthPyramidHandle =
            graph.Import(mDepthPyramidPass->GetLastFrameMinDepthPyramid().get());

        IndirectTerrainCullPass::TerrainCullInput terrainCullInput;
        IndirectTerrainCullPass::TerrainCullOutput terrainCullOutput;
        terrainCullInput.perframeBufferHandle = indirectCullOutput.perframeBufferHandle;
        terrainCullInput.lastMinDepthPyramidHandle = lastFrameMinDepthPyramidHandle;
        
        mTerrainCullPass->update(graph, terrainCullInput, terrainCullOutput);
        //=================================================================================

        //=================================================================================
        // indirect draw shadow
        IndirectShadowPass::ShadowInputParameters  mShadowmapIntputParams;
        IndirectShadowPass::ShadowOutputParameters mShadowmapOutputParams;

        mShadowmapIntputParams.perframeBufferHandle = indirectCullOutput.perframeBufferHandle;
        mShadowmapIntputParams.meshBufferHandle     = indirectCullOutput.meshBufferHandle;
        mShadowmapIntputParams.materialBufferHandle = indirectCullOutput.materialBufferHandle;
        for (size_t i = 0; i < indirectCullOutput.directionShadowmapHandles.size(); i++)
        {
            mShadowmapIntputParams.dirIndirectSortBufferHandles.push_back(indirectCullOutput.directionShadowmapHandles[i].indirectSortBufferHandle);
        }
        for (size_t i = 0; i < indirectCullOutput.spotShadowmapHandles.size(); i++)
        {
            mShadowmapIntputParams.spotsIndirectSortBufferHandles.push_back(indirectCullOutput.spotShadowmapHandles[i].indirectSortBufferHandle);
        }
        mIndirectShadowPass->update(graph, mShadowmapIntputParams, mShadowmapOutputParams);
        //=================================================================================
        
        //=================================================================================
        // indirect terrain draw shadow
        IndirectTerrainShadowPass::ShadowInputParameters  mTerrainShadowmapIntputParams;
        IndirectTerrainShadowPass::ShadowOutputParameters mTerrainShadowmapOutputParams;

        mTerrainShadowmapIntputParams.perframeBufferHandle = indirectCullOutput.perframeBufferHandle;
        mTerrainShadowmapIntputParams.terrainPatchNodeHandle = terrainCullOutput.terrainPatchNodeBufferHandle;
        mTerrainShadowmapIntputParams.terrainHeightmapHandle = terrainCullOutput.terrainHeightmapHandle;
        for (int i = 0; i < terrainCullOutput.directionShadowmapHandles.size(); i++)
        {
            auto& _indexAndSigHandle = terrainCullOutput.directionShadowmapHandles[i];
            mTerrainShadowmapIntputParams.dirShadowIndexAndSigHandle.push_back(
                {_indexAndSigHandle.commandSigBufferHandle, _indexAndSigHandle.indirectIndexBufferHandle});
        }
        mTerrainShadowmapOutputParams.directionalShadowmapHandle = mShadowmapOutputParams.directionalShadowmapHandle;
        mIndirectTerrainShadowPass->update(graph, mTerrainShadowmapIntputParams, mTerrainShadowmapOutputParams);
        //=================================================================================

        //=================================================================================
        // shadowmap output
        RHI::RgResourceHandle directionalShadowmapHandle = mTerrainShadowmapOutputParams.directionalShadowmapHandle;
        std::vector<RHI::RgResourceHandle> spotShadowmapHandle = mShadowmapOutputParams.spotShadowmapHandle;
        //=================================================================================

        //=================================================================================
        // prepare gbuffer output
        GBufferOutput mGBufferOutput;
        mIndirectGBufferPass->initializeRenderTarget(graph, &mGBufferOutput);

        //=================================================================================

        //=================================================================================
        // indirect gbuffer
        IndirectGBufferPass::DrawInputParameters mGBufferIntput;
        
        mGBufferIntput.perframeBufferHandle = indirectCullOutput.perframeBufferHandle;
        mGBufferIntput.meshBufferHandle     = indirectCullOutput.meshBufferHandle;
        mGBufferIntput.materialBufferHandle = indirectCullOutput.materialBufferHandle;
        mGBufferIntput.opaqueDrawHandle     = indirectCullOutput.opaqueDrawHandle.indirectSortBufferHandle;
        mIndirectGBufferPass->update(graph, mGBufferIntput, mGBufferOutput);
        //=================================================================================
        
        //=================================================================================
        // indirect terrain gbuffer
        IndirectTerrainGBufferPass::DrawInputParameters mTerrainGBufferIntput;
        mTerrainGBufferIntput.perframeBufferHandle = indirectCullOutput.perframeBufferHandle;
        mTerrainGBufferIntput.terrainPatchNodeHandle = terrainCullOutput.terrainPatchNodeBufferHandle;
        mTerrainGBufferIntput.terrainHeightmapHandle = terrainCullOutput.terrainHeightmapHandle;
        mTerrainGBufferIntput.terrainNormalmapHandle = terrainCullOutput.terrainNormalmapHandle;

        mTerrainGBufferIntput.drawIndexAndSigHandle  = {terrainCullOutput.terrainDrawHandle.commandSigBufferHandle, terrainCullOutput.terrainDrawHandle.indirectIndexBufferHandle};
        for (int i = 0; i < terrainCullOutput.directionShadowmapHandles.size(); i++)
        {
            auto& _indexAndSigHandle = terrainCullOutput.directionShadowmapHandles[i];
            mTerrainGBufferIntput.dirShadowIndexAndSigHandle.push_back({_indexAndSigHandle.commandSigBufferHandle, _indexAndSigHandle.indirectIndexBufferHandle});
        }

        mIndirectTerrainGBufferPass->update(graph, mTerrainGBufferIntput, mGBufferOutput);
        //=================================================================================


        //=================================================================================
        // depth pyramid
        DepthPyramidPass::DrawInputParameters mDepthPyramidInput;
        DepthPyramidPass::DrawOutputParameters mDepthPyramidOutput;
        mDepthPyramidInput.depthHandle          = mGBufferOutput.depthHandle;
        mDepthPyramidInput.perframeBufferHandle = indirectCullOutput.perframeBufferHandle;

        mDepthPyramidPass->update(graph, mDepthPyramidInput, mDepthPyramidOutput);
        //=================================================================================

        //=================================================================================
        // ambient occlusion
        AOPass::DrawInputParameters mAOIntput;
        AOPass::DrawOutputParameters mAOOutput;

        mAOIntput.perframeBufferHandle = indirectCullOutput.perframeBufferHandle;
        mAOIntput.worldNormalHandle    = mGBufferOutput.worldNormalHandle;
        mAOIntput.depthHandle          = mGBufferOutput.depthHandle;
        mAOPass->update(graph, mAOIntput, mAOOutput);
        //=================================================================================

        //=================================================================================
        // light loop pass
        IndirectLightLoopPass::DrawInputParameters mLightLoopIntput;
        IndirectLightLoopPass::DrawOutputParameters mLightLoopOutput;

        mLightLoopIntput.perframeBufferHandle = indirectCullOutput.perframeBufferHandle;
        mLightLoopIntput.albedoHandle           = mGBufferOutput.albedoHandle;
        mLightLoopIntput.worldNormalHandle      = mGBufferOutput.worldNormalHandle;
        mLightLoopIntput.worldTangentHandle     = mGBufferOutput.worldTangentHandle;
        mLightLoopIntput.materialNormalHandle   = mGBufferOutput.matNormalHandle;
        mLightLoopIntput.ambientOcclusionHandle = mAOOutput.outputAOHandle;
        mLightLoopIntput.metallic_Roughness_Reflectance_AO_Handle =
            mGBufferOutput.metallic_Roughness_Reflectance_AO_Handle;
        mLightLoopIntput.clearCoat_ClearCoatRoughness_Anisotropy_Handle =
            mGBufferOutput.clearCoat_ClearCoatRoughness_Anisotropy_Handle;
        mLightLoopIntput.gbufferDepthHandle = mGBufferOutput.depthHandle;
        mIndirectLightLoopPass->update(graph, mLightLoopIntput, mLightLoopOutput);
        //=================================================================================
        /*
        //=================================================================================
        // indirect opaque draw
        IndirectDrawPass::DrawInputParameters  mDrawIntputParams;
        IndirectDrawPass::DrawOutputParameters mDrawOutputParams;

        mDrawIntputParams.perframeBufferHandle = indirectCullOutput.perframeBufferHandle;
        mDrawIntputParams.meshBufferHandle     = indirectCullOutput.meshBufferHandle;
        mDrawIntputParams.materialBufferHandle = indirectCullOutput.materialBufferHandle;
        mDrawIntputParams.opaqueDrawHandle     = indirectCullOutput.opaqueDrawHandle.indirectSortBufferHandle;
        mDrawIntputParams.directionalShadowmapTexHandle = directionalShadowmapHandle;
        for (size_t i = 0; i < spotShadowmapHandle.size(); i++)
        {
            mDrawIntputParams.spotShadowmapTexHandles.push_back(spotShadowmapHandle[i]);
        }
        mIndirectOpaqueDrawPass->update(graph, mDrawIntputParams, mDrawOutputParams);
        //=================================================================================
        */
        //=================================================================================
        // skybox draw
        SkyBoxPass::DrawInputParameters  mSkyboxIntputParams;
        SkyBoxPass::DrawOutputParameters mSkyboxOutputParams;

        mSkyboxIntputParams.perframeBufferHandle    = indirectCullOutput.perframeBufferHandle;
        mSkyboxOutputParams.renderTargetColorHandle = mLightLoopOutput.colorHandle;
        mSkyboxOutputParams.renderTargetDepthHandle = mGBufferOutput.depthHandle;
        //mSkyboxOutputParams.renderTargetColorHandle = mDrawOutputParams.renderTargetColorHandle;
        //mSkyboxOutputParams.renderTargetDepthHandle = mDrawOutputParams.renderTargetDepthHandle;
        mSkyBoxPass->update(graph, mSkyboxIntputParams, mSkyboxOutputParams);
        //=================================================================================


        //=================================================================================
        // indirect transparent draw
        IndirectDrawTransparentPass::DrawInputParameters  mDrawTransIntputParams;
        IndirectDrawTransparentPass::DrawOutputParameters mDrawTransOutputParams;

        mDrawTransIntputParams.perframeBufferHandle = indirectCullOutput.perframeBufferHandle;
        mDrawTransIntputParams.meshBufferHandle     = indirectCullOutput.meshBufferHandle;
        mDrawTransIntputParams.materialBufferHandle = indirectCullOutput.materialBufferHandle;
        mDrawTransIntputParams.transparentDrawHandle = indirectCullOutput.transparentDrawHandle.indirectSortBufferHandle;
        mDrawTransIntputParams.directionalShadowmapTexHandle = directionalShadowmapHandle;
        for (size_t i = 0; i < spotShadowmapHandle.size(); i++)
        {
            mDrawTransIntputParams.spotShadowmapTexHandles.push_back(spotShadowmapHandle[i]);
        }
        mDrawTransOutputParams.renderTargetColorHandle = mSkyboxOutputParams.renderTargetColorHandle;
        mDrawTransOutputParams.renderTargetDepthHandle = mSkyboxOutputParams.renderTargetDepthHandle;
        mIndirectTransparentDrawPass->update(graph, mDrawTransIntputParams, mDrawTransOutputParams);
        //=================================================================================


        //=================================================================================
        //RHI::RgResourceHandle outputRTColorHandle = mDrawOutputParams.renderTargetColorHandle;

        // postprocess rendertarget
        PostprocessPasses::PostprocessInputParameters  mPostprocessIntputParams;
        PostprocessPasses::PostprocessOutputParameters mPostprocessOutputParams;

        mPostprocessIntputParams.perframeBufferHandle  = indirectCullOutput.perframeBufferHandle;
        mPostprocessIntputParams.motionVectorHandle    = mGBufferOutput.motionVectorHandle;
        mPostprocessIntputParams.inputSceneColorHandle = mDrawTransOutputParams.renderTargetColorHandle;
        mPostprocessIntputParams.inputSceneDepthHandle = mDrawTransOutputParams.renderTargetDepthHandle;
        mPostprocessPasses->update(graph, mPostprocessIntputParams, mPostprocessOutputParams);

        //outputRTColorHandle = mPostprocessOutputParams.outputColorHandle;
        //=================================================================================

        //=================================================================================
        // display
        DisplayPass::DisplayInputParameters  mDisplayIntputParams;
        DisplayPass::DisplayOutputParameters mDisplayOutputParams;

        mDisplayIntputParams.inputRTColorHandle   = mPostprocessOutputParams.outputColorHandle;
        //mDisplayIntputParams.inputRTColorHandle   = mTerrainGBufferOutput.albedoHandle;
        //mDisplayIntputParams.inputRTColorHandle      = mAOOutput.outputAOHandle;
        //mDisplayOutputParams.renderTargetColorHandle = renderTargetColorHandle;
        mDisplayOutputParams.renderTargetColorHandle = backBufColorHandle;
        mDisplayPass->update(graph, mDisplayIntputParams, mDisplayOutputParams);
        //=================================================================================
        
        ////=================================================================================
        //if (mUIPass != nullptr)
        //{
        //    UIPass::UIInputParameters mUIIntputParams;
        //    UIPass::UIOutputParameters mUIOutputParams;

        //    //mUIIntputParams.renderTargetColorHandle = renderTargetColorHandle;
        //    mUIIntputParams.renderTargetColorHandle = mDisplayOutputParams.renderTargetColorHandle;
        //    mUIOutputParams.backBufColorHandle = backBufColorHandle;
        //    
        //    mUIPass->update(graph, mUIIntputParams, mUIOutputParams);
        //}
        ////=================================================================================

        graph.Execute(context);

        {
            // Transfer the state of the backbuffer to Present
            context->TransitionBarrier(pBackBufferResource,
                                      D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PRESENT,
                                      D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
                                      true);
        }

        //DgmlBuilder Builder("Render Graph");
        //graph.ExportDgml(Builder);
        //Builder.SaveAs(std::filesystem::current_path() / "RenderGraph.dgml");
    }

    void DeferredRenderer::PreRender(double deltaTime)
    {
        {
            // 生成specular LD和DFG，生成diffuse radiance
            ToolPass::ToolInputParameters  _toolPassInput;
            ToolPass::ToolOutputParameters _toolPassOutput;
            mToolPass->preUpdate(_toolPassInput, _toolPassOutput);
        }
    }

    void DeferredRenderer::PostRender(double deltaTime)
    {

    }

}

