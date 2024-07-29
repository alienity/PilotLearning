#include "renderer_deferred.h"

#include "runtime/function/render/renderer/renderer_registry.h"
#include "runtime/function/global/global_context.h"
#include "runtime/resource/config_manager/config_manager.h"

#include "runtime/resource/basic_geometry/mesh_tools.h"
#include "runtime/resource/basic_geometry/icosphere_mesh.h"
#include "runtime/resource/basic_geometry/cube_mesh.h"

#include "runtime/function/render/render_helper.h"
#include "runtime/function/render/terrain_render_helper.h"

namespace MoYu
{
    DeferredRenderer::DeferredRenderer(RendererInitParams& renderInitParams) :
        Renderer(renderInitParams)
    {
    }

    void DeferredRenderer::Initialize()
    {
        RHI::D3D12Texture* pBackBufferResource = pSwapChain->GetCurrentBackBufferResource();

        CD3DX12_RESOURCE_DESC backDesc = pBackBufferResource->GetDesc();

        backBufferWidth = backDesc.Width;
        backBufferHeight = backDesc.Height;
        backBufferFormat = backDesc.Format;
        depthBufferFormat = DXGI_FORMAT_D32_FLOAT;

        pipleineColorFormat = DXGI_FORMAT_R32G32B32A32_FLOAT;
        pipleineDepthFormat = DXGI_FORMAT_D32_FLOAT;

        SetViewPort(0, 0, backBufferWidth, backBufferHeight);

        std::shared_ptr<ConfigManager> config_manager = g_runtime_global_context.m_config_manager;
        const std::filesystem::path& shaderRootPath = config_manager->getShaderFolder();

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
        constexpr FLOAT renderTargetClearColor[4] = {0, 0, 0, 0};
        auto renderTargetClearValue = CD3DX12_CLEAR_VALUE(backBufferFormat, renderTargetClearColor);

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

        for (int i = 0; i < 2; i++)
        {
            std::wstring _name = fmt::format(L"ColorPyramid_{}", i);
            p_ColorPyramidRTs[i] =
                RHI::D3D12Texture::Create2D(pDevice->GetLinkedDevice(),
                                            viewport.width,
                                            viewport.height,
                                            8,
                                            DXGI_FORMAT_R32G32B32A32_FLOAT,
                                            RHI::RHISurfaceCreateRandomWrite | RHI::RHISurfaceCreateMipmap,
                                            1,
                                            _name,
                                            D3D12_RESOURCE_STATE_COMMON,
                                            std::nullopt);
        }
    }

