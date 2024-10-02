#include "runtime/function/render/renderer/terrain_cull_pass.h"
#include "runtime/resource/config_manager/config_manager.h"
#include "runtime/function/render/window_system.h"
#include "runtime/function/render/rhi/rhi_core.h"
#include "runtime/core/math/moyu_math2.h"
#include "runtime/function/render/rhi/d3d12/d3d12_graphicsCommon.h"
#include "runtime/function/render/terrain_render_helper.h"
#include "runtime/function/render/renderer/pass_helper.h"
#include "fmt/core.h"
#include <cassert>

namespace MoYu
{
    void IndirectTerrainCullPass::initialize(const TerrainCullInitInfo& init_info)
    {
        ShaderCompiler* m_ShaderCompiler = init_info.m_ShaderCompiler;
        std::filesystem::path m_ShaderRootPath = init_info.m_ShaderRootPath;

        //--------------------------------------------------------------------------
        {
            ShaderCompileOptions shaderCompileOpt = ShaderCompileOptions(L"InitQuadTree");
            shaderCompileOpt.SetDefine(L"INIT_QUAD_TREE", L"1");

            InitQuadTreeCS = m_ShaderCompiler->CompileShader(
                RHI_SHADER_TYPE::Compute, m_ShaderRootPath / "pipeline/Runtime/Tools/Terrain/TerrainBuildCS.hlsl", shaderCompileOpt);

            RHI::RootSignatureDesc rootSigDesc =
                RHI::RootSignatureDesc()
                .Add32BitConstants<0, 0>(16)
                .AddStaticSampler<10, 0>(D3D12_FILTER::D3D12_FILTER_ANISOTROPIC, D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_WRAP, 8)
                .AllowInputLayout()
                .AllowResourceDescriptorHeapIndexing()
                .AllowSampleDescriptorHeapIndexing();

            pInitQuadTreeSignature = std::make_shared<RHI::D3D12RootSignature>(m_Device, rootSigDesc);

            struct PsoStream
            {
                PipelineStateStreamRootSignature RootSignature;
                PipelineStateStreamCS            CS;
            } psoStream;
            psoStream.RootSignature = PipelineStateStreamRootSignature(pInitQuadTreeSignature.get());
            psoStream.CS = &InitQuadTreeCS;
            PipelineStateStreamDesc psoDesc = { sizeof(PsoStream), &psoStream };

            pInitQuadTreePSO =
                std::make_shared<RHI::D3D12PipelineState>(m_Device, L"InitQuadTreePSO", psoDesc);
        }
        //--------------------------------------------------------------------------

        //--------------------------------------------------------------------------
        {
            ShaderCompileOptions shaderCompileOpt = ShaderCompileOptions(L"TraverseQuadTree");
            shaderCompileOpt.SetDefine(L"TRAVERSE_QUAD_TREE", L"1");

            TraverseQuadTreeCS = m_ShaderCompiler->CompileShader(
                RHI_SHADER_TYPE::Compute, m_ShaderRootPath / "pipeline/Runtime/Tools/Terrain/TerrainBuildCS.hlsl", shaderCompileOpt);

            RHI::RootSignatureDesc rootSigDesc =
                RHI::RootSignatureDesc()
                .Add32BitConstants<0, 0>(16)
                .AddStaticSampler<10, 0>(D3D12_FILTER::D3D12_FILTER_ANISOTROPIC, D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_WRAP, 8)
                .AllowInputLayout()
                .AllowResourceDescriptorHeapIndexing()
                .AllowSampleDescriptorHeapIndexing();

            pTraverseQuadTreeSignature = std::make_shared<RHI::D3D12RootSignature>(m_Device, rootSigDesc);

            struct PsoStream
            {
                PipelineStateStreamRootSignature RootSignature;
                PipelineStateStreamCS            CS;
            } psoStream;
            psoStream.RootSignature = PipelineStateStreamRootSignature(pTraverseQuadTreeSignature.get());
            psoStream.CS = &TraverseQuadTreeCS;
            PipelineStateStreamDesc psoDesc = { sizeof(PsoStream), &psoStream };

            pTraverseQuadTreePSO =
                std::make_shared<RHI::D3D12PipelineState>(m_Device, L"TraverseQuadTreePSO", psoDesc);
        }
        //--------------------------------------------------------------------------

        //--------------------------------------------------------------------------
        {
            ShaderCompileOptions shaderCompileOpt = ShaderCompileOptions(L"BuildLodMap");
            shaderCompileOpt.SetDefine(L"BUILD_LODMAP", L"1");

            BuildLodMapCS = m_ShaderCompiler->CompileShader(
                RHI_SHADER_TYPE::Compute, m_ShaderRootPath / "pipeline/Runtime/Tools/Terrain/TerrainBuildCS.hlsl", shaderCompileOpt);

            RHI::RootSignatureDesc rootSigDesc =
                RHI::RootSignatureDesc()
                .Add32BitConstants<0, 0>(16)
                .AddStaticSampler<10, 0>(D3D12_FILTER::D3D12_FILTER_ANISOTROPIC, D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_WRAP, 8)
                .AllowInputLayout()
                .AllowResourceDescriptorHeapIndexing()
                .AllowSampleDescriptorHeapIndexing();

            pBuildLodMapSignature = std::make_shared<RHI::D3D12RootSignature>(m_Device, rootSigDesc);

            struct PsoStream
            {
                PipelineStateStreamRootSignature RootSignature;
                PipelineStateStreamCS            CS;
            } psoStream;
            psoStream.RootSignature = PipelineStateStreamRootSignature(pBuildLodMapSignature.get());
            psoStream.CS = &BuildLodMapCS;
            PipelineStateStreamDesc psoDesc = { sizeof(PsoStream), &psoStream };

            pBuildLodMapPSO =
                std::make_shared<RHI::D3D12PipelineState>(m_Device, L"BuildLodMapPSO", psoDesc);
        }

        //--------------------------------------------------------------------------

        //--------------------------------------------------------------------------
        {
            ShaderCompileOptions shaderCompileOpt = ShaderCompileOptions(L"BuildPatches");
            shaderCompileOpt.SetDefine(L"BUILD_PATCHES", L"1");
            shaderCompileOpt.SetDefine(L"ENABLE_FRUS_CULL", L"1");

            BuildPatchesCS = m_ShaderCompiler->CompileShader(
                RHI_SHADER_TYPE::Compute, m_ShaderRootPath / "pipeline/Runtime/Tools/Terrain/TerrainBuildCS.hlsl", shaderCompileOpt);

            RHI::RootSignatureDesc rootSigDesc =
                RHI::RootSignatureDesc()
                .Add32BitConstants<0, 0>(16)
                .AddStaticSampler<10, 0>(D3D12_FILTER::D3D12_FILTER_ANISOTROPIC, D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_WRAP, 8)
                .AllowInputLayout()
                .AllowResourceDescriptorHeapIndexing()
                .AllowSampleDescriptorHeapIndexing();

            pBuildPatchesSignature = std::make_shared<RHI::D3D12RootSignature>(m_Device, rootSigDesc);

            struct PsoStream
            {
                PipelineStateStreamRootSignature RootSignature;
                PipelineStateStreamCS            CS;
            } psoStream;
            psoStream.RootSignature = PipelineStateStreamRootSignature(pBuildPatchesSignature.get());
            psoStream.CS = &BuildPatchesCS;
            PipelineStateStreamDesc psoDesc = { sizeof(PsoStream), &psoStream };

            pBuildPatchesPSO =
                std::make_shared<RHI::D3D12PipelineState>(m_Device, L"BuildPatchesPSO", psoDesc);
        }

        //--------------------------------------------------------------------------

        iMinMaxHeightReady = false;
    }