    void DeferredRenderer::InitPass()
    {
        RenderPassCommonInfo renderPassCommonInfo = {
            &renderGraphAllocator, &renderGraphRegistry, pDevice, pWindowSystem
        };

        int sampleCount = EngineConfig::g_AntialiasingMode == EngineConfig::MSAA
                              ? EngineConfig::g_MASSConfig.m_MSAASampleCount
                              : 1;

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
            IndirectCullPass::IndirectCullInitInfo indirectCullInit;
            indirectCullInit.albedoTexDesc = colorTexDesc;
            indirectCullInit.depthTexDesc = depthTexDesc;
            indirectCullInit.m_ShaderCompiler = pCompiler;
            indirectCullInit.m_ShaderRootPath = g_runtime_global_context.m_config_manager->getShaderFolder();

            mIndirectCullPass = std::make_shared<IndirectCullPass>();
            mIndirectCullPass->setCommonInfo(renderPassCommonInfo);
            mIndirectCullPass->initialize(indirectCullInit);
        }
        // Terrain Cull pass
        {
            TerrainCullInitInfo drawPassInit;
            drawPassInit.colorTexDesc = colorTexDesc;
            drawPassInit.m_ShaderCompiler = pCompiler;
            drawPassInit.m_ShaderRootPath = g_runtime_global_context.m_config_manager->getShaderFolder();

            mTerrainCullPass = std::make_shared<IndirectTerrainCullPass>();
            mTerrainCullPass->setCommonInfo(renderPassCommonInfo);
            mTerrainCullPass->initialize(drawPassInit);
        }
        // DepthPrePass
        {
            DepthPrePass::DrawPassInitInfo drawPassInit;
            drawPassInit.depthTexDesc = depthTexDesc;
            drawPassInit.m_ShaderCompiler = pCompiler;
            drawPassInit.m_ShaderRootPath = g_runtime_global_context.m_config_manager->getShaderFolder();

            mDepthPrePass = std::make_shared<DepthPrePass>();
            mDepthPrePass->setCommonInfo(renderPassCommonInfo);
            mDepthPrePass->initialize(drawPassInit);
        }
        // GBuffer pass
        {
            IndirectGBufferPass::DrawPassInitInfo drawPassInit;
            drawPassInit.colorTexDesc = colorTexDesc;
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
            drawPassInit.albedoDesc = mIndirectGBufferPass->gbuffer0Desc;
            drawPassInit.worldNormalDesc = mIndirectGBufferPass->gbuffer0Desc;
            drawPassInit.motionVectorDesc = mIndirectGBufferPass->gbuffer0Desc;
            drawPassInit.metallic_Roughness_Reflectance_AO_Desc = mIndirectGBufferPass->gbuffer0Desc;
            drawPassInit.clearCoat_ClearCoatRoughness_AnisotropyDesc = mIndirectGBufferPass->gbuffer0Desc;
            drawPassInit.depthDesc = mIndirectGBufferPass->depthDesc;

            drawPassInit.m_ShaderCompiler = pCompiler;
            drawPassInit.m_ShaderRootPath = g_runtime_global_context.m_config_manager->getShaderFolder();

            mIndirectTerrainGBufferPass = std::make_shared<IndirectTerrainGBufferPass>();
            mIndirectTerrainGBufferPass->setCommonInfo(renderPassCommonInfo);
            mIndirectTerrainGBufferPass->initialize(drawPassInit);
        }
        // Depth Pyramid
        {
            DepthPyramidPass::DrawPassInitInfo depthDrawPassInit;
            depthDrawPassInit.albedoTexDesc = mIndirectGBufferPass->gbuffer0Desc;
            depthDrawPassInit.depthTexDesc = mIndirectGBufferPass->depthDesc;

            depthDrawPassInit.m_ShaderCompiler = pCompiler;
            depthDrawPassInit.m_ShaderRootPath = g_runtime_global_context.m_config_manager->getShaderFolder();

            mDepthPyramidPass = std::make_shared<DepthPyramidPass>();
            mDepthPyramidPass->setCommonInfo(renderPassCommonInfo);
            mDepthPyramidPass->initialize(depthDrawPassInit);
        }
        // Color Pyramid
        {
            ColorPyramidPass::DrawPassInitInfo colorPyramidPassInit;
            colorPyramidPassInit.albedoTexDesc = mIndirectGBufferPass->gbuffer0Desc;

            colorPyramidPassInit.m_ShaderCompiler = pCompiler;
            colorPyramidPassInit.m_ShaderRootPath = g_runtime_global_context.m_config_manager->getShaderFolder();

            mColorPyramidPass = std::make_shared<ColorPyramidPass>();
            mColorPyramidPass->setCommonInfo(renderPassCommonInfo);
            mColorPyramidPass->initialize(colorPyramidPassInit);
        }
        // VolumeLight Pass
        {
            VolumeLightPass::PassInitInfo passInitInfo;
            passInitInfo.colorTexDesc = colorTexDesc;
            passInitInfo.depthTexDesc = depthTexDesc;
            passInitInfo.m_ShaderCompiler = pCompiler;
            passInitInfo.m_ShaderRootPath = g_runtime_global_context.m_config_manager->getShaderFolder();

            mVolumeLightPass = std::make_shared<VolumeLightPass>();
            mVolumeLightPass->setCommonInfo(renderPassCommonInfo);
            mVolumeLightPass->initialize(passInitInfo);
        }
        // LightLoop pass
        {
            IndirectLightLoopPass::DrawPassInitInfo drawPassInit;
            drawPassInit.colorTexDesc = colorTexDesc;
            drawPassInit.m_ShaderCompiler = pCompiler;
            drawPassInit.m_ShaderRootPath = g_runtime_global_context.m_config_manager->getShaderFolder();

            mIndirectLightLoopPass = std::make_shared<IndirectLightLoopPass>();
            mIndirectLightLoopPass->setCommonInfo(renderPassCommonInfo);
            mIndirectLightLoopPass->initialize(drawPassInit);
        }
        // SubsurfaceScattering pass
        {
            SubsurfaceScatteringPass::PassInitInfo drawPassInit;
            drawPassInit.colorTexDesc = colorTexDesc;
            drawPassInit.m_ShaderCompiler = pCompiler;
            drawPassInit.m_ShaderRootPath = g_runtime_global_context.m_config_manager->getShaderFolder();

            mSubsurfaceScatteringPass = std::make_shared<SubsurfaceScatteringPass>();
            mSubsurfaceScatteringPass->setCommonInfo(renderPassCommonInfo);
            mSubsurfaceScatteringPass->initialize(drawPassInit);
        }
        // Opaque drawing pass
        {
            IndirectDrawPass::DrawPassInitInfo drawPassInit;
            drawPassInit.colorTexDesc = colorTexDesc;
            drawPassInit.depthTexDesc = depthTexDesc;
            drawPassInit.m_ShaderCompiler = pCompiler;
            drawPassInit.m_ShaderRootPath = g_runtime_global_context.m_config_manager->getShaderFolder();

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
        // AtmosphericScattering pass
        {
            AtmosphericScatteringPass::PassInitInfo drawPassInit;
            drawPassInit.colorTexDesc = colorTexDesc;
            drawPassInit.m_ShaderCompiler = pCompiler;
            drawPassInit.m_ShaderRootPath = g_runtime_global_context.m_config_manager->getShaderFolder();

            mAtmosphericScatteringPass = std::make_shared<AtmosphericScatteringPass>();
            mAtmosphericScatteringPass->setCommonInfo(renderPassCommonInfo);
            mAtmosphericScatteringPass->initialize(drawPassInit);
        }
        // VolumeCloud pass
        {
            VolumeCloudPass::PassInitInfo drawPassInit;
            drawPassInit.colorTexDesc = colorTexDesc;
            drawPassInit.m_ShaderCompiler = pCompiler;
            drawPassInit.m_ShaderRootPath = g_runtime_global_context.m_config_manager->getShaderFolder();

            mVolumeCloudPass = std::make_shared<VolumeCloudPass>();
            mVolumeCloudPass->setCommonInfo(renderPassCommonInfo);
            mVolumeCloudPass->initialize(drawPassInit);
        }
        // Transparent drawing pass
        {
            IndirectDrawTransparentPass::DrawPassInitInfo drawPassInit;
            drawPassInit.colorTexDesc = colorTexDesc;
            drawPassInit.depthTexDesc = depthTexDesc;
            drawPassInit.m_ShaderCompiler = pCompiler;
            drawPassInit.m_ShaderRootPath = g_runtime_global_context.m_config_manager->getShaderFolder();

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
            aoPassInit.colorTexDesc = colorTexDesc;
            aoPassInit.m_ShaderCompiler = pCompiler;
            aoPassInit.m_ShaderRootPath = g_runtime_global_context.m_config_manager->getShaderFolder();

            mAOPass = std::make_shared<AOPass>();
            mAOPass->setCommonInfo(renderPassCommonInfo);
            mAOPass->initialize(aoPassInit);
        }
        // gtao pass
        {
            GTAOPass::AOInitInfo aoPassInit;
            aoPassInit.colorTexDesc = colorTexDesc;
            aoPassInit.depthTexDesc = depthTexDesc;
            aoPassInit.m_ShaderCompiler = pCompiler;
            aoPassInit.m_ShaderRootPath = g_runtime_global_context.m_config_manager->getShaderFolder();

            mGTAOPass = std::make_shared<GTAOPass>();
            mGTAOPass->setCommonInfo(renderPassCommonInfo);
            mGTAOPass->initialize(aoPassInit);
        }
        // ssr pass
        {
            SSRPass::SSRInitInfo ssrPassInit;
            ssrPassInit.m_ColorTexDesc = colorTexDesc;
            ssrPassInit.m_ShaderCompiler = pCompiler;
            ssrPassInit.m_ShaderRootPath = g_runtime_global_context.m_config_manager->getShaderFolder();

            mSSRPass = std::make_shared<SSRPass>();
            mSSRPass->setCommonInfo(renderPassCommonInfo);
            mSSRPass->initialize(ssrPassInit);
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
            &renderGraphAllocator, &renderGraphRegistry, pDevice, pWindowSystem
        };

        mUIPass = std::make_shared<UIPass>();
        mUIPass->setCommonInfo(renderPassCommonInfo);
        UIPass::UIPassInitInfo uiPassInitInfo;
        uiPassInitInfo.window_ui = window_ui;
        mUIPass->initialize(uiPassInitInfo);

        window_ui->setGameView(p_RenderTargetTex->GetDefaultSRV()->GetGpuHandle(), backBufferWidth, backBufferHeight);
    }