    void IndirectTerrainCullPass::prepareMeshData(std::shared_ptr<RenderResource> render_resource)
    {
        if (pMinHeightMap == nullptr || pMaxHeightMap == nullptr)
        {
            InternalTerrainRenderer& internalTerrainRenderer = m_render_scene->m_terrain_renderers[0].internalTerrainRenderer;

            std::shared_ptr<RHI::D3D12Texture> terrainHeightmap = internalTerrainRenderer.ref_terrain.terrain_heightmap;
            auto heightmapDesc = terrainHeightmap->GetDesc();

            int mipLevel = glm::log2(glm::fmin((float)heightmapDesc.Width, heightmapDesc.Height));

            pMinHeightMap =
                RHI::D3D12Texture::Create2D(m_Device->GetLinkedDevice(),
                    heightmapDesc.Width,
                    heightmapDesc.Height,
                    mipLevel,
                    heightmapDesc.Format,
                    RHI::RHISurfaceCreateRandomWrite | RHI::RHISurfaceCreateMipmap,
                    1,
                    L"TerrainMinHeightmap",
                    D3D12_RESOURCE_STATE_COMMON,
                    std::nullopt);

            pMaxHeightMap =
                RHI::D3D12Texture::Create2D(m_Device->GetLinkedDevice(),
                    heightmapDesc.Width,
                    heightmapDesc.Height,
                    mipLevel,
                    heightmapDesc.Format,
                    RHI::RHISurfaceCreateRandomWrite | RHI::RHISurfaceCreateMipmap,
                    1,
                    L"TerrainMaxHeightmap",
                    D3D12_RESOURCE_STATE_COMMON,
                    std::nullopt);
        }

        if (pLodMap == nullptr)
        {
            pLodMap =
                RHI::D3D12Texture::Create2D(
                    m_Device->GetLinkedDevice(),
                    160,
                    160,
                    1,
                    DXGI_FORMAT_R8_SNORM,
                    RHI::RHISurfaceCreateRandomWrite,
                    1,
                    L"LodMap",
                    D3D12_RESOURCE_STATE_COMMON,
                    std::nullopt);

        }

        if (mTerrainConsBuffer == nullptr)
        {
            mTerrainConsBuffer =
                RHI::D3D12Buffer::Create(
                    m_Device->GetLinkedDevice(),
                    RHI::RHIBufferTargetNone,
                    1,
                    MoYu::AlignUp(sizeof(TerrainConsBuffer), 256),
                    L"TerrainConsBuffer",
                    RHI::RHIBufferModeDynamic,
                    D3D12_RESOURCE_STATE_GENERIC_READ);
        }

        if (mTerrainDirConsBuffer == nullptr)
        {
            mTerrainDirConsBuffer =
                RHI::D3D12Buffer::Create(
                    m_Device->GetLinkedDevice(),
                    RHI::RHIBufferTargetNone,
                    1,
                    MoYu::AlignUp(sizeof(TerrainConsBuffer), 256),
                    L"TerrainDirConsBuffer",
                    RHI::RHIBufferModeDynamic,
                    D3D12_RESOURCE_STATE_GENERIC_READ);
        }


        HLSL::FrameUniforms& frameUniform = render_resource->m_FrameUniforms;
        glm::float3 cameraPosition = frameUniform.cameraUniform._WorldSpaceCameraPos;
        glm::float4x4 cameraViewProj = frameUniform.cameraUniform._ViewProjMatrix;

        InternalTerrainRenderer& internalTerrainRenderer = m_render_scene->m_terrain_renderers[0].internalTerrainRenderer;
        glm::float3 terrainSize = internalTerrainRenderer.ref_terrain.terrain_size;
        
        int nodeCount = MAX_LOD_NODE_COUNT;
        for (int lod = MAX_TERRAIN_LOD; lod >= 0; lod--)
        {
            int nodeSize = terrainSize.x / nodeCount;
            int patchExtent = nodeSize / PATCH_MESH_GRID_COUNT;
            int sectorCountPerNode = (int)glm::pow(2, lod);
            worldLODParams[lod] = glm::float4(nodeSize, patchExtent, nodeCount, sectorCountPerNode);
            nodeCount *= 2;
        }


        int nodeIdOffset = 0;
        for (int lod = MAX_TERRAIN_LOD; lod >= 0; lod--)
        {
            nodeIDOffsetLOD[lod] = nodeIdOffset;
            nodeIdOffset += (int)(worldLODParams[lod].z * worldLODParams[lod].z);
        }

        int maxNodeCount = nodeIdOffset;

        glm::float4x4 modelMatrix = internalTerrainRenderer.model_matrix;

        TerrainConsBuffer TerrainCB;
        TerrainCB.TerrainModelMatrix = modelMatrix;
        TerrainCB.CameraViewProj = cameraViewProj;
        TerrainCB.CameraPositionWS = cameraPosition;
        TerrainCB.BoundsHeightRedundance = 0.1f;
        TerrainCB.WorldSize = terrainSize;
        TerrainCB.NodeEvaluationC = glm::float4(1, 0, 0, 0);
        memcpy(TerrainCB.WorldLodParams, worldLODParams, sizeof(glm::float4) * (MAX_TERRAIN_LOD + 1));
        memcpy(TerrainCB.NodeIDOffsetOfLOD, nodeIDOffsetLOD, sizeof(uint32_t) * (MAX_TERRAIN_LOD + 1));

        memcpy(mTerrainConsBuffer->GetCpuVirtualAddress<TerrainConsBuffer>(), &TerrainCB, sizeof(TerrainConsBuffer));

        glm::float3 dirLightPosWS = frameUniform.lightDataUniform.directionalLightData.positionRWS;
        int dirShadowIndex = frameUniform.lightDataUniform.directionalLightData.shadowIndex;
        HLSL::HDShadowData dirShadowData = frameUniform.lightDataUniform.shadowDatas[dirShadowIndex];
        glm::float4x4 dirShadowViewProjMatrix = dirShadowData.viewProjMatrix;

        TerrainConsBuffer TerrainDirCB;
        memcpy(&TerrainDirCB, &TerrainCB, sizeof(TerrainConsBuffer));
        TerrainDirCB.CameraViewProj = dirShadowViewProjMatrix;
        TerrainDirCB.CameraPositionWS = dirLightPosWS;

        memcpy(mTerrainDirConsBuffer->GetCpuVirtualAddress<TerrainConsBuffer>(), &TerrainDirCB, sizeof(TerrainConsBuffer));

        //--------------------------------------------------------------------------

        if (TempNodeList[0] == nullptr)
        {
            for (int i = 0; i < 2; i++)
            {
                TempNodeList[i] = RHI::D3D12Buffer::Create(
                    m_Device->GetLinkedDevice(),
                    RHI::RHIBufferRandomReadWrite | RHI::RHIBufferTargetStructured | RHI::RHIBufferTargetCounter,
                    1024,
                    sizeof(glm::uvec2),
                    fmt::format(L"TempNodeList_{}", i));
            }
            FinalNodeList = RHI::D3D12Buffer::Create(
                m_Device->GetLinkedDevice(),
                RHI::RHIBufferRandomReadWrite | RHI::RHIBufferTargetStructured | RHI::RHIBufferTargetCounter,
                1024,
                sizeof(glm::uvec3),
                L"FinalNodeList");

            NodeDescriptors = RHI::D3D12Buffer::Create(
                m_Device->GetLinkedDevice(),
                RHI::RHIBufferRandomReadWrite | RHI::RHIBufferTargetStructured | RHI::RHIBufferTargetCounter,
                maxNodeCount,
                sizeof(uint32_t),
                L"NodeDescriptors");
        }

        if (CulledPatchListBuffer == nullptr)
        {
            CulledPatchListBuffer = RHI::D3D12Buffer::Create(
                m_Device->GetLinkedDevice(),
                RHI::RHIBufferRandomReadWrite | RHI::RHIBufferTargetStructured | RHI::RHIBufferTargetCounter,
                maxNodeCount,
                sizeof(HLSL::TerrainRenderPatch),
                L"CulledPatchListBuffer");
        }

        if (CulledDirPatchListBuffer == nullptr)
        {
            CulledDirPatchListBuffer = RHI::D3D12Buffer::Create(
                m_Device->GetLinkedDevice(),
                RHI::RHIBufferRandomReadWrite | RHI::RHIBufferTargetStructured | RHI::RHIBufferTargetCounter,
                maxNodeCount,
                sizeof(HLSL::TerrainRenderPatch),
                L"CulledDirPatchListBuffer");
        }

        if (pTerrainMatPropertiesBuffer == nullptr)
        {
            pTerrainMatPropertiesBuffer =
                RHI::D3D12Buffer::Create(
                    m_Device->GetLinkedDevice(),
                    RHI::RHIBufferTargetStructured,
                    maxNodeCount,
                    sizeof(HLSL::PropertiesPerMaterial),
                    L"TerrainMaterialProperty",
                    RHI::RHIBufferMode::RHIBufferModeDynamic);
        }
        if (pTerrainRenderDataBuffer == nullptr)
        {
            pTerrainRenderDataBuffer =
                RHI::D3D12Buffer::Create(
                    m_Device->GetLinkedDevice(),
                    RHI::RHIBufferTargetStructured,
                    maxNodeCount,
                    sizeof(HLSL::TerrainRenderData),
                    L"TerrainRenderData", 
                    RHI::RHIBufferMode::RHIBufferModeDynamic);
        }

        //--------------------------------------------------------------------------

        if (camUploadPatchCmdSigBuffer == nullptr || camPatchCmdSigBuffer == nullptr || dirPatchCmdSigBuffer == nullptr)
        {
            camUploadPatchCmdSigBuffer =
                RHI::D3D12Buffer::Create(
                    m_Device->GetLinkedDevice(),
                    RHI::RHIBufferTargetStructured,
                    1, MoYu::AlignUp(sizeof(HLSL::ToDrawCommandSignatureParams), 256),
                    L"UploadPatchCommandSignatureBuffer",
                    RHI::RHIBufferMode::RHIBufferModeDynamic);

            camPatchCmdSigBuffer = 
                RHI::D3D12Buffer::Create(
                    m_Device->GetLinkedDevice(),
                    RHI::RHIBufferTargetStructured | RHI::RHIBufferTargetIndirectArgs,
                    1, MoYu::AlignUp(sizeof(HLSL::ToDrawCommandSignatureParams), 256),
                    L"PatchCommandSignatureBuffer");

            dirPatchCmdSigBuffer =
                RHI::D3D12Buffer::Create(
                    m_Device->GetLinkedDevice(),
                    RHI::RHIBufferTargetStructured | RHI::RHIBufferTargetIndirectArgs,
                    1, MoYu::AlignUp(sizeof(HLSL::ToDrawCommandSignatureParams), 256),
                    L"PatchDirCommandSignatureBuffer");

        }

        //--------------------------------------------------------------------------
        
        std::vector<CachedTerrainRenderer>& mesh_renderers = m_render_scene->m_terrain_renderers;
        if (mesh_renderers.size() > 0)
        {
            CachedTerrainRenderer& terrainRenderer = mesh_renderers[0];
            InternalTerrainRenderer& internalTerrainRenderer = terrainRenderer.internalTerrainRenderer;
            //glm::int3 terrainSize = internalTerrainRenderer.ref_terrain.terrain_size;

            InternalMesh& patchMesh = internalTerrainRenderer.ref_terrain.patch_mesh;

            D3D12_VERTEX_BUFFER_VIEW curVertexBufferView = patchMesh.vertex_buffer.vertex_buffer->GetVertexBufferView();
            D3D12_INDEX_BUFFER_VIEW curIndexBufferView = patchMesh.index_buffer.index_buffer->GetIndexBufferView();

            D3D12_DRAW_INDEXED_ARGUMENTS drawIndexedArgs;
            drawIndexedArgs.IndexCountPerInstance = patchMesh.index_buffer.index_count;
            drawIndexedArgs.InstanceCount = 1;
            drawIndexedArgs.StartIndexLocation = 0;
            drawIndexedArgs.BaseVertexLocation = 0;
            drawIndexedArgs.StartInstanceLocation = 0;

            HLSL::ToDrawCommandSignatureParams drawPatchParams;
            memcpy(&drawPatchParams.vertexBufferView, &curVertexBufferView, sizeof(D3D12_VERTEX_BUFFER_VIEW));
            memcpy(&drawPatchParams.indexBufferView, &curIndexBufferView, sizeof(D3D12_INDEX_BUFFER_VIEW));
            memcpy(&drawPatchParams.drawIndexedArguments, &drawIndexedArgs, sizeof(D3D12_DRAW_INDEXED_ARGUMENTS));

            memcpy(camUploadPatchCmdSigBuffer->GetCpuVirtualAddress<HLSL::ToDrawCommandSignatureParams>(), &drawPatchParams, sizeof(HLSL::ToDrawCommandSignatureParams));



            HLSL::TerrainRenderData terrainRenderData;
            terrainRenderData.objectToWorldMatrix = internalTerrainRenderer.model_matrix;
            terrainRenderData.worldToObjectMatrix = internalTerrainRenderer.model_matrix_inverse;
            terrainRenderData.prevObjectToWorldMatrix = internalTerrainRenderer.prev_model_matrix;
            terrainRenderData.prevWorldToObjectMatrix = internalTerrainRenderer.prev_model_matrix_inverse;
            terrainRenderData.terrainSize = glm::float4(terrainSize, 0);

            memcpy(pTerrainRenderDataBuffer->GetCpuVirtualAddress<HLSL::TerrainRenderData>(), &terrainRenderData, sizeof(HLSL::TerrainRenderData));



            InternalStandardLightMaterial& m_InteralMat = terrainRenderer.internalTerrainRenderer.ref_material.m_intenral_light_mat;

            HLSL::PropertiesPerMaterial curPropertiesPerMaterial {};
            curPropertiesPerMaterial._BaseColorMapIndex = m_InteralMat._BaseColorMap->GetDefaultSRV()->GetIndex();
            curPropertiesPerMaterial._MaskMapIndex = m_InteralMat._MaskMap->GetDefaultSRV()->GetIndex();
            curPropertiesPerMaterial._NormalMapIndex = m_InteralMat._NormalMap->GetDefaultSRV()->GetIndex();
            curPropertiesPerMaterial._NormalMapOSIndex = m_InteralMat._NormalMapOS->GetDefaultSRV()->GetIndex();
            curPropertiesPerMaterial._BentNormalMapIndex = m_InteralMat._BentNormalMap->GetDefaultSRV()->GetIndex();
            curPropertiesPerMaterial._BentNormalMapOSIndex = m_InteralMat._BentNormalMapOS->GetDefaultSRV()->GetIndex();
            curPropertiesPerMaterial._HeightMapIndex = m_InteralMat._HeightMap->GetDefaultSRV()->GetIndex();
            curPropertiesPerMaterial._DetailMapIndex = m_InteralMat._DetailMap->GetDefaultSRV()->GetIndex();
            curPropertiesPerMaterial._TangentMapIndex = m_InteralMat._TangentMap->GetDefaultSRV()->GetIndex();
            curPropertiesPerMaterial._TangentMapOSIndex = m_InteralMat._TangentMapOS->GetDefaultSRV()->GetIndex();
            curPropertiesPerMaterial._AnisotropyMapIndex = m_InteralMat._AnisotropyMap->GetDefaultSRV()->GetIndex();
            curPropertiesPerMaterial._SubsurfaceMaskMapIndex = m_InteralMat._SubsurfaceMaskMap->GetDefaultSRV()->GetIndex();
            curPropertiesPerMaterial._TransmissionMaskMapIndex = m_InteralMat._TransmissionMaskMap->GetDefaultSRV()->GetIndex();
            curPropertiesPerMaterial._ThicknessMapIndex = m_InteralMat._ThicknessMap->GetDefaultSRV()->GetIndex();
            curPropertiesPerMaterial._IridescenceThicknessMapIndex = m_InteralMat._IridescenceThicknessMap->GetDefaultSRV()->GetIndex();
            curPropertiesPerMaterial._IridescenceMaskMapIndex = m_InteralMat._IridescenceMaskMap->GetDefaultSRV()->GetIndex();
            curPropertiesPerMaterial._CoatMaskMapIndex = m_InteralMat._CoatMaskMap->GetDefaultSRV()->GetIndex();
            curPropertiesPerMaterial._EmissiveColorMapIndex = m_InteralMat._EmissiveColorMap->GetDefaultSRV()->GetIndex();
            curPropertiesPerMaterial._TransmittanceColorMapIndex = m_InteralMat._TransmittanceColorMap->GetDefaultSRV()->GetIndex();
            curPropertiesPerMaterial._BaseColor = m_InteralMat._BaseColor;
            curPropertiesPerMaterial._BaseColorMap_ST = m_InteralMat._BaseColorMap_ST;
            curPropertiesPerMaterial._BaseColorMap_TexelSize = m_InteralMat._BaseColorMap_TexelSize;
            curPropertiesPerMaterial._BaseColorMap_MipInfo = m_InteralMat._BaseColorMap_MipInfo;
            curPropertiesPerMaterial._Metallic = m_InteralMat._Metallic;
            curPropertiesPerMaterial._Smoothness = m_InteralMat._Smoothness;
            curPropertiesPerMaterial._MetallicRemapMin = m_InteralMat._MetallicRemapMin;
            curPropertiesPerMaterial._MetallicRemapMax = m_InteralMat._MetallicRemapMax;
            curPropertiesPerMaterial._SmoothnessRemapMin = m_InteralMat._SmoothnessRemapMin;
            curPropertiesPerMaterial._SmoothnessRemapMax = m_InteralMat._SmoothnessRemapMax;
            curPropertiesPerMaterial._AlphaRemapMin = m_InteralMat._AlphaRemapMin;
            curPropertiesPerMaterial._AlphaRemapMax = m_InteralMat._AlphaRemapMax;
            curPropertiesPerMaterial._AORemapMin = m_InteralMat._AORemapMin;
            curPropertiesPerMaterial._AORemapMax = m_InteralMat._AORemapMax;
            curPropertiesPerMaterial._NormalScale = m_InteralMat._NormalScale;
            curPropertiesPerMaterial._HeightAmplitude = m_InteralMat._HeightAmplitude;
            curPropertiesPerMaterial._HeightCenter = m_InteralMat._HeightCenter;
            curPropertiesPerMaterial._HeightMapParametrization = m_InteralMat._HeightMapParametrization;
            curPropertiesPerMaterial._HeightOffset = m_InteralMat._HeightOffset;
            curPropertiesPerMaterial._HeightMin = m_InteralMat._HeightMin;
            curPropertiesPerMaterial._HeightMax = m_InteralMat._HeightMax;
            curPropertiesPerMaterial._HeightTessAmplitude = m_InteralMat._HeightTessAmplitude;
            curPropertiesPerMaterial._HeightTessCenter = m_InteralMat._HeightTessCenter;
            curPropertiesPerMaterial._HeightPoMAmplitude = m_InteralMat._HeightPoMAmplitude;
            curPropertiesPerMaterial._DetailMap_ST = m_InteralMat._DetailMap_ST;
            curPropertiesPerMaterial._DetailAlbedoScale = m_InteralMat._DetailAlbedoScale;
            curPropertiesPerMaterial._DetailNormalScale = m_InteralMat._DetailNormalScale;
            curPropertiesPerMaterial._DetailSmoothnessScale = m_InteralMat._DetailSmoothnessScale;
            curPropertiesPerMaterial._Anisotropy = m_InteralMat._Anisotropy;
            curPropertiesPerMaterial._DiffusionProfileHash = m_InteralMat._DiffusionProfileHash;
            curPropertiesPerMaterial._SubsurfaceMask = m_InteralMat._SubsurfaceMask;
            curPropertiesPerMaterial._TransmissionMask = m_InteralMat._TransmissionMask;
            curPropertiesPerMaterial._Thickness = m_InteralMat._Thickness;
            curPropertiesPerMaterial._ThicknessRemap = m_InteralMat._ThicknessRemap;
            curPropertiesPerMaterial._IridescenceThicknessRemap = m_InteralMat._IridescenceThicknessRemap;
            curPropertiesPerMaterial._IridescenceThickness = m_InteralMat._IridescenceThickness;
            curPropertiesPerMaterial._IridescenceMask = m_InteralMat._IridescenceMask;
            curPropertiesPerMaterial._CoatMask = m_InteralMat._CoatMask;
            curPropertiesPerMaterial._EnergyConservingSpecularColor = m_InteralMat._EnergyConservingSpecularColor;
            curPropertiesPerMaterial._SpecularOcclusionMode = m_InteralMat._SpecularOcclusionMode;
            curPropertiesPerMaterial._EmissiveColor = m_InteralMat._EmissiveColor;
            curPropertiesPerMaterial._AlbedoAffectEmissive = m_InteralMat._AlbedoAffectEmissive;
            curPropertiesPerMaterial._EmissiveIntensityUnit = m_InteralMat._EmissiveIntensityUnit;
            curPropertiesPerMaterial._UseEmissiveIntensity = m_InteralMat._UseEmissiveIntensity;
            curPropertiesPerMaterial._EmissiveIntensity = m_InteralMat._EmissiveIntensity;
            curPropertiesPerMaterial._EmissiveExposureWeight = m_InteralMat._EmissiveExposureWeight;
            curPropertiesPerMaterial._UseShadowThreshold = m_InteralMat._UseShadowThreshold;
            curPropertiesPerMaterial._AlphaCutoffEnable = m_InteralMat._AlphaCutoffEnable;
            curPropertiesPerMaterial._AlphaCutoff = m_InteralMat._AlphaCutoff;
            curPropertiesPerMaterial._AlphaCutoffShadow = m_InteralMat._AlphaCutoffShadow;
            curPropertiesPerMaterial._AlphaCutoffPrepass = m_InteralMat._AlphaCutoffPrepass;
            curPropertiesPerMaterial._AlphaCutoffPostpass = m_InteralMat._AlphaCutoffPostpass;
            curPropertiesPerMaterial._TransparentDepthPrepassEnable = m_InteralMat._TransparentDepthPrepassEnable;
            curPropertiesPerMaterial._TransparentBackfaceEnable = m_InteralMat._TransparentBackfaceEnable;
            curPropertiesPerMaterial._TransparentDepthPostpassEnable = m_InteralMat._TransparentDepthPostpassEnable;
            curPropertiesPerMaterial._TransparentSortPriority = m_InteralMat._TransparentSortPriority;
            curPropertiesPerMaterial._RefractionModel = m_InteralMat._RefractionModel;
            curPropertiesPerMaterial._Ior = m_InteralMat._Ior;
            curPropertiesPerMaterial._TransmittanceColor = m_InteralMat._TransmittanceColor;
            curPropertiesPerMaterial._ATDistance = m_InteralMat._ATDistance;
            curPropertiesPerMaterial._TransparentWritingMotionVec = m_InteralMat._TransparentWritingMotionVec;
            curPropertiesPerMaterial._SurfaceType = m_InteralMat._SurfaceType;
            curPropertiesPerMaterial._BlendMode = m_InteralMat._BlendMode;
            curPropertiesPerMaterial._SrcBlend = m_InteralMat._SrcBlend;
            curPropertiesPerMaterial._DstBlend = m_InteralMat._DstBlend;
            curPropertiesPerMaterial._AlphaSrcBlend = m_InteralMat._AlphaSrcBlend;
            curPropertiesPerMaterial._AlphaDstBlend = m_InteralMat._AlphaDstBlend;
            curPropertiesPerMaterial._EnableFogOnTransparent = m_InteralMat._EnableFogOnTransparent;
            curPropertiesPerMaterial._DoubleSidedEnable = m_InteralMat._DoubleSidedEnable;
            curPropertiesPerMaterial._DoubleSidedNormalMode = m_InteralMat._DoubleSidedNormalMode;
            curPropertiesPerMaterial._DoubleSidedConstants = m_InteralMat._DoubleSidedConstants;
            curPropertiesPerMaterial._DoubleSidedGIMode = m_InteralMat._DoubleSidedGIMode;
            curPropertiesPerMaterial._UVBase = m_InteralMat._UVBase;
            curPropertiesPerMaterial._ObjectSpaceUVMapping = m_InteralMat._ObjectSpaceUVMapping;
            curPropertiesPerMaterial._TexWorldScale = m_InteralMat._TexWorldScale;
            curPropertiesPerMaterial._UVMappingMask = m_InteralMat._UVMappingMask;
            curPropertiesPerMaterial._NormalMapSpace = m_InteralMat._NormalMapSpace;
            curPropertiesPerMaterial._MaterialID = m_InteralMat._MaterialID;
            curPropertiesPerMaterial._TransmissionEnable = m_InteralMat._TransmissionEnable;
            curPropertiesPerMaterial._PPDMinSamples = m_InteralMat._PPDMinSamples;
            curPropertiesPerMaterial._PPDMaxSamples = m_InteralMat._PPDMaxSamples;
            curPropertiesPerMaterial._PPDLodThreshold = m_InteralMat._PPDLodThreshold;
            curPropertiesPerMaterial._PPDPrimitiveLength = m_InteralMat._PPDPrimitiveLength;
            curPropertiesPerMaterial._PPDPrimitiveWidth = m_InteralMat._PPDPrimitiveWidth;
            curPropertiesPerMaterial._InvPrimScale = m_InteralMat._InvPrimScale;
            curPropertiesPerMaterial._UVDetailsMappingMask = m_InteralMat._UVDetailsMappingMask;
            curPropertiesPerMaterial._UVDetail = m_InteralMat._UVDetail;
            curPropertiesPerMaterial._LinkDetailsWithBase = m_InteralMat._LinkDetailsWithBase;
            curPropertiesPerMaterial._EmissiveColorMode = m_InteralMat._EmissiveColorMode;
            curPropertiesPerMaterial._UVEmissive = m_InteralMat._UVEmissive;

            memcpy(pTerrainMatPropertiesBuffer->GetCpuVirtualAddress<HLSL::PropertiesPerMaterial>(), &curPropertiesPerMaterial, sizeof(HLSL::PropertiesPerMaterial));
        }


        //--------------------------------------------------------------------------

        traverseDispatchArgsBufferDesc =
            RHI::RgBufferDesc("TraverseDisaptchArgs")
            .SetSize(1, sizeof(D3D12_DISPATCH_ARGUMENTS))
            .SetRHIBufferMode(RHI::RHIBufferMode::RHIBufferModeImmutable)
            .SetRHIBufferTarget(RHI::RHIBufferTarget::RHIBufferTargetIndirectArgs | RHI::RHIBufferTarget::RHIBufferRandomReadWrite | RHI::RHIBufferTarget::RHIBufferTargetRaw);

        buildPatchArgsBufferDesc =
            RHI::RgBufferDesc("BuildPatchArgs")
            .SetSize(1, sizeof(D3D12_DISPATCH_ARGUMENTS))
            .SetRHIBufferMode(RHI::RHIBufferMode::RHIBufferModeImmutable)
            .SetRHIBufferTarget(RHI::RHIBufferTarget::RHIBufferTargetIndirectArgs | RHI::RHIBufferTarget::RHIBufferRandomReadWrite | RHI::RHIBufferTarget::RHIBufferTargetRaw);
    }

    void IndirectTerrainCullPass::update(RHI::RenderGraph& graph, TerrainCullInput& passInput, TerrainCullOutput& passOutput)
    {
        InternalTerrainRenderer& internalTerrainRenderer = m_render_scene->m_terrain_renderers[0].internalTerrainRenderer;
        std::shared_ptr<RHI::D3D12Texture> terrainHeightmap = internalTerrainRenderer.ref_terrain.terrain_heightmap;
        std::shared_ptr<RHI::D3D12Texture> terrainNormalmap = internalTerrainRenderer.ref_terrain.terrain_normalmap;

        RHI::RgResourceHandle terrainHeightmapHandle = GImport(graph, terrainHeightmap.get());
        RHI::RgResourceHandle terrainNormalmapHandle = GImport(graph, terrainNormalmap.get());

        RHI::RgResourceHandle terrainMinHeightHandle = GImport(graph, pMinHeightMap.get());
        RHI::RgResourceHandle terrainMaxHeightHandle = GImport(graph, pMaxHeightMap.get());

        RHI::RgResourceHandle terrainRenderDataHandle = GImport(graph, pTerrainRenderDataBuffer.get());
        RHI::RgResourceHandle terrainMatPropertiesHandle = GImport(graph, pTerrainMatPropertiesBuffer.get());
        
        if (!iMinMaxHeightReady)
        {
            iMinMaxHeightReady = true;

            RHI::RenderPass& genHeightMapMipMapPass = graph.AddRenderPass("GenerateHeightMapMipMapPass");

            genHeightMapMipMapPass.Read(terrainHeightmapHandle, true);
            genHeightMapMipMapPass.Read(terrainMinHeightHandle, true);
            genHeightMapMipMapPass.Read(terrainMaxHeightHandle, true);
            genHeightMapMipMapPass.Write(terrainMinHeightHandle, true);
            genHeightMapMipMapPass.Write(terrainMaxHeightHandle, true);

            genHeightMapMipMapPass.Execute([=](RHI::RenderGraphRegistry* registry, RHI::D3D12CommandContext* context) {
                RHI::D3D12ComputeContext* pContext = context->GetComputeContext();

                {
                    pContext->TransitionBarrier(RegGetTex(terrainHeightmapHandle), D3D12_RESOURCE_STATE_COPY_SOURCE);
                    pContext->TransitionBarrier(RegGetTex(terrainMinHeightHandle), D3D12_RESOURCE_STATE_COPY_DEST, 0);
                    pContext->TransitionBarrier(RegGetTex(terrainMaxHeightHandle), D3D12_RESOURCE_STATE_COPY_DEST, 0);
                    pContext->FlushResourceBarriers();

                    const CD3DX12_TEXTURE_COPY_LOCATION src(terrainHeightmap->GetResource(), 0);

                    const CD3DX12_TEXTURE_COPY_LOCATION dst_min(RegGetTex(terrainMinHeightHandle)->GetResource(), 0);
                    context->GetGraphicsCommandList()->CopyTextureRegion(&dst_min, 0, 0, 0, &src, nullptr);

                    const CD3DX12_TEXTURE_COPY_LOCATION dst_max(RegGetTex(terrainMaxHeightHandle)->GetResource(), 0);
                    context->GetGraphicsCommandList()->CopyTextureRegion(&dst_max, 0, 0, 0, &src, nullptr);

                    context->FlushResourceBarriers();
                }

                //--------------------------------------------------
                // Generate MinHeightMap
                //--------------------------------------------------
                {
                    RHI::D3D12Texture* _SrcTexture = RegGetTex(terrainMinHeightHandle);
                    generateMipmapForTerrainHeightmap(pContext, _SrcTexture, true);
                }

                //--------------------------------------------------
                // Generate MaxHeightMap
                //--------------------------------------------------
                {
                    RHI::D3D12Texture* _SrcTexture = RegGetTex(terrainMaxHeightHandle);
                    generateMipmapForTerrainHeightmap(pContext, _SrcTexture, false);
                }

            });
        }

        //------------------------------------------------------------------------------------

        RHI::RgResourceHandle traverseDispatchArgsHandle = graph.Create<RHI::D3D12Buffer>(traverseDispatchArgsBufferDesc);

        RHI::RgResourceHandle TempNodeListHandle[2];
        TempNodeListHandle[0] = GImport(graph, TempNodeList[0].get());
        TempNodeListHandle[1] = GImport(graph, TempNodeList[1].get());
        RHI::RgResourceHandle FinalNodeListHandle = GImport(graph, FinalNodeList.get());
        RHI::RgResourceHandle NodeDescriptorsHandle = GImport(graph, NodeDescriptors.get());

        RHI::RgResourceHandle terrainConsBufferHandle = GImport(graph, mTerrainConsBuffer.get());

        RHI::RenderPass& traverseQuadTreePass = graph.AddRenderPass("TraverseQuadTreePass");

        traverseQuadTreePass.Read(terrainConsBufferHandle);
        traverseQuadTreePass.Read(terrainMinHeightHandle);
        traverseQuadTreePass.Read(terrainMaxHeightHandle);
        traverseQuadTreePass.Read(TempNodeListHandle[0], true);
        traverseQuadTreePass.Read(TempNodeListHandle[1], true);

        traverseQuadTreePass.Write(TempNodeListHandle[0], true);
        traverseQuadTreePass.Write(TempNodeListHandle[1], true);
        traverseQuadTreePass.Write(FinalNodeListHandle, true);
        traverseQuadTreePass.Write(NodeDescriptorsHandle, true);

        traverseQuadTreePass.Execute([=](RHI::RenderGraphRegistry* registry, RHI::D3D12CommandContext* context) {
            RHI::D3D12ComputeContext* pContext = context->GetComputeContext();

            pContext->TransitionBarrier(RegGetBufCounter(TempNodeListHandle[0]), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST);
            pContext->TransitionBarrier(RegGetBufCounter(TempNodeListHandle[1]), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST);
            pContext->TransitionBarrier(RegGetBufCounter(FinalNodeListHandle), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST);
            pContext->TransitionBarrier(RegGetBufCounter(NodeDescriptorsHandle), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST);

            pContext->FlushResourceBarriers();

            pContext->ResetCounter(RegGetBufCounter(TempNodeListHandle[0]));
            pContext->ResetCounter(RegGetBufCounter(TempNodeListHandle[1]));
            pContext->ResetCounter(RegGetBufCounter(FinalNodeListHandle));
            pContext->ResetCounter(RegGetBufCounter(NodeDescriptorsHandle));

            int index = 0;

            //****************************************************************************
            pContext->TransitionBarrier(RegGetBuf(TempNodeListHandle[0]), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
            pContext->TransitionBarrier(RegGetBufCounter(TempNodeListHandle[0]), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
            pContext->TransitionBarrier(RegGetBuf(TempNodeListHandle[1]), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
            pContext->TransitionBarrier(RegGetBufCounter(TempNodeListHandle[1]), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
            pContext->TransitionBarrier(RegGetBuf(FinalNodeListHandle), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
            pContext->TransitionBarrier(RegGetBufCounter(FinalNodeListHandle), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
            pContext->TransitionBarrier(RegGetBuf(NodeDescriptorsHandle), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
            pContext->TransitionBarrier(RegGetBufCounter(NodeDescriptorsHandle), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
            pContext->FlushResourceBarriers();

            RHI::RgResourceHandle AppendNodeListHandle = TempNodeListHandle[index];

            RHI::D3D12UnorderedAccessView* AppendNodeListUAV = registry->GetD3D12Buffer(AppendNodeListHandle)->GetDefaultUAV().get();

            pContext->SetRootSignature(pInitQuadTreeSignature.get());
            pContext->SetPipelineState(pInitQuadTreePSO.get());
            pContext->SetConstant(0, 0, AppendNodeListUAV->GetIndex());
            pContext->Dispatch(1, 1, 1);

            //****************************************************************************
            for (int i = MAX_TERRAIN_LOD; i >= 0; i--)
            {
                pContext->TransitionBarrier(RegGetBufCounter(AppendNodeListHandle), D3D12_RESOURCE_STATE_COPY_SOURCE);
                pContext->TransitionBarrier(RegGetBuf(traverseDispatchArgsHandle), D3D12_RESOURCE_STATE_COPY_DEST);
                pContext->CopyBufferRegion(RegGetBuf(traverseDispatchArgsHandle), 0, RegGetBufCounter(AppendNodeListHandle), 0, sizeof(uint32_t));
                pContext->FillBuffer(RegGetBuf(traverseDispatchArgsHandle), sizeof(uint32_t), 1, sizeof(uint32_t) * 2);

                RHI::RgResourceHandle ConsumeNodeListHandle = TempNodeListHandle[index % 2];
                AppendNodeListHandle = TempNodeListHandle[(index + 1) % 2];

                pContext->TransitionBarrier(RegGetBuf(ConsumeNodeListHandle), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
                pContext->TransitionBarrier(RegGetBufCounter(ConsumeNodeListHandle), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
                pContext->TransitionBarrier(RegGetBuf(AppendNodeListHandle), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
                pContext->TransitionBarrier(RegGetBufCounter(AppendNodeListHandle), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
                pContext->TransitionBarrier(RegGetBuf(traverseDispatchArgsHandle), D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
                pContext->FlushResourceBarriers();

                pContext->SetRootSignature(pTraverseQuadTreeSignature.get());
                pContext->SetPipelineState(pTraverseQuadTreePSO.get());

                struct RootIndexBuffer
                {
                    uint32_t consBufferIndex;
                    uint32_t minHeightmapIndex;
                    uint32_t maxHeightmapIndex;
                    uint32_t consumeNodeListBufferIndex;
                    uint32_t appendNodeListBufferIndex;
                    uint32_t appendFinalNodeListBufferIndex;
                    uint32_t nodeDescriptorsBufferIndex;
                    uint32_t PassLOD; //表示TraverseQuadTree kernel执行的LOD级别
                };

                RootIndexBuffer rootIndexBuffer = RootIndexBuffer{ RegGetBufDefCBVIdx(terrainConsBufferHandle),
                                                                   RegGetTexDefSRVIdx(terrainMinHeightHandle),
                                                                   RegGetTexDefSRVIdx(terrainMaxHeightHandle),
                                                                   RegGetBufDefUAVIdx(ConsumeNodeListHandle),
                                                                   RegGetBufDefUAVIdx(AppendNodeListHandle),
                                                                   RegGetBufDefUAVIdx(FinalNodeListHandle),
                                                                   RegGetBufDefUAVIdx(NodeDescriptorsHandle),
                                                                   (uint32_t)i };

                pContext->SetConstantArray(0, sizeof(rootIndexBuffer) / sizeof(uint32_t), &rootIndexBuffer);

                pContext->DispatchIndirect(RegGetBuf(traverseDispatchArgsHandle), 0);

                index += 1;
            }
        });

        //------------------------------------------------------------------------------------

        RHI::RgResourceHandle LodMapHandle = GImport(graph, pLodMap.get());

        RHI::RenderPass& buildLodMapPass = graph.AddRenderPass("BuildLodMapPass");

        buildLodMapPass.Read(terrainConsBufferHandle);
        buildLodMapPass.Read(NodeDescriptorsHandle, true);
        buildLodMapPass.Write(LodMapHandle, true);

        buildLodMapPass.Execute([=](RHI::RenderGraphRegistry* registry, RHI::D3D12CommandContext* context) {
            RHI::D3D12ComputeContext* pContext = context->GetComputeContext();

            pContext->TransitionBarrier(RegGetBuf(NodeDescriptorsHandle), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
            pContext->TransitionBarrier(RegGetTex(LodMapHandle), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
            pContext->FlushResourceBarriers();

            pContext->SetRootSignature(pBuildLodMapSignature.get());
            pContext->SetPipelineState(pBuildLodMapPSO.get());

            struct RootIndexBuffer
            {
                uint32_t consBufferIndex;
                uint32_t nodeDescriptorsBufferIndex;
                uint32_t lodMapIndex;
            };

            RootIndexBuffer rootIndexBuffer = RootIndexBuffer{ RegGetBufDefCBVIdx(terrainConsBufferHandle),
                                                               RegGetBufDefSRVIdx(NodeDescriptorsHandle),
                                                               RegGetTexDefUAVIdx(LodMapHandle) };

            pContext->SetConstantArray(0, sizeof(rootIndexBuffer) / sizeof(uint32_t), &rootIndexBuffer);

            pContext->Dispatch2D(160, 160, 8, 8);
        });

        //------------------------------------------------------------------------------------

        RHI::RgResourceHandle buildPatchArgsHandle = graph.Create<RHI::D3D12Buffer>(buildPatchArgsBufferDesc);
        RHI::RgResourceHandle culledPatchListHandle = GImport(graph, CulledPatchListBuffer.get());

        RHI::RenderPass& buildPatchesPass = graph.AddRenderPass("BuildCamPatchesPass");

        buildPatchesPass.Read(terrainConsBufferHandle);
        buildPatchesPass.Read(terrainMinHeightHandle);
        buildPatchesPass.Read(terrainMaxHeightHandle);
        buildPatchesPass.Read(LodMapHandle);
        buildPatchesPass.Read(FinalNodeListHandle);
        buildPatchesPass.Write(culledPatchListHandle, true);

        buildPatchesPass.Execute([=](RHI::RenderGraphRegistry* registry, RHI::D3D12CommandContext* context) {
            RHI::D3D12ComputeContext* pContext = context->GetComputeContext();

            pContext->TransitionBarrier(RegGetBufCounter(FinalNodeListHandle), D3D12_RESOURCE_STATE_COPY_SOURCE);
            pContext->TransitionBarrier(RegGetBuf(buildPatchArgsHandle), D3D12_RESOURCE_STATE_COPY_DEST);
            pContext->CopyBufferRegion(RegGetBuf(buildPatchArgsHandle), 0, RegGetBufCounter(FinalNodeListHandle), 0, sizeof(uint32_t));
            pContext->FillBuffer(RegGetBuf(buildPatchArgsHandle), sizeof(uint32_t), 1, sizeof(uint32_t) * 2);

            pContext->TransitionBarrier(RegGetBufCounter(culledPatchListHandle), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST);
            pContext->ResetCounter(RegGetBufCounter(culledPatchListHandle));
            pContext->FlushResourceBarriers();

            pContext->TransitionBarrier(RegGetBuf(culledPatchListHandle), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
            pContext->TransitionBarrier(RegGetBufCounter(culledPatchListHandle), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
            pContext->TransitionBarrier(RegGetBuf(buildPatchArgsHandle), D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
            pContext->FlushResourceBarriers();

            pContext->SetRootSignature(pBuildPatchesSignature.get());
            pContext->SetPipelineState(pBuildPatchesPSO.get());

            struct RootIndexBuffer
            {
                uint32_t consBufferIndex;
                uint32_t minHeightmapIndex;
                uint32_t maxHeightmapIndex;
                uint32_t lodMapIndex;
                uint32_t finalNodeListBufferIndex;
                uint32_t culledPatchListBufferIndex;
            };

            RootIndexBuffer rootIndexBuffer = RootIndexBuffer{ RegGetBufDefCBVIdx(terrainConsBufferHandle),
                                                               RegGetTexDefSRVIdx(terrainMinHeightHandle),
                                                               RegGetTexDefSRVIdx(terrainMaxHeightHandle),
                                                               RegGetTexDefSRVIdx(LodMapHandle),
                                                               RegGetBufDefSRVIdx(FinalNodeListHandle),
                                                               RegGetBufDefUAVIdx(culledPatchListHandle) };

            pContext->SetConstantArray(0, sizeof(rootIndexBuffer) / sizeof(uint32_t), &rootIndexBuffer);

            pContext->DispatchIndirect(RegGetBuf(buildPatchArgsHandle), 0);
        });

        //------------------------------------------------------------------------------------
        
        RHI::RgResourceHandle camUploadPatchCmdSigBufferHandle = GImport(graph, camUploadPatchCmdSigBuffer.get());
        RHI::RgResourceHandle camPatchCmdSigBufferHandle = GImport(graph, camPatchCmdSigBuffer.get());

        RHI::RenderPass& genTerrainCmdSigPass = graph.AddRenderPass("GenTerrainCmdSigPass");

        genTerrainCmdSigPass.Read(culledPatchListHandle, true);
        genTerrainCmdSigPass.Read(camUploadPatchCmdSigBufferHandle, true);
        genTerrainCmdSigPass.Write(camPatchCmdSigBufferHandle, true);

        genTerrainCmdSigPass.Execute([=](RHI::RenderGraphRegistry* registry, RHI::D3D12CommandContext* context) {
            RHI::D3D12ComputeContext* pContext = context->GetComputeContext();

            pContext->TransitionBarrier(RegGetBufCounter(culledPatchListHandle), D3D12_RESOURCE_STATE_COPY_SOURCE);
            pContext->TransitionBarrier(RegGetBuf(camUploadPatchCmdSigBufferHandle), D3D12_RESOURCE_STATE_COPY_SOURCE);
            pContext->TransitionBarrier(RegGetBuf(camPatchCmdSigBufferHandle), D3D12_RESOURCE_STATE_COPY_DEST);
            pContext->FlushResourceBarriers();

            pContext->CopyBufferRegion(
                RegGetBuf(camPatchCmdSigBufferHandle), 0, RegGetBuf(camUploadPatchCmdSigBufferHandle), 0, sizeof(HLSL::ToDrawCommandSignatureParams));

            // HLSL::ToDrawCommandSignatureParams
            int instanceCountOffsetInBuffer = sizeof(D3D12_VERTEX_BUFFER_VIEW) + sizeof(D3D12_INDEX_BUFFER_VIEW) + sizeof(uint32_t);

            pContext->CopyBufferRegion(
                RegGetBuf(camPatchCmdSigBufferHandle), instanceCountOffsetInBuffer, RegGetBufCounter(culledPatchListHandle), 0, sizeof(uint32_t));
        });
        
        //------------------------------------------------------------------------------------

        RHI::RgResourceHandle terrainDirConsBufferHandle = GImport(graph, mTerrainDirConsBuffer.get());
        RHI::RgResourceHandle culledDirPatchListHandle = GImport(graph, CulledDirPatchListBuffer.get());

        RHI::RenderPass& buildDirPatchesPass = graph.AddRenderPass("BuildDirPatchesPass");

        buildDirPatchesPass.Read(terrainDirConsBufferHandle);
        buildDirPatchesPass.Read(terrainMinHeightHandle);
        buildDirPatchesPass.Read(terrainMaxHeightHandle);
        buildDirPatchesPass.Read(LodMapHandle);
        buildDirPatchesPass.Read(FinalNodeListHandle);
        buildDirPatchesPass.Write(culledDirPatchListHandle, true);

        buildDirPatchesPass.Execute([=](RHI::RenderGraphRegistry* registry, RHI::D3D12CommandContext* context) {
            RHI::D3D12ComputeContext* pContext = context->GetComputeContext();

            pContext->TransitionBarrier(RegGetBufCounter(FinalNodeListHandle), D3D12_RESOURCE_STATE_COPY_SOURCE);
            pContext->TransitionBarrier(RegGetBuf(buildPatchArgsHandle), D3D12_RESOURCE_STATE_COPY_DEST);
            pContext->CopyBufferRegion(RegGetBuf(buildPatchArgsHandle), 0, RegGetBufCounter(FinalNodeListHandle), 0, sizeof(uint32_t));
            pContext->FillBuffer(RegGetBuf(buildPatchArgsHandle), sizeof(uint32_t), 1, sizeof(uint32_t) * 2);

            pContext->TransitionBarrier(RegGetBufCounter(culledDirPatchListHandle), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST);
            pContext->ResetCounter(RegGetBufCounter(culledDirPatchListHandle));
            pContext->FlushResourceBarriers();

            pContext->TransitionBarrier(RegGetBuf(culledDirPatchListHandle), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
            pContext->TransitionBarrier(RegGetBufCounter(culledDirPatchListHandle), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
            pContext->TransitionBarrier(RegGetBuf(buildPatchArgsHandle), D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
            pContext->FlushResourceBarriers();

            pContext->SetRootSignature(pBuildPatchesSignature.get());
            pContext->SetPipelineState(pBuildPatchesPSO.get());

            struct RootIndexBuffer
            {
                uint32_t consBufferIndex;
                uint32_t minHeightmapIndex;
                uint32_t maxHeightmapIndex;
                uint32_t lodMapIndex;
                uint32_t finalNodeListBufferIndex;
                uint32_t culledPatchListBufferIndex;
            };

            RootIndexBuffer rootIndexBuffer = RootIndexBuffer{ RegGetBufDefCBVIdx(terrainDirConsBufferHandle),
                                                               RegGetTexDefSRVIdx(terrainMinHeightHandle),
                                                               RegGetTexDefSRVIdx(terrainMaxHeightHandle),
                                                               RegGetTexDefSRVIdx(LodMapHandle),
                                                               RegGetBufDefSRVIdx(FinalNodeListHandle),
                                                               RegGetBufDefUAVIdx(culledDirPatchListHandle) };

            pContext->SetConstantArray(0, sizeof(rootIndexBuffer) / sizeof(uint32_t), &rootIndexBuffer);

            pContext->DispatchIndirect(RegGetBuf(buildPatchArgsHandle), 0);
        });
        
        //------------------------------------------------------------------------------------
        
        //RHI::RgResourceHandle camUploadPatchCmdSigBufferHandle = GImport(graph, camUploadPatchCmdSigBuffer.get());
        RHI::RgResourceHandle dirPatchCmdSigBufferHandle = GImport(graph, dirPatchCmdSigBuffer.get());

        RHI::RenderPass& genTerrainDirCmdSigPass = graph.AddRenderPass("GenTerrainDirCmdSigPass");

        genTerrainDirCmdSigPass.Read(culledDirPatchListHandle, true);
        genTerrainDirCmdSigPass.Read(camUploadPatchCmdSigBufferHandle, true);
        genTerrainDirCmdSigPass.Write(dirPatchCmdSigBufferHandle, true);

        genTerrainDirCmdSigPass.Execute([=](RHI::RenderGraphRegistry* registry, RHI::D3D12CommandContext* context) {
            RHI::D3D12ComputeContext* pContext = context->GetComputeContext();

            pContext->TransitionBarrier(RegGetBufCounter(culledDirPatchListHandle), D3D12_RESOURCE_STATE_COPY_SOURCE);
            pContext->TransitionBarrier(RegGetBuf(camUploadPatchCmdSigBufferHandle), D3D12_RESOURCE_STATE_COPY_SOURCE);
            pContext->TransitionBarrier(RegGetBuf(dirPatchCmdSigBufferHandle), D3D12_RESOURCE_STATE_COPY_DEST);
            pContext->FlushResourceBarriers();

            pContext->CopyBufferRegion(
                RegGetBuf(dirPatchCmdSigBufferHandle), 0, RegGetBuf(camUploadPatchCmdSigBufferHandle), 0, sizeof(HLSL::ToDrawCommandSignatureParams));

            // HLSL::ToDrawCommandSignatureParams
            int instanceCountOffsetInBuffer = sizeof(D3D12_VERTEX_BUFFER_VIEW) + sizeof(D3D12_INDEX_BUFFER_VIEW) + sizeof(uint32_t);

            pContext->CopyBufferRegion(
                RegGetBuf(dirPatchCmdSigBufferHandle), instanceCountOffsetInBuffer, RegGetBufCounter(culledDirPatchListHandle), 0, sizeof(uint32_t));
        });
        
        //------------------------------------------------------------------------------------

        passOutput.terrainHeightmapHandle = terrainHeightmapHandle;
        passOutput.terrainNormalmapHandle = terrainNormalmapHandle;
        passOutput.minHeightmapPyramidHandle = terrainMinHeightHandle;
        passOutput.maxHeightmapPyramidHandle = terrainMaxHeightHandle;
        passOutput.terrainRenderDataHandle = terrainRenderDataHandle;
        passOutput.terrainMatPropertyHandle = terrainMatPropertiesHandle;
        passOutput.mainCamVisPatchListHandle = culledPatchListHandle;
        passOutput.mainCamVisCmdSigBufferHandle = camPatchCmdSigBufferHandle;
        passOutput.dirVisPatchListHandle = culledDirPatchListHandle;
        passOutput.dirVisCmdSigBufferHandle = dirPatchCmdSigBufferHandle;

        /*
        bool needClearRenderTarget = initializeRenderTarget(graph, &passOutput);

        InternalTerrainRenderer& internalTerrainRenderer = m_render_scene->m_terrain_renderers[0].internalTerrainRenderer;
        std::shared_ptr<RHI::D3D12Texture> terrainHeightmap = internalTerrainRenderer.ref_terrain.terrain_heightmap;
        std::shared_ptr<RHI::D3D12Texture> terrainNormalmap = internalTerrainRenderer.ref_terrain.terrain_normalmap;

        std::shared_ptr<RHI::D3D12Buffer> uploadTransformBuffer = internalTerrainRenderer.ref_terrain.terrain_clipmap.instance_buffer.clip_transform_buffer;
        std::shared_ptr<RHI::D3D12Buffer> uploadMeshCountBuffer = internalTerrainRenderer.ref_terrain.terrain_clipmap.instance_buffer.clip_mesh_count_buffer;
        std::shared_ptr<RHI::D3D12Buffer> uploadBaseMeshCommandSigBuffer = internalTerrainRenderer.ref_terrain.terrain_clipmap.instance_buffer.clip_base_mesh_command_buffer;

        RHI::RgResourceHandle terrainHeightmapHandle = GImport(graph, terrainHeightmap.get());
        RHI::RgResourceHandle terrainNormalmapHandle = GImport(graph, terrainNormalmap.get());

        RHI::RgResourceHandle terrainMinHeightMapHandle = GImport(graph, terrainMinHeightMap.get());
        RHI::RgResourceHandle terrainMaxHeightMapHandle = GImport(graph, terrainMaxHeightMap.get());

        RHI::RgResourceHandle uploadBaseMeshCommandSigHandle = GImport(graph, uploadBaseMeshCommandSigBuffer.get());
        RHI::RgResourceHandle uploadClipmapTransformHandle   = GImport(graph, uploadTransformBuffer.get());
        RHI::RgResourceHandle uploadMeshCountHandle          = GImport(graph, uploadMeshCountBuffer.get());

        RHI::RgResourceHandle clipmapBaseCommandSighandle = GImport(graph, clipmapBaseCommandSigBuffer.get());
        RHI::RgResourceHandle clipmapTransformHandle      = GImport(graph, clipmapTransformBuffer.get());
        RHI::RgResourceHandle clipmapMeshCountHandle      = GImport(graph, clipmapMeshCountBuffer.get());

        RHI::RgResourceHandle camVisableClipmapHandle = GImport(graph, camVisableClipmapBuffer.get());

        std::vector<RHI::RgResourceHandle> dirVisableClipmapHandles {};
        int dirVisableClipmapBufferCount = dirVisableClipmapBuffers.m_VisableClipmapBuffers.size();
        for (int i = 0; i < dirVisableClipmapBufferCount; i++)
        {
            dirVisableClipmapHandles.push_back(GImport(graph, dirVisableClipmapBuffers.m_VisableClipmapBuffers[i].get()));
        }


        RHI::RgResourceHandle perframeBufferHandle = passInput.perframeBufferHandle;

        //=================================================================================
        //
        //=================================================================================
        if (!isTerrainMinMaxHeightReady)
        {
            isTerrainMinMaxHeightReady = true;

            RHI::RenderPass& genHeightMapMipMapPass = graph.AddRenderPass("GenerateHeightMapMipMapPass");

            genHeightMapMipMapPass.Read(terrainHeightmapHandle, true);
            genHeightMapMipMapPass.Read(terrainMinHeightMapHandle, true);
            genHeightMapMipMapPass.Read(terrainMaxHeightMapHandle, true);
            genHeightMapMipMapPass.Write(terrainMinHeightMapHandle, true);
            genHeightMapMipMapPass.Write(terrainMaxHeightMapHandle, true);

            genHeightMapMipMapPass.Execute([=](RHI::RenderGraphRegistry* registry, RHI::D3D12CommandContext* context) {
                RHI::D3D12ComputeContext* pContext = context->GetComputeContext();

                {
                    pContext->TransitionBarrier(RegGetTex(terrainHeightmapHandle), D3D12_RESOURCE_STATE_COPY_SOURCE);
                    pContext->TransitionBarrier(RegGetTex(terrainMinHeightMapHandle), D3D12_RESOURCE_STATE_COPY_DEST, 0);
                    pContext->TransitionBarrier(RegGetTex(terrainMaxHeightMapHandle), D3D12_RESOURCE_STATE_COPY_DEST, 0);
                    pContext->FlushResourceBarriers();

                    const CD3DX12_TEXTURE_COPY_LOCATION src(terrainHeightmap->GetResource(), 0);

                    const CD3DX12_TEXTURE_COPY_LOCATION dst(terrainMinHeightMap->GetResource(), 0);
                    context->GetGraphicsCommandList()->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);

                    const CD3DX12_TEXTURE_COPY_LOCATION dst_max(terrainMaxHeightMap->GetResource(), 0);
                    context->GetGraphicsCommandList()->CopyTextureRegion(&dst_max, 0, 0, 0, &src, nullptr);

                    context->FlushResourceBarriers();
                }

                //--------------------------------------------------
                // ����MinHeightMap
                //--------------------------------------------------
                {
                    RHI::D3D12Texture* _SrcTexture = RegGetTex(terrainMinHeightMapHandle);
                    generateMipmapForTerrainHeightmap(pContext, _SrcTexture, true);
                }

                //--------------------------------------------------
                // ����MaxHeightMap
                //--------------------------------------------------
                {
                    RHI::D3D12Texture* _SrcTexture = RegGetTex(terrainMaxHeightMapHandle);
                    generateMipmapForTerrainHeightmap(pContext, _SrcTexture, false);
                }

            });
        }
        //=================================================================================
        //
        //=================================================================================
        RHI::RenderPass& terrainCommandSigPreparePass = graph.AddRenderPass("TerrainClipmapPreparePass");

        terrainCommandSigPreparePass.Read(uploadBaseMeshCommandSigHandle, true);
        terrainCommandSigPreparePass.Read(uploadClipmapTransformHandle, true);
        terrainCommandSigPreparePass.Read(uploadMeshCountHandle, true);
        terrainCommandSigPreparePass.Write(clipmapBaseCommandSighandle, true);
        terrainCommandSigPreparePass.Write(clipmapTransformHandle, true);
        terrainCommandSigPreparePass.Write(clipmapMeshCountHandle, true);

        terrainCommandSigPreparePass.Execute(
            [=](RHI::RenderGraphRegistry* registry, RHI::D3D12CommandContext* context) {
                RHI::D3D12ComputeContext* pContext = context->GetComputeContext();

                pContext->TransitionBarrier(RegGetBuf(clipmapBaseCommandSighandle), D3D12_RESOURCE_STATE_COPY_DEST);
                pContext->TransitionBarrier(RegGetBuf(clipmapTransformHandle), D3D12_RESOURCE_STATE_COPY_DEST);
                pContext->TransitionBarrier(RegGetBuf(clipmapMeshCountHandle), D3D12_RESOURCE_STATE_COPY_DEST);
                pContext->FlushResourceBarriers();

                pContext->CopyBuffer(RegGetBuf(clipmapBaseCommandSighandle), RegGetBuf(uploadBaseMeshCommandSigHandle));
                pContext->CopyBuffer(RegGetBuf(clipmapTransformHandle), RegGetBuf(uploadClipmapTransformHandle));
                pContext->CopyBuffer(RegGetBuf(clipmapMeshCountHandle), RegGetBuf(uploadMeshCountHandle));

                pContext->FlushResourceBarriers();
            });

        //=================================================================================
        //
        //=================================================================================
        RHI::RenderPass& terrainMainCullingPass = graph.AddRenderPass("TerrainMainCullingPass");

        terrainMainCullingPass.Read(perframeBufferHandle, true);
        terrainMainCullingPass.Read(clipmapBaseCommandSighandle, true);
        terrainMainCullingPass.Read(clipmapTransformHandle, true);
        terrainMainCullingPass.Read(clipmapMeshCountHandle, true);
        terrainMainCullingPass.Read(terrainMinHeightMapHandle, true);
        terrainMainCullingPass.Read(terrainMaxHeightMapHandle, true);
        terrainMainCullingPass.Write(camVisableClipmapHandle, true);

        terrainMainCullingPass.Execute([=](RHI::RenderGraphRegistry* registry, RHI::D3D12CommandContext* context) {
            RHI::D3D12ComputeContext* pContext = context->GetComputeContext();

            pContext->TransitionBarrier(RegGetBufCounter(camVisableClipmapHandle), D3D12_RESOURCE_STATE_COPY_DEST);
            pContext->FlushResourceBarriers();
            pContext->ResetCounter(RegGetBufCounter(camVisableClipmapHandle));

            pContext->TransitionBarrier(RegGetBuf(perframeBufferHandle), D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
            pContext->TransitionBarrier(RegGetBuf(clipmapMeshCountHandle), D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
            pContext->TransitionBarrier(RegGetBuf(clipmapBaseCommandSighandle), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
            pContext->TransitionBarrier(RegGetBuf(clipmapTransformHandle), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
            pContext->TransitionBarrier(RegGetTex(terrainMinHeightMapHandle), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
            pContext->TransitionBarrier(RegGetTex(terrainMaxHeightMapHandle), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
            pContext->TransitionBarrier(RegGetBuf(camVisableClipmapHandle), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
            pContext->TransitionBarrier(RegGetBufCounter(camVisableClipmapHandle), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
            pContext->FlushResourceBarriers();

            pContext->SetRootSignature(pMainCamVisClipmapSignature.get());
            pContext->SetPipelineState(pMainCamVisClipmapGenPSO.get());

            struct RootIndexBuffer
            {
                uint32_t perFrameBufferIndex;
                uint32_t clipmapMeshCountIndex;
                uint32_t transformBufferIndex;
                uint32_t clipmapBaseCommandSigIndex;
                uint32_t terrainMinHeightmapIndex;
                uint32_t terrainMaxHeightmapIndex;
                uint32_t clipmapVisableBufferIndex;
            };

            RootIndexBuffer rootIndexBuffer = RootIndexBuffer {RegGetBufDefCBVIdx(perframeBufferHandle),
                                                               RegGetBufDefCBVIdx(clipmapMeshCountHandle),
                                                               RegGetBufDefSRVIdx(clipmapTransformHandle),
                                                               RegGetBufDefSRVIdx(clipmapBaseCommandSighandle),
                                                               RegGetTexDefSRVIdx(terrainMinHeightMapHandle),
                                                               RegGetTexDefSRVIdx(terrainMaxHeightMapHandle),
                                                               RegGetBufDefUAVIdx(camVisableClipmapHandle)};

            pContext->SetConstantArray(0, sizeof(RootIndexBuffer) / sizeof(UINT), &rootIndexBuffer);

            pContext->Dispatch1D(256, 8);
        });

        //=================================================================================
        //
        //=================================================================================
        int dirShadowmapVisiableBufferCount = dirVisableClipmapBuffers.m_VisableClipmapBuffers.size();
        if (dirShadowmapVisiableBufferCount != 0)
        {
            RHI::RenderPass& dirLightShadowCullPass = graph.AddRenderPass("DirectionLightTerrainCullingPass");

            dirLightShadowCullPass.Read(perframeBufferHandle, true);
            dirLightShadowCullPass.Read(clipmapBaseCommandSighandle, true);
            dirLightShadowCullPass.Read(clipmapTransformHandle, true);
            dirLightShadowCullPass.Read(clipmapMeshCountHandle, true);
            dirLightShadowCullPass.Read(terrainMinHeightMapHandle, true);
            dirLightShadowCullPass.Read(terrainMaxHeightMapHandle, true);
            for (int i = 0; i < dirShadowmapVisiableBufferCount; i++)
            {
                dirLightShadowCullPass.Write(dirVisableClipmapHandles[i], true);
            }
            dirLightShadowCullPass.Execute([=](RHI::RenderGraphRegistry* registry, RHI::D3D12CommandContext* context) {
                RHI::D3D12ComputeContext* pContext = context->GetComputeContext();

                for (int i = 0; i < dirShadowmapVisiableBufferCount; i++)
                {
                    pContext->TransitionBarrier(RegGetBufCounter(dirVisableClipmapHandles[i]), D3D12_RESOURCE_STATE_COPY_DEST);
                }
                pContext->FlushResourceBarriers();
                for (int i = 0; i < dirShadowmapVisiableBufferCount; i++)
                {
                    pContext->ResetCounter(RegGetBufCounter(dirVisableClipmapHandles[i]));
                }
                pContext->TransitionBarrier(RegGetBuf(perframeBufferHandle), D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
                pContext->TransitionBarrier(RegGetBuf(clipmapMeshCountHandle), D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
                pContext->TransitionBarrier(RegGetBuf(clipmapBaseCommandSighandle), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
                pContext->TransitionBarrier(RegGetBuf(clipmapTransformHandle), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
                pContext->TransitionBarrier(RegGetTex(terrainMinHeightMapHandle), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
                pContext->TransitionBarrier(RegGetTex(terrainMaxHeightMapHandle), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
                for (int i = 0; i < dirShadowmapVisiableBufferCount; i++)
                {
                    pContext->TransitionBarrier(RegGetBuf(dirVisableClipmapHandles[i]), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
                    pContext->TransitionBarrier(RegGetBufCounter(dirVisableClipmapHandles[i]), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
                }
                pContext->FlushResourceBarriers();

                for (uint32_t i = 0; i < dirShadowmapVisiableBufferCount; i++)
                {
                    pContext->SetRootSignature(pDirVisClipmapSignature.get());
                    pContext->SetPipelineState(pDirVisClipmapGenPSO.get());

                    struct RootIndexBuffer
                    {
                        uint32_t cascadeLevel;
                        uint32_t perFrameBufferIndex;
                        uint32_t clipmapMeshCountIndex;
                        uint32_t transformBufferIndex;
                        uint32_t clipmapBaseCommandSigIndex;
                        uint32_t terrainMinHeightmapIndex;
                        uint32_t terrainMaxHeightmapIndex;
                        uint32_t clipmapVisableBufferIndex;
                    };

                    RootIndexBuffer rootIndexBuffer = RootIndexBuffer {i,
                                                                       RegGetBufDefCBVIdx(perframeBufferHandle),
                                                                       RegGetBufDefCBVIdx(clipmapMeshCountHandle),
                                                                       RegGetBufDefSRVIdx(clipmapTransformHandle),
                                                                       RegGetBufDefSRVIdx(clipmapBaseCommandSighandle),
                                                                       RegGetTexDefSRVIdx(terrainMinHeightMapHandle),
                                                                       RegGetTexDefSRVIdx(terrainMaxHeightMapHandle),
                                                                       RegGetBufDefUAVIdx(dirVisableClipmapHandles[i])};

                    pContext->SetConstantArray(0, sizeof(RootIndexBuffer) / sizeof(UINT), &rootIndexBuffer);

                    pContext->Dispatch1D(256, 8);
                }
            });
        }



        /*
        RHI::RenderPass& terrainNodesBuildPass = graph.AddRenderPass("TerrainNodesBuildPass");

        terrainNodesBuildPass.Read(perframeBufferHandle, true); // RHIResourceState::RHI_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER
        terrainNodesBuildPass.Read(terrainMinHeightMapHandle, true);
        terrainNodesBuildPass.Read(terrainMaxHeightMapHandle, true);
        terrainNodesBuildPass.Write(terrainPatchNodeHandle, true); // RHIResourceState::RHI_RESOURCE_STATE_UNORDERED_ACCESS

        terrainNodesBuildPass.Execute([=](RHI::RenderGraphRegistry* registry, RHI::D3D12CommandContext* context) {
            RHI::D3D12ComputeContext* pContext = context->GetComputeContext();

            pContext->TransitionBarrier(RegGetBufCounter(terrainPatchNodeHandle), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST);
            pContext->FlushResourceBarriers();
            pContext->ResetCounter(RegGetBufCounter(terrainPatchNodeHandle));
            pContext->TransitionBarrier(RegGetBufCounter(terrainPatchNodeHandle), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
            pContext->FlushResourceBarriers();

            pContext->TransitionBarrier(RegGetBuf(perframeBufferHandle), D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
            pContext->TransitionBarrier(RegGetBuf(terrainPatchNodeHandle), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
            pContext->TransitionBarrier(RegGetTex(terrainMinHeightMapHandle), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
            pContext->TransitionBarrier(RegGetTex(terrainMaxHeightMapHandle), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
            pContext->FlushResourceBarriers();

            RHI::D3D12Texture* minHeightmap = registry->GetD3D12Texture(terrainMinHeightMapHandle);
            auto minHeightmapDesc = minHeightmap->GetDesc();

            RHI::D3D12ConstantBufferView* perframeCBVHandle = registry->GetD3D12Buffer(perframeBufferHandle)->GetDefaultCBV().get();
            RHI::D3D12UnorderedAccessView* terrainNodeHandleUAV = registry->GetD3D12Buffer(terrainPatchNodeHandle)->GetDefaultUAV().get();
            RHI::D3D12ShaderResourceView* terrainMinHeightmapSRV = registry->GetD3D12Texture(terrainMinHeightMapHandle)->GetDefaultSRV().get();
            RHI::D3D12ShaderResourceView* terrainMaxHeightmapSRV = registry->GetD3D12Texture(terrainMaxHeightMapHandle)->GetDefaultSRV().get();

            pContext->SetRootSignature(pIndirecTerrainPatchNodesGenSignature.get());
            pContext->SetPipelineState(pIndirecTerrainPatchNodesGenPSO.get());
            pContext->SetConstant(0, 0, perframeCBVHandle->GetIndex());
            pContext->SetConstant(0, 1, terrainNodeHandleUAV->GetIndex());
            pContext->SetConstant(0, 2, terrainMinHeightmapSRV->GetIndex());
            pContext->SetConstant(0, 3, terrainMaxHeightmapSRV->GetIndex());

            pContext->Dispatch2D(minHeightmapDesc.Width, minHeightmapDesc.Height, 8, 8);
        });

        //=================================================================================
        //
        //=================================================================================

        RHI::RenderPass& terrainMainCullingPass = graph.AddRenderPass("TerrainMainCullingPass");

        terrainMainCullingPass.Read(perframeBufferHandle, true);
        terrainMainCullingPass.Read(terrainPatchNodeHandle, true);
        terrainMainCullingPass.Write(mainCamVisiableIndexHandle, true);

        terrainMainCullingPass.Execute([=](RHI::RenderGraphRegistry* registry, RHI::D3D12CommandContext* context) {
            RHI::D3D12ComputeContext* pContext = context->GetComputeContext();

            pContext->TransitionBarrier(RegGetBufCounter(mainCamVisiableIndexHandle), D3D12_RESOURCE_STATE_COPY_DEST);
            pContext->FlushResourceBarriers();
            pContext->ResetCounter(RegGetBufCounter(mainCamVisiableIndexHandle));

            pContext->TransitionBarrier(RegGetBuf(perframeBufferHandle), D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
            pContext->TransitionBarrier(RegGetBuf(terrainPatchNodeHandle), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
            pContext->TransitionBarrier(RegGetBufCounter(terrainPatchNodeHandle), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
            pContext->TransitionBarrier(RegGetBuf(mainCamVisiableIndexHandle), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
            pContext->TransitionBarrier(RegGetBufCounter(mainCamVisiableIndexHandle), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
            pContext->FlushResourceBarriers();

            pContext->SetRootSignature(pPatchNodeVisToMainCamIndexGenSignature.get());
            pContext->SetPipelineState(pPatchNodeVisToMainCamIndexGenPSO.get());

            struct RootIndexBuffer
            {
                UINT meshPerFrameBufferIndex;
                UINT terrainPatchNodeIndex;
                UINT terrainPatchNodeCountIndex;
                UINT terrainVisNodeIdxIndex;
            };

            RootIndexBuffer rootIndexBuffer =
                RootIndexBuffer {RegGetBufDefCBVIdx(perframeBufferHandle),
                                 RegGetBufDefSRVIdx(terrainPatchNodeHandle),
                                 RegGetBufCounterSRVIdx(terrainPatchNodeHandle),
                                 RegGetBufDefUAVIdx(mainCamVisiableIndexHandle)};

            pContext->SetConstantArray(0, sizeof(RootIndexBuffer) / sizeof(UINT), &rootIndexBuffer);

            pContext->Dispatch1D(4096, 128);
        });

        //=================================================================================
        //
        //=================================================================================
        int dirShadowmapVisiableBufferCount = dirShadowmapCommandBuffers.m_PatchNodeVisiableIndexBuffers.size();
        if (dirShadowmapVisiableBufferCount != 0)
        {
            RHI::RenderPass& dirLightShadowCullPass = graph.AddRenderPass("DirectionLightTerrainPatchNodeCullPass");

            dirLightShadowCullPass.Read(perframeBufferHandle, true);
            dirLightShadowCullPass.Read(terrainPatchNodeHandle, true);
            for (int i = 0; i < dirShadowmapVisiableBufferCount; i++)
            {
                dirLightShadowCullPass.Write(dirShadowPatchNodeVisiableIndexHandles[i], true);
            }
            dirLightShadowCullPass.Execute([=](RHI::RenderGraphRegistry* registry, RHI::D3D12CommandContext* context) {
                RHI::D3D12ComputeContext* pContext = context->GetComputeContext();

                for (int i = 0; i < dirShadowmapVisiableBufferCount; i++)
                {
                    pContext->TransitionBarrier(RegGetBufCounter(dirShadowPatchNodeVisiableIndexHandles[i]), D3D12_RESOURCE_STATE_COPY_DEST);
                }
                pContext->FlushResourceBarriers();
                for (int i = 0; i < dirShadowmapVisiableBufferCount; i++)
                {
                    pContext->ResetCounter(RegGetBufCounter(dirShadowPatchNodeVisiableIndexHandles[i]));
                }
                for (int i = 0; i < dirShadowmapVisiableBufferCount; i++)
                {
                    pContext->TransitionBarrier(RegGetBuf(dirShadowPatchNodeVisiableIndexHandles[i]), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
                    pContext->TransitionBarrier(RegGetBufCounter(dirShadowPatchNodeVisiableIndexHandles[i]), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
                }
                pContext->TransitionBarrier(RegGetBuf(perframeBufferHandle), D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
                pContext->TransitionBarrier(RegGetBuf(terrainPatchNodeHandle), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
                pContext->TransitionBarrier(RegGetBufCounter(terrainPatchNodeHandle), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
                pContext->FlushResourceBarriers();

                for (int i = 0; i < dirShadowmapVisiableBufferCount; i++)
                {
                    pContext->SetRootSignature(pPatchNodeVisToDirCascadeIndexGenSignature.get());
                    pContext->SetPipelineState(pPatchNodeVisToDirCascadeIndexGenPSO.get());

                    struct RootIndexBuffer
                    {
                        UINT cascadeLevel;
                        UINT meshPerFrameBufferIndex;
                        UINT terrainPatchNodeIndex;
                        UINT terrainPatchNodeCountIndex;
                        UINT terrainVisNodeIdxIndex;
                    };

                    RootIndexBuffer rootIndexBuffer =
                        RootIndexBuffer {(UINT)i,
                                         RegGetBufDefCBVIdx(perframeBufferHandle),
                                         RegGetBufDefSRVIdx(terrainPatchNodeHandle),
                                         RegGetBufCounterSRVIdx(terrainPatchNodeHandle),
                                         RegGetBufDefUAVIdx(dirShadowPatchNodeVisiableIndexHandles[i])};

                    pContext->SetConstantArray(0, sizeof(RootIndexBuffer) / sizeof(UINT), &rootIndexBuffer);

                    pContext->Dispatch1D(4096, 128);
                }
            });
        }

        //=================================================================================
        //
        //=================================================================================

        if (dirShadowmapVisiableBufferCount != 0)
        {
            RHI::RenderPass& dirShadowCullingPass = graph.AddRenderPass("TerrainCommandSignatureForDirShadowPreparePass");

            dirShadowCullingPass.Read(mainCommandSigUploadBufferHandle, true);
            for (int i = 0; i < dirShadowmapVisiableBufferCount; i++)
            {
                dirShadowCullingPass.Read(dirShadowPatchNodeVisiableIndexHandles[i], true);
                dirShadowCullingPass.Write(dirShadowCommandSigBufferHandles[i], true);
            }

            dirShadowCullingPass.Execute([=](RHI::RenderGraphRegistry* registry, RHI::D3D12CommandContext* context) {
                RHI::D3D12ComputeContext* pContext = context->GetComputeContext();

                pContext->TransitionBarrier(RegGetBuf(mainCommandSigUploadBufferHandle), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ);
                for (int i = 0; i < dirShadowmapVisiableBufferCount; i++)
                {
                    pContext->TransitionBarrier(RegGetBufCounter(dirShadowPatchNodeVisiableIndexHandles[i]), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_SOURCE);
                    pContext->TransitionBarrier(RegGetBuf(dirShadowCommandSigBufferHandles[i]), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST);
                }
                pContext->FlushResourceBarriers();

                for (int i = 0; i < dirShadowmapVisiableBufferCount; i++)
                {
                    pContext->CopyBufferRegion(RegGetBuf(dirShadowCommandSigBufferHandles[i]),
                                               0,
                                               RegGetBuf(mainCommandSigUploadBufferHandle),
                                               0,
                                               sizeof(MoYu::TerrainCommandSignatureParams));
                    pContext->CopyBufferRegion(RegGetBuf(dirShadowCommandSigBufferHandles[i]),
                                               terrainInstanceCountOffset,
                                               RegGetBufCounter(dirShadowPatchNodeVisiableIndexHandles[i]),
                                               0,
                                               sizeof(int));
                    pContext->TransitionBarrier(RegGetBuf(dirShadowCommandSigBufferHandles[i]),
                                                D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
                }

                pContext->FlushResourceBarriers();

            });
        }

        //=================================================================================
        //
        //=================================================================================

        passOutput.terrainHeightmapHandle    = terrainHeightmapHandle;
        passOutput.terrainNormalmapHandle    = terrainNormalmapHandle;
        passOutput.maxHeightmapPyramidHandle = terrainMaxHeightMapHandle;
        passOutput.minHeightmapPyramidHandle = terrainMinHeightMapHandle;

        passOutput.transformBufferHandle     = clipmapTransformHandle;

        passOutput.mainCamVisCommandSigHandle = camVisableClipmapHandle;
        passOutput.dirVisCommandSigHandles.assign(dirVisableClipmapHandles.begin(), dirVisableClipmapHandles.end());
        */
    }

    void IndirectTerrainCullPass::destroy()
    {



    }

    bool IndirectTerrainCullPass::initializeRenderTarget(RHI::RenderGraph& graph, TerrainCullOutput* drawPassOutput)
    {
        return true;
    }

    void IndirectTerrainCullPass::generateMipmapForTerrainHeightmap(RHI::D3D12ComputeContext* context, RHI::D3D12Texture* _SrcTexture, bool genMin)
    {
        int numSubresource = _SrcTexture->GetNumSubresources();
        int iterCount = glm::ceil(numSubresource / 4.0f);
        for (int i = 0; i < iterCount; i++)
        {
            generateMipmapForTerrainHeightmap(context, _SrcTexture, 4 * i, genMin);
        }
    }

    void IndirectTerrainCullPass::generateMipmapForTerrainHeightmap(RHI::D3D12ComputeContext* pContext, RHI::D3D12Texture* _SrcTexture, int srcIndex, bool genMin)
    {
        int numSubresource = _SrcTexture->GetNumSubresources();
        int outMipCount = glm::max(0, glm::min(numSubresource - 1 - srcIndex, 4));

        pContext->TransitionBarrier(_SrcTexture, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE, srcIndex);
        for (int i = 1; i < outMipCount; i++)
        {
            pContext->TransitionBarrier(_SrcTexture, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, i + srcIndex);
        }
        pContext->FlushResourceBarriers();

        auto _SrcDesc = _SrcTexture->GetDesc();
        auto _Mip0SRVDesc = RHI::D3D12ShaderResourceView::GetDesc(_SrcTexture, false, srcIndex, 1);
        glm::uint _SrcIndex = _SrcTexture->CreateSRV(_Mip0SRVDesc)->GetIndex();

        int _width = _SrcDesc.Width >> (srcIndex + 1);
        int _height = _SrcDesc.Height >> (srcIndex + 1);

        glm::float2 _Mip1TexelSize = { 1.0f / _width, 1.0f / _height };
        glm::uint   _SrcMipLevel = srcIndex;
        glm::uint   _NumMipLevels = outMipCount;

        MipGenInBuffer maxMipGenBuffer = {};

        maxMipGenBuffer.SrcMipLevel = _SrcMipLevel;
        maxMipGenBuffer.NumMipLevels = _NumMipLevels;
        maxMipGenBuffer.TexelSize = _Mip1TexelSize;
        maxMipGenBuffer.SrcIndex = _SrcIndex;

        for (int i = 0; i < outMipCount; i++)
        {
            auto _Mip1UAVDesc = RHI::D3D12UnorderedAccessView::GetDesc(_SrcTexture, 0, srcIndex + i + 1);
            glm::uint _OutMip1Index = _SrcTexture->CreateUAV(_Mip1UAVDesc)->GetIndex();
            *(&maxMipGenBuffer.OutMip1Index + i) = _OutMip1Index;
        }

        pContext->SetRootSignature(RootSignatures::pGenerateMipsLinearSignature.get());
        if (genMin)
        {
            pContext->SetPipelineState(PipelineStates::pGenerateMinMipsLinearPSO.get());
        }
        else
        {
            pContext->SetPipelineState(PipelineStates::pGenerateMaxMipsLinearPSO.get());
        }

        pContext->SetConstantArray(0, sizeof(MipGenInBuffer) / sizeof(int), &maxMipGenBuffer);

        pContext->Dispatch2D(_width, _height, 8, 8);
    }

}