    void DeferredRenderer::PreparePassData(std::shared_ptr<RenderResource> render_resource)
    {
        render_resource->ReleaseTransientResources();

        render_resource->updateFrameUniforms(RenderPass::m_render_scene, RenderPass::m_render_camera);

        mIndirectCullPass->prepareMeshData(render_resource);
        mIndirectShadowPass->prepareShadowmaps(render_resource);
        mIndirectTerrainShadowPass->prepareShadowmaps(render_resource);
        mSkyBoxPass->prepareMeshData(render_resource);
        mTerrainCullPass->prepareMeshData(render_resource);
        mIndirectTerrainGBufferPass->prepareMatBuffer(render_resource);
        mDepthPrePass->prepareMatBuffer(render_resource);
        mIndirectGBufferPass->prepareMatBuffer(render_resource);
        mDepthPyramidPass->prepareMeshData(render_resource);
        mColorPyramidPass->prepareMeshData(render_resource);
        mSubsurfaceScatteringPass->prepareMetaData(render_resource);
        mSSRPass->prepareMetaData(render_resource);
        mVolumeLightPass->prepareMeshData(render_resource);
        mPostprocessPasses->PreparePassData(render_resource);

        mAtmosphericScatteringPass->prepareMetaData(render_resource);
        mVolumeCloudPass->prepareMetaData(render_resource);

        mGTAOPass->prepareMatBuffer(render_resource);

        mIndirectCullPass->inflatePerframeBuffer(render_resource);
    }

    DeferredRenderer::~DeferredRenderer()
    {
        mToolPass = nullptr;
        mUIPass = nullptr;
        mIndirectCullPass = nullptr;
        mTerrainCullPass = nullptr;
        mIndirectShadowPass = nullptr;
        mIndirectTerrainShadowPass = nullptr;
        mDepthPrePass = nullptr;
        mIndirectGBufferPass = nullptr;
        mIndirectTerrainGBufferPass = nullptr;
        mDepthPyramidPass = nullptr;
        mColorPyramidPass = nullptr;
        mIndirectLightLoopPass = nullptr;
        mSubsurfaceScatteringPass = nullptr;
        mIndirectOpaqueDrawPass = nullptr;
        mAOPass = nullptr;
        mGTAOPass = nullptr;
        mSSRPass = nullptr;
        mVolumeLightPass = nullptr;
        mSkyBoxPass = nullptr;
        mAtmosphericScatteringPass = nullptr;
        mVolumeCloudPass = nullptr;
        mIndirectTransparentDrawPass = nullptr;
        mPostprocessPasses = nullptr;
        mDisplayPass = nullptr;

        // Release global objects
        RootSignatures::Release();
        CommandSignatures::Release();
        PipelineStates::Release();
    }

    void DeferredRenderer::OnRender(RHI::D3D12CommandContext* context)
    {
        ////IndirectCullPass::IndirectCullOutput indirectCullOutput;
        ////mIndirectCullPass->cullMeshs(context, &renderGraphRegistry, indirectCullOutput);

        RHI::RenderGraph graph(renderGraphAllocator, renderGraphRegistry);
        // backbuffer output
        RHI::D3D12Texture* pBackBufferResource = pSwapChain->GetCurrentBackBufferResource();
        RHI::RgResourceHandle backBufColorHandle = graph.Import(pBackBufferResource);
        // game view output
        RHI::RgResourceHandle renderTargetColorHandle = graph.Import(p_RenderTargetTex.get());

        // last frame color buffer
        RHI::RgResourceHandle curFrameColorRTHandle = graph.Import(GetCurrentFrameColorPyramid().get());
        // current frame color buffer
        RHI::RgResourceHandle lastFrameColorRTHandle = graph.Import(GetLastFrameColorPyramid().get());

        /**/
        //=================================================================================
        // 应该再给graph添加一个signal同步，目前先这样
        IndirectCullPass::IndirectCullOutput indirectCullOutput;
        mIndirectCullPass->update(graph, indirectCullOutput);
        //=================================================================================

        //=================================================================================
        // Terrain剪裁Pass
        RHI::RgResourceHandle lastFrameMinDepthPyramidHandle =
            graph.Import(mDepthPyramidPass->GetLastFrameMinDepthPyramid().get());

        IndirectTerrainCullPass::TerrainCullInput terrainCullInput;
        IndirectTerrainCullPass::TerrainCullOutput terrainCullOutput;
        terrainCullInput.perframeBufferHandle = indirectCullOutput.perframeBufferHandle;

        mTerrainCullPass->update(graph, terrainCullInput, terrainCullOutput);
        //=================================================================================
        /*
        //=================================================================================
        // Terrain使用上一帧depth剪裁Pass
        IndirectTerrainCullPass::DepthCullIndexInput _input = {};
        _input.minDepthPyramidHandle  = lastFrameMinDepthPyramidHandle;
        _input.perframeBufferHandle   = indirectCullOutput.perframeBufferHandle;
        IndirectTerrainCullPass::DrawCallCommandBufferHandle _output = {};

        mTerrainCullPass->cullByLastFrameDepth(graph, _input, _output);
        //=================================================================================
        */
        //=================================================================================
        // indirect draw shadow
        IndirectShadowPass::ShadowInputParameters mShadowmapIntputParams;
        IndirectShadowPass::ShadowOutputParameters mShadowmapOutputParams;

        mShadowmapIntputParams.perframeBufferHandle = indirectCullOutput.perframeBufferHandle;
        mShadowmapIntputParams.renderDataPerDrawHandle = indirectCullOutput.renderDataPerDrawHandle;
        mShadowmapIntputParams.propertiesPerMaterialHandle = indirectCullOutput.propertiesPerMaterialHandle;
        for (size_t i = 0; i < indirectCullOutput.directionShadowmapHandles.size(); i++)
        {
            mShadowmapIntputParams.dirIndirectSortBufferHandles.push_back(
                indirectCullOutput.directionShadowmapHandles[i].indirectSortBufferHandle);
        }
        for (size_t i = 0; i < indirectCullOutput.spotShadowmapHandles.size(); i++)
        {
            mShadowmapIntputParams.spotsIndirectSortBufferHandles.push_back(
                indirectCullOutput.spotShadowmapHandles[i].indirectSortBufferHandle);
        }
        mIndirectShadowPass->update(graph, mShadowmapIntputParams, mShadowmapOutputParams);
        //=================================================================================

        //=================================================================================
        // indirect terrain draw shadow
        IndirectTerrainShadowPass::ShadowInputParameters mTerrainShadowmapIntputParams;
        IndirectTerrainShadowPass::ShadowOutputParameters mTerrainShadowmapOutputParams;

        mTerrainShadowmapIntputParams.perframeBufferHandle = indirectCullOutput.perframeBufferHandle;
        mTerrainShadowmapIntputParams.terrainHeightmapHandle = terrainCullOutput.terrainHeightmapHandle;
        mTerrainShadowmapIntputParams.transformBufferHandle = terrainCullOutput.transformBufferHandle;
        mTerrainShadowmapIntputParams.dirCommandSigHandle.assign(terrainCullOutput.dirVisCommandSigHandles.begin(),
                                                                 terrainCullOutput.dirVisCommandSigHandles.end());

        mTerrainShadowmapOutputParams.directionalShadowmapHandles = mShadowmapOutputParams.directionalShadowmapHandles;

        mIndirectTerrainShadowPass->update(graph, mTerrainShadowmapIntputParams, mTerrainShadowmapOutputParams);
        //=================================================================================

        //=================================================================================
        // shadowmap output
        std::vector<RHI::RgResourceHandle> directionalShadowmapHandles = mShadowmapOutputParams.
            directionalShadowmapHandles;
        std::vector<RHI::RgResourceHandle> spotShadowmapHandle = mShadowmapOutputParams.spotShadowmapHandle;
        //=================================================================================

        //=================================================================================
        // depth prepass
        DepthPrePass::DrawInputParameters mDepthPrePassInput;
        DepthPrePass::DrawOutput mDepthPrepassOutput;

        mDepthPrePassInput.perframeBufferHandle = indirectCullOutput.perframeBufferHandle;
        mDepthPrePassInput.renderDataPerDrawHandle = indirectCullOutput.renderDataPerDrawHandle;
        mDepthPrePassInput.propertiesPerMaterialHandle = indirectCullOutput.propertiesPerMaterialHandle;
        mDepthPrePassInput.opaqueDrawHandle = indirectCullOutput.opaqueDrawHandle.indirectSortBufferHandle;

        mDepthPrePass->update(graph, mDepthPrePassInput, mDepthPrepassOutput);
        //=================================================================================

        //=================================================================================
        // indirect gbuffer
        IndirectGBufferPass::DrawInputParameters mGBufferIntput;
        GBufferOutput mGBufferOutput;

        mGBufferIntput.perframeBufferHandle = indirectCullOutput.perframeBufferHandle;
        mGBufferIntput.renderDataPerDrawHandle = indirectCullOutput.renderDataPerDrawHandle;
        mGBufferIntput.propertiesPerMaterialHandle = indirectCullOutput.propertiesPerMaterialHandle;
        mGBufferIntput.opaqueDrawHandle = indirectCullOutput.opaqueDrawHandle.indirectSortBufferHandle;
        mGBufferOutput.depthHandle = mDepthPrepassOutput.depthBufferHandle;

        mIndirectGBufferPass->update(graph, mGBufferIntput, mGBufferOutput);
        //=================================================================================

        //=================================================================================
        // indirect terrain gbuffer
        IndirectTerrainGBufferPass::DrawInputParameters mTerrainGBufferIntput;
        mTerrainGBufferIntput.perframeBufferHandle = indirectCullOutput.perframeBufferHandle;
        mTerrainGBufferIntput.terrainHeightmapHandle = terrainCullOutput.terrainHeightmapHandle;
        mTerrainGBufferIntput.terrainNormalmapHandle = terrainCullOutput.terrainNormalmapHandle;
        mTerrainGBufferIntput.transformBufferHandle = terrainCullOutput.transformBufferHandle;
        mTerrainGBufferIntput.terrainCommandSigHandle = terrainCullOutput.mainCamVisCommandSigHandle;

        mIndirectTerrainGBufferPass->update(graph, mTerrainGBufferIntput, mGBufferOutput);
        //=================================================================================

        //=================================================================================
        // depth pyramid
        DepthPyramidPass::DrawInputParameters mDepthPyramidInput;
        DepthPyramidPass::DrawOutputParameters mDepthPyramidOutput;
        mDepthPyramidInput.depthHandle = mGBufferOutput.depthHandle;
        mDepthPyramidInput.perframeBufferHandle = indirectCullOutput.perframeBufferHandle;

        mDepthPyramidPass->update(graph, mDepthPyramidInput, mDepthPyramidOutput);
        //=================================================================================
        /*
        //=================================================================================
        // Terrain使用当前帧depth剪裁Pass
        IndirectTerrainCullPass::DepthCullIndexInput _input2 = {};
        _input2.minDepthPyramidHandle = mDepthPyramidOutput.minDepthPtyramidHandle;
        _input2.perframeBufferHandle = indirectCullOutput.perframeBufferHandle;
        IndirectTerrainCullPass::DrawCallCommandBufferHandle _output2 = {};

        mTerrainCullPass->cullByCurrentFrameDepth(graph, _input2, _output2);
        //=================================================================================
        
        //=================================================================================
        // indirect terrain gbuffer for pass 2
        IndirectTerrainGBufferPass::DrawInputParameters mTerrainGBufferIntput2;
        mTerrainGBufferIntput2.perframeBufferHandle  = indirectCullOutput.perframeBufferHandle;
        mTerrainGBufferIntput2.terrainPatchNodeHandle = terrainCullOutput.terrainPatchNodeBufferHandle;
        mTerrainGBufferIntput2.terrainHeightmapHandle = terrainCullOutput.terrainHeightmapHandle;
        mTerrainGBufferIntput2.terrainNormalmapHandle = terrainCullOutput.terrainNormalmapHandle;

        mTerrainGBufferIntput2.drawIndexAndSigHandle  = {_output2.commandSigBufferHandle, _output2.indirectIndexBufferHandle};
        for (int i = 0; i < terrainCullOutput.directionShadowmapHandles.size(); i++)
        {
            auto& _indexAndSigHandle = terrainCullOutput.directionShadowmapHandles[i];
            mTerrainGBufferIntput2.dirShadowIndexAndSigHandle.push_back({_indexAndSigHandle.commandSigBufferHandle, _indexAndSigHandle.indirectIndexBufferHandle});
        }

        mIndirectTerrainGBufferPass->update(graph, mTerrainGBufferIntput2, mGBufferOutput);
        //=================================================================================
        */
        /*
        //=================================================================================
        // depth pyramid2
        DepthPyramidPass::DrawInputParameters mDepthPyramidInput2;
        DepthPyramidPass::DrawOutputParameters mDepthPyramidOutput2;
        mDepthPyramidInput2.depthHandle          = mGBufferOutput.depthHandle;
        mDepthPyramidInput2.perframeBufferHandle = indirectCullOutput.perframeBufferHandle;

        mDepthPyramidPass->update(graph, mDepthPyramidInput2, mDepthPyramidOutput2);
        //=================================================================================
        */


        ////=================================================================================
        //// VolumeCloud Shadow draw
        //VolumeCloudPass::ShadowInputParameters  mVCSIntputParams;
        //VolumeCloudPass::ShadowOutputParameters mVCSOutputParams;

        //mVCSIntputParams.perframeBufferHandle = indirectCullOutput.perframeBufferHandle;
        //mVolumeCloudPass->updateShadow(graph, mVCSIntputParams, mVCSOutputParams);
        ////=================================================================================

        ////=================================================================================
        //// volume light
        //VolumeLightPass::DrawInputParameters mVolumeLightInput;
        //VolumeLightPass::DrawOutputParameters mVolumeLightOutput;
        //mVolumeLightInput.perframeBufferHandle = indirectCullOutput.perframeBufferHandle;
        //mVolumeLightInput.maxDepthPtyramidHandle = mDepthPyramidOutput.maxDepthPtyramidHandle;
        //mVolumeLightInput.volumeCloudShadowHandle = mVCSOutputParams.outCloudShadowHandle;

        //mVolumeLightPass->update(graph, mVolumeLightInput, mVolumeLightOutput);
        ////=================================================================================

        ///*
        ////=================================================================================
        //// ambient occlusion
        //AOPass::DrawInputParameters mAOIntput;
        //AOPass::DrawOutputParameters mAOOutput;

        //mAOIntput.perframeBufferHandle = indirectCullOutput.perframeBufferHandle;
        //mAOIntput.worldNormalHandle    = mGBufferOutput.worldNormalHandle;
        //mAOIntput.depthHandle          = mGBufferOutput.depthHandle;
        //mAOPass->update(graph, mAOIntput, mAOOutput);
        ////=================================================================================
        //*/
        //=================================================================================
        // ambient occlusion
        GTAOPass::DrawInputParameters mGTAOIntput;
        GTAOPass::DrawOutputParameters mGTAOOutput;

        mGTAOIntput.perframeBufferHandle = indirectCullOutput.perframeBufferHandle;
        mGTAOIntput.packedNormalHandle = mGBufferOutput.gbuffer1Handle;
        mGTAOIntput.depthHandle = mGBufferOutput.depthHandle;
        mGTAOPass->update(graph, mGTAOIntput, mGTAOOutput);
        //=================================================================================

        ////=================================================================================
        //// ssr
        //SSRPass::DrawInputParameters mSSRInput;
        //SSRPass::DrawOutputParameters mSSROutput;
        //mSSRInput.perframeBufferHandle   = indirectCullOutput.perframeBufferHandle;
        //mSSRInput.worldNormalHandle      = mGBufferOutput.worldNormalHandle;
        //mSSRInput.mrraMapHandle          = mGBufferOutput.metallic_Roughness_Reflectance_AO_Handle;
        //mSSRInput.maxDepthPtyramidHandle = mDepthPyramidOutput.maxDepthPtyramidHandle;
        //mSSRInput.lastFrameColorHandle   = lastFrameColorRTHandle;
        //mSSRPass->update(graph, mSSRInput, mSSROutput);
        ////=================================================================================

        //=================================================================================
        // light loop pass
        IndirectLightLoopPass::DrawInputParameters mLightLoopIntput;
        IndirectLightLoopPass::DrawOutputParameters mLightLoopOutput;

        mLightLoopIntput.perframeBufferHandle = indirectCullOutput.perframeBufferHandle;
        mLightLoopIntput.gbuffer0Handle = mGBufferOutput.gbuffer0Handle;
        mLightLoopIntput.gbuffer1Handle = mGBufferOutput.gbuffer1Handle;
        mLightLoopIntput.gbuffer2Handle = mGBufferOutput.gbuffer2Handle;
        mLightLoopIntput.gbuffer3Handle = mGBufferOutput.gbuffer3Handle;
        mLightLoopIntput.gbufferDepthHandle = mGBufferOutput.depthHandle;
        mLightLoopIntput.mAOHandle = mGTAOOutput.outputAOHandle;
        mLightLoopIntput.directionLightShadowmapHandle = directionalShadowmapHandles;
        mLightLoopIntput.spotShadowmapHandles = spotShadowmapHandle;
        mIndirectLightLoopPass->update(graph, mLightLoopIntput, mLightLoopOutput);
        //=================================================================================


        ////=================================================================================
        //// subsurface scattering pass
        //SubsurfaceScatteringPass::DrawInputParameters mSubsurfaceScatteringInput;
        //SubsurfaceScatteringPass::DrawOutputParameters mSubsurfaceScatteringOutput;

        //mSubsurfaceScatteringInput.perframeBufferHandle = indirectCullOutput.perframeBufferHandle;
        //mSubsurfaceScatteringInput.renderTargetDepthHandle = mDepthPyramidOutput.maxDepthPtyramidHandle;
        //mSubsurfaceScatteringInput.irradianceSourceHandle = mLightLoopOutput.sssDiffuseHandle;
        //mSubsurfaceScatteringInput.specularSourceHandle = mLightLoopOutput.colorHandle;
        //mSubsurfaceScatteringInput.sssBufferTexHandle = mGBufferOutput.albedoHandle;
        //mSubsurfaceScatteringInput.volumeLight3DHandle = mVolumeLightOutput.volumeLightHandle;

        //mSubsurfaceScatteringPass->update(graph, mSubsurfaceScatteringInput, mSubsurfaceScatteringOutput);

        //=================================================================================

        RHI::RgResourceHandle outColorHandle = mLightLoopOutput.specularLightinghandle;
        RHI::RgResourceHandle outDepthHandle = mGBufferOutput.depthHandle;

        //=================================================================================
        // color pyramid
        ColorPyramidPass::DrawInputParameters mColorPyramidInput;
        ColorPyramidPass::DrawOutputParameters mColorPyramidOutput;
        mColorPyramidInput.colorHandle = outColorHandle;
        mColorPyramidInput.perframeBufferHandle = indirectCullOutput.perframeBufferHandle;
        mColorPyramidOutput.colorPyramidHandle = curFrameColorRTHandle;
        mColorPyramidPass->update(graph, mColorPyramidInput, mColorPyramidOutput);

        // 因为color pyramid必须在绘制transparent对象之前
        outColorHandle = mColorPyramidOutput.colorHandle;
        //=================================================================================

        /*
        //=================================================================================
        // indirect opaque draw
        IndirectDrawPass::DrawInputParameters  mDrawIntputParams;
        IndirectDrawPass::DrawOutputParameters mDrawOutputParams;

        mDrawIntputParams.perframeBufferHandle = indirectCullOutput.perframeBufferHandle;
        mDrawIntputParams.renderDataPerDrawHandle = indirectCullOutput.renderDataPerDrawHandle;
        mDrawIntputParams.propertiesPerMaterialHandle = indirectCullOutput.propertiesPerMaterialHandle;
        mDrawIntputParams.opaqueDrawHandle = indirectCullOutput.opaqueDrawHandle.indirectSortBufferHandle;
        mDrawIntputParams.directionalShadowmapTexHandles = directionalShadowmapHandles;
        for (size_t i = 0; i < spotShadowmapHandle.size(); i++)
        {
            mDrawIntputParams.spotShadowmapTexHandles.push_back(spotShadowmapHandle[i]);
        }
        mIndirectOpaqueDrawPass->update(graph, mDrawIntputParams, mDrawOutputParams);
        //=================================================================================
        */

        ///*
        ////=================================================================================
        //// skybox draw
        //SkyBoxPass::DrawInputParameters  mSkyboxIntputParams;
        //SkyBoxPass::DrawOutputParameters mSkyboxOutputParams;

        //mSkyboxIntputParams.perframeBufferHandle    = indirectCullOutput.perframeBufferHandle;
        //mSkyboxOutputParams.renderTargetColorHandle = outColorHandle;
        //mSkyboxOutputParams.renderTargetDepthHandle = mGBufferOutput.depthHandle;
        ////mSkyboxOutputParams.renderTargetColorHandle = mDrawOutputParams.renderTargetColorHandle;
        ////mSkyboxOutputParams.renderTargetDepthHandle = mDrawOutputParams.renderTargetDepthHandle;
        //mSkyBoxPass->update(graph, mSkyboxIntputParams, mSkyboxOutputParams);
        ////=================================================================================
        //*/
        //=================================================================================
        // AtmosphericScattering draw
        AtmosphericScatteringPass::DrawInputParameters mASIntputParams;
        AtmosphericScatteringPass::DrawOutputParameters mASOutputParams;

        mASIntputParams.perframeBufferHandle = indirectCullOutput.perframeBufferHandle;
        mASOutputParams.renderTargetColorHandle = outColorHandle;
        mASOutputParams.renderTargetDepthHandle = outDepthHandle;
        mAtmosphericScatteringPass->update(graph, mASIntputParams, mASOutputParams);

        outColorHandle = mASOutputParams.renderTargetColorHandle;
        outDepthHandle = mASOutputParams.renderTargetDepthHandle;
        //=================================================================================

        ////=================================================================================
        //// VolumeCloud draw
        //VolumeCloudPass::DrawInputParameters  mVCIntputParams;
        //VolumeCloudPass::DrawOutputParameters mVCOutputParams;

        //mVCIntputParams.perframeBufferHandle = indirectCullOutput.perframeBufferHandle;
        //mVCIntputParams.renderTargetColorHandle = mASOutputParams.renderTargetColorHandle;
        //mVCIntputParams.renderTargetDepthHandle  = mASOutputParams.renderTargetDepthHandle;
        //mVolumeCloudPass->update(graph, mVCIntputParams, mVCOutputParams);
        ////=================================================================================


        //=================================================================================
        // indirect transparent draw
        IndirectDrawTransparentPass::DrawInputParameters mDrawTransIntputParams;
        IndirectDrawTransparentPass::DrawOutputParameters mDrawTransOutputParams;

        mDrawTransIntputParams.perframeBufferHandle = indirectCullOutput.perframeBufferHandle;
        mDrawTransIntputParams.renderDataPerDrawHandle = indirectCullOutput.renderDataPerDrawHandle;
        mDrawTransIntputParams.propertiesPerMaterialHandle = indirectCullOutput.propertiesPerMaterialHandle;
        mDrawTransIntputParams.transparentDrawHandle = indirectCullOutput.transparentDrawHandle.
                                                                          indirectSortBufferHandle;
        mDrawTransIntputParams.directionalShadowmapTexHandles = directionalShadowmapHandles;
        mDrawTransIntputParams.spotShadowmapTexHandles = spotShadowmapHandle;
        mDrawTransOutputParams.renderTargetColorHandle = outColorHandle;
        mDrawTransOutputParams.renderTargetDepthHandle = outDepthHandle;
        mIndirectTransparentDrawPass->update(graph, mDrawTransIntputParams, mDrawTransOutputParams);

        outColorHandle = mDrawTransOutputParams.renderTargetColorHandle;
        outDepthHandle = mDrawTransOutputParams.renderTargetDepthHandle;
        //=================================================================================


        //=================================================================================
        // postprocess rendertarget
        PostprocessPasses::PostprocessInputParameters mPostprocessIntputParams;
        PostprocessPasses::PostprocessOutputParameters mPostprocessOutputParams;

        mPostprocessIntputParams.perframeBufferHandle = indirectCullOutput.perframeBufferHandle;
        mPostprocessIntputParams.motionVectorHandle = outColorHandle;
        mPostprocessIntputParams.inputSceneColorHandle = mDrawTransOutputParams.renderTargetColorHandle;
        mPostprocessIntputParams.inputSceneDepthHandle = mDrawTransOutputParams.renderTargetDepthHandle;
        mPostprocessPasses->update(graph, mPostprocessIntputParams, mPostprocessOutputParams);

        outColorHandle = mPostprocessOutputParams.outputColorHandle;
        //=================================================================================


        //=================================================================================
        // display
        DisplayPass::DisplayInputParameters mDisplayIntputParams;
        DisplayPass::DisplayOutputParameters mDisplayOutputParams;

        //mDisplayIntputParams.inputRTColorHandle = mTerrainShadowmapOutputParams.directionalShadowmapHandles[3];
        mDisplayIntputParams.inputRTColorHandle = outColorHandle;
        mDisplayOutputParams.renderTargetColorHandle = renderTargetColorHandle;
        //mDisplayOutputParams.renderTargetColorHandle = backBufColorHandle;
        mDisplayPass->update(graph, mDisplayIntputParams, mDisplayOutputParams);
        //=================================================================================

        //=================================================================================
        if (mUIPass != nullptr)
        {
            UIPass::UIInputParameters mUIIntputParams;
            UIPass::UIOutputParameters mUIOutputParams;

            //mUIIntputParams.renderTargetColorHandle = renderTargetColorHandle;
            mUIIntputParams.renderTargetColorHandle = mDisplayOutputParams.renderTargetColorHandle;
            mUIOutputParams.backBufColorHandle = backBufColorHandle;

            mUIPass->update(graph, mUIIntputParams, mUIOutputParams);
        }
        //=================================================================================

        graph.Execute(context);

        {
            // Transfer the state of the backbuffer to Present
            context->TransitionBarrier(pBackBufferResource,
                                       D3D12_RESOURCE_STATE_PRESENT,
                                       D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
                                       true);
        }

        ////DgmlBuilder Builder("Render Graph");
        ////graph.ExportDgml(Builder);
        ////Builder.SaveAs(std::filesystem::current_path() / "RenderGraph.dgml");
    }

    void DeferredRenderer::PreRender(double deltaTime)
    {
        {
            // 生成specular LD和DFG，生成diffuse radiance
            ToolPass::ToolInputParameters _toolPassInput;
            ToolPass::ToolOutputParameters _toolPassOutput;
            mToolPass->preUpdate(_toolPassInput, _toolPassOutput);
        }
    }

    void DeferredRenderer::PostRender(double deltaTime)
    {
    }

    std::shared_ptr<RHI::D3D12Texture> DeferredRenderer::GetCurrentFrameColorPyramid()
    {
        int curIndex = pDevice->GetLinkedDevice()->m_FrameIndex;
        return p_ColorPyramidRTs[curIndex];
    }

    std::shared_ptr<RHI::D3D12Texture> DeferredRenderer::GetLastFrameColorPyramid()
    {
        int curIndex = pDevice->GetLinkedDevice()->m_FrameIndex;
        curIndex = (curIndex + 1) % 2;
        return p_ColorPyramidRTs[curIndex];
    }
}
