#include "runtime/function/render/renderer/indirect_cull_pass.h"
#include "runtime/resource/config_manager/config_manager.h"
#include "runtime/function/render/window_system.h"
#include "runtime/function/render/rhi/rhi_core.h"
#include "runtime/core/math/moyu_math2.h"
#include "runtime/function/render/rhi/d3d12/d3d12_graphicsCommon.h"
#include "runtime/function/render/renderer/pass_helper.h"

#include "fmt/core.h"
#include <cassert>

namespace MoYu
{
#define CreateCullingBuffer(numElement, elementSize, bufferName) \
    RHI::D3D12Buffer::Create(m_Device->GetLinkedDevice(), \
                             RHI::RHIBufferRandomReadWrite, \
                             numElement, \
                             elementSize, \
                             bufferName, \
                             RHI::RHIBufferModeImmutable, \
                             D3D12_RESOURCE_STATE_GENERIC_READ)

#define CreateUploadBuffer(numElement, elementSize, bufferName) \
    RHI::D3D12Buffer::Create(m_Device->GetLinkedDevice(), \
                             RHI::RHIBufferTargetNone, \
                             numElement, \
                             elementSize, \
                             bufferName, \
                             RHI::RHIBufferModeDynamic, \
                             D3D12_RESOURCE_STATE_GENERIC_READ)

#define CreateArgBufferDesc(name, numElements) \
    RHI::RgBufferDesc(name) \
        .SetSize(numElements, sizeof(D3D12_DISPATCH_ARGUMENTS)) \
        .SetRHIBufferMode(RHI::RHIBufferMode::RHIBufferModeImmutable) \
        .SetRHIBufferTarget(RHI::RHIBufferTarget::RHIBufferTargetIndirectArgs | RHI::RHIBufferTarget::RHIBufferRandomReadWrite | RHI::RHIBufferTarget::RHIBufferTargetRaw)

#define CreateIndexBuffer(name) \
    RHI::D3D12Buffer::Create(m_Device->GetLinkedDevice(), \
                             RHI::RHIBufferRandomReadWrite | RHI::RHIBufferTargetStructured | RHI::RHIBufferTargetCounter, \
                             HLSL::MeshLimit, \
                             sizeof(HLSL::BitonicSortCommandSigParams), \
                             name)

#define CreateSortCommandBuffer(name) \
    RHI::D3D12Buffer::Create(m_Device->GetLinkedDevice(), \
                             RHI::RHIBufferRandomReadWrite | RHI::RHIBufferTargetStructured | RHI::RHIBufferTargetCounter, \
                             HLSL::MeshLimit, \
                             sizeof(HLSL::CommandSignatureParams), \
                             name)

    void IndirectCullPass::initialize(const IndirectCullInitInfo& init_info)
    {
        albedoDesc = init_info.albedoTexDesc;
        depthDesc  = init_info.depthTexDesc;

        // create default buffer
        pFrameUniformBuffer = CreateCullingBuffer(1, MoYu::AlignUp(sizeof(HLSL::FrameUniforms), 256), L"FrameUniforms");
        pRenderDataPerDrawBuffer = CreateCullingBuffer(HLSL::MaterialLimit, sizeof(HLSL::RenderDataPerDraw), L"RenderDataPerDraw");
        pPropertiesPerMaterialBuffer = CreateCullingBuffer(HLSL::MaterialLimit, sizeof(HLSL::PropertiesPerMaterial), L"PropertiesPerMaterial");
        
        // create upload buffer
        pUploadFrameUniformBuffer = CreateUploadBuffer(1, MoYu::AlignUp(sizeof(HLSL::FrameUniforms), 256), L"UploadFrameUniforms");
        pUploadRenderDataPerDrawBuffer = CreateUploadBuffer(HLSL::MaterialLimit, sizeof(HLSL::RenderDataPerDraw), L"UploadRenderDataPerDraw");
        pUploadPropertiesPerMaterialBuffer = CreateUploadBuffer(HLSL::MaterialLimit, sizeof(HLSL::PropertiesPerMaterial), L"UploadPropertiesPerMaterial");
        
        sortDispatchArgsBufferDesc = CreateArgBufferDesc("SortDispatchArgs", 22 * 23 / 2);
        grabDispatchArgsBufferDesc = CreateArgBufferDesc("GrabDispatchArgs", 22 * 23 / 2);

        // buffer for opaque draw
        commandBufferForOpaqueDraw.p_IndirectIndexCommandBuffer = CreateIndexBuffer(L"OpaqueIndexBuffer");
        commandBufferForOpaqueDraw.p_IndirectSortCommandBuffer  = CreateSortCommandBuffer(L"OpaqueBuffer");

        // buffer for transparent draw
        commandBufferForTransparentDraw.p_IndirectIndexCommandBuffer = CreateIndexBuffer(L"TransparentIndexBuffer");
        commandBufferForTransparentDraw.p_IndirectSortCommandBuffer  = CreateSortCommandBuffer(L"TransparentBuffer");
    }

    void IndirectCullPass::inflatePerframeBuffer(std::shared_ptr<RenderResource> render_resource)
    {
        memcpy(pUploadFrameUniformBuffer->GetCpuVirtualAddress<HLSL::FrameUniforms>(), &render_resource->m_FrameUniforms, sizeof(HLSL::FrameUniforms));
    }

    void IndirectCullPass::prepareMeshData(std::shared_ptr<RenderResource> render_resource)
    {
        std::vector<CachedMeshRenderer>& _mesh_renderers = m_render_scene->m_mesh_renderers;

        HLSL::PropertiesPerMaterial* pPropertiesPerMaterial = pUploadPropertiesPerMaterialBuffer->GetCpuVirtualAddress<HLSL::PropertiesPerMaterial>();

        HLSL::RenderDataPerDraw* pUploadRenderDataPerDraw = pUploadRenderDataPerDrawBuffer->GetCpuVirtualAddress<HLSL::RenderDataPerDraw>();
        
        uint32_t numMeshes = _mesh_renderers.size();
        ASSERT(numMeshes < HLSL::MeshLimit);
        for (size_t i = 0; i < numMeshes; i++)
        {
            InternalMeshRenderer& temp_mesh_renderer = _mesh_renderers[i].internalMeshRenderer;

            InternalMesh& temp_ref_mesh = temp_mesh_renderer.ref_mesh;
            InternalMaterial& temp_ref_material = temp_mesh_renderer.ref_material;

            //------------------------------------------------------

            D3D12_DRAW_INDEXED_ARGUMENTS curDrawIndexedArguments = {};
            curDrawIndexedArguments.IndexCountPerInstance        = temp_ref_mesh.index_buffer.index_count; // temp_node.ref_mesh->mesh_index_count;
            curDrawIndexedArguments.InstanceCount                = 1;
            curDrawIndexedArguments.StartIndexLocation           = 0;
            curDrawIndexedArguments.BaseVertexLocation           = 0;
            curDrawIndexedArguments.StartInstanceLocation        = 0;

            glm::float3 boundingBoxCenter = temp_ref_mesh.axis_aligned_box.getCenter(); // temp_node.bounding_box.center;
            glm::float3 boundingBoxExtents = temp_ref_mesh.axis_aligned_box.getHalfExtent();//temp_node.bounding_box.extent;

            D3D12_VERTEX_BUFFER_VIEW curVertexBufferView = temp_ref_mesh.vertex_buffer.vertex_buffer->GetVertexBufferView();
            D3D12_INDEX_BUFFER_VIEW curIndexBufferView = temp_ref_mesh.index_buffer.index_buffer->GetIndexBufferView();

            uint32_t pPropertiesBufferAddress = pPropertiesPerMaterialBuffer->GetDefaultSRV()->GetIndex();

            HLSL::RenderDataPerDraw curRenderDataPerDraw = {};
            memset(&curRenderDataPerDraw, 0, sizeof(HLSL::RenderDataPerDraw));
            curRenderDataPerDraw.objectToWorldMatrix = temp_mesh_renderer.model_matrix; // temp_node.model_matrix;
            curRenderDataPerDraw.worldToObjectMatrix = temp_mesh_renderer.model_matrix_inverse;//temp_node.model_matrix_inverse;
            curRenderDataPerDraw.prevObjectToWorldMatrix = temp_mesh_renderer.prev_model_matrix;
            curRenderDataPerDraw.prevWorldToObjectMatrix = temp_mesh_renderer.prev_model_matrix_inverse;
            memcpy(&curRenderDataPerDraw.vertexBufferView, &curVertexBufferView, sizeof(D3D12_VERTEX_BUFFER_VIEW));//temp_node.ref_mesh->p_mesh_vertex_buffer->GetVertexBufferView();
            memcpy(&curRenderDataPerDraw.indexBufferView, &curIndexBufferView, sizeof(D3D12_INDEX_BUFFER_VIEW));//temp_node.ref_mesh->p_mesh_vertex_buffer->GetIndexBufferView();
            memcpy(&curRenderDataPerDraw.drawIndexedArguments0, &curDrawIndexedArguments, sizeof(glm::float4));
            memcpy(&curRenderDataPerDraw.drawIndexedArguments1, ((char*)&curDrawIndexedArguments + 4), sizeof(float));
            memcpy(((char*)&curRenderDataPerDraw.drawIndexedArguments1 + 1), &pPropertiesBufferAddress, sizeof(UINT32));
            memcpy(((char*)&curRenderDataPerDraw.drawIndexedArguments1 + 2), &i, sizeof(UINT32));
            curRenderDataPerDraw.boundingBoxCenter = glm::float4(boundingBoxCenter, 0);
            curRenderDataPerDraw.boundingBoxExtents = glm::float4(boundingBoxExtents, 0);

            pUploadRenderDataPerDraw[i] = curRenderDataPerDraw;

            //------------------------------------------------------

            InternalStandardLightMaterial& m_InteralMat = temp_ref_material.m_intenral_light_mat;

            HLSL::PropertiesPerMaterial curPropertiesPerMaterial = {};

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
            curPropertiesPerMaterial._SpecularColorMapIndex = m_InteralMat._SpecularColorMap->GetDefaultSRV()->GetIndex();
            curPropertiesPerMaterial._EmissiveColorMapIndex = m_InteralMat._EmissiveColorMap->GetDefaultSRV()->GetIndex();
            curPropertiesPerMaterial._TransmittanceColorMapIndex = m_InteralMat._TransmittanceColorMap->GetDefaultSRV()->GetIndex();

            curPropertiesPerMaterial._BaseColor = m_InteralMat._BaseColor;
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
            curPropertiesPerMaterial._SpecularColor = m_InteralMat._SpecularColor;
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
            curPropertiesPerMaterial._EnableBlendModePreserveSpecularLighting = m_InteralMat._EnableBlendModePreserveSpecularLighting;
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

            pPropertiesPerMaterial[i] = curPropertiesPerMaterial;
        }

        prepareBuffer();
        prepareRenderTexture();
    }

    void IndirectCullPass::prepareRenderTexture()
    {
        
    }

    void IndirectCullPass::prepareBuffer()
    {
        if (m_render_scene->m_directional_light.m_identifier != UndefCommonIdentifier)
        {
            if (dirShadowmapCommandBuffers.m_identifier != m_render_scene->m_directional_light.m_identifier)
            {
                dirShadowmapCommandBuffers.Reset();
            }

            if (dirShadowmapCommandBuffers.m_DrawCallCommandBuffer.size() != m_render_scene->m_directional_light.m_cascade)
            {
                dirShadowmapCommandBuffers.m_identifier = m_render_scene->m_directional_light.m_identifier;

                for (size_t i = 0; i < m_render_scene->m_directional_light.m_cascade; i++)
                {
                    std::wstring _name = fmt::format(L"DirectionIndirectSortCommandBuffer_Cascade_{}", i);
                    
                    DrawCallCommandBuffer _CommandBuffer = {};
                    _CommandBuffer.p_IndirectIndexCommandBuffer = CreateIndexBuffer(_name);
                    _CommandBuffer.p_IndirectSortCommandBuffer  = CreateSortCommandBuffer(_name);

                    dirShadowmapCommandBuffers.m_DrawCallCommandBuffer.push_back(_CommandBuffer);
                }
            }
        }
        else
        {
            dirShadowmapCommandBuffers.Reset();
        }

        {
            std::vector<SpotShadowmapCommandBuffer> _SpotLightToGPU = {};

            for (size_t i = 0; i < m_render_scene->m_spot_light_list.size(); i++)
            {
                InternalSpotLight& _internalSpotLight = m_render_scene->m_spot_light_list[i];
                if (_internalSpotLight.m_shadowmap)
                {
                    std::wstring _name = fmt::format(L"SpotIndirectSortCommandBuffer_{}", i);

                    bool isFindInOldCommandBuffer = false;
                    for (size_t j = 0; j < spotShadowmapCommandBuffer.size(); j++)
                    {
                        if (_internalSpotLight.m_identifier == spotShadowmapCommandBuffer[j].m_identifier)
                        {
                            SpotShadowmapCommandBuffer _SpotShadowCommandBuffer = spotShadowmapCommandBuffer[j];
                            _SpotShadowCommandBuffer.m_lightIndex = i;
                            _SpotShadowCommandBuffer.m_DrawCallCommandBuffer.p_IndirectIndexCommandBuffer->SetResourceName(_name);
                            _SpotShadowCommandBuffer.m_DrawCallCommandBuffer.p_IndirectSortCommandBuffer->SetResourceName(_name);
                            _SpotLightToGPU.push_back(_SpotShadowCommandBuffer);
                            spotShadowmapCommandBuffer.erase(spotShadowmapCommandBuffer.begin() + j);
                            isFindInOldCommandBuffer = true;
                            break;
                        }
                    }
                    if (!isFindInOldCommandBuffer)
                    {
                        SpotShadowmapCommandBuffer _SpotShadowCommandBuffer = {};
                        _SpotShadowCommandBuffer.m_lightIndex = i;
                        _SpotShadowCommandBuffer.m_identifier = _internalSpotLight.m_identifier;
                        _SpotShadowCommandBuffer.m_DrawCallCommandBuffer.p_IndirectIndexCommandBuffer = CreateIndexBuffer(_name);
                        _SpotShadowCommandBuffer.m_DrawCallCommandBuffer.p_IndirectSortCommandBuffer = CreateSortCommandBuffer(_name);
                        _SpotLightToGPU.push_back(_SpotShadowCommandBuffer);
                    }
                }
            }
            spotShadowmapCommandBuffer.clear();
            spotShadowmapCommandBuffer = _SpotLightToGPU;
        }
    }

    void IndirectCullPass::bitonicSort(RHI::D3D12ComputeContext* context,
                                       RHI::D3D12Buffer*         keyIndexList,
                                       RHI::D3D12Buffer*         countBuffer,
                                       RHI::D3D12Buffer*         sortDispatchArgBuffer, // pSortDispatchArgs
                                       bool                      isPartiallyPreSorted,
                                       bool                      sortAscending)
    {
        const uint32_t ElementSizeBytes      = keyIndexList->GetStride();
        const uint32_t MaxNumElements        = 1024; //keyIndexList->GetSizeInBytes() / ElementSizeBytes;
        const uint32_t AlignedMaxNumElements = MoYu::AlignPowerOfTwo(MaxNumElements);
        const uint32_t MaxIterations         = MoYu::Log2(std::max(2048u, AlignedMaxNumElements)) - 10;

        ASSERT(ElementSizeBytes == 4 || ElementSizeBytes == 8); // , "Invalid key-index list for bitonic sort"

        context->TransitionBarrier(countBuffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        context->TransitionBarrier(sortDispatchArgBuffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        context->FlushResourceBarriers();

        context->SetRootSignature(RootSignatures::pBitonicSortRootSignature.get());
        // Generate execute indirect arguments
        context->SetPipelineState(PipelineStates::pBitonicIndirectArgsPSO.get());

        // This controls two things.  It is a key that will sort to the end, and it is a mask used to
        // determine whether the current group should sort ascending or descending.
        //context.SetConstants(3, counterOffset, sortAscending ? 0xffffffff : 0);
        context->SetConstants(0, MaxIterations);
        context->SetDescriptorTable(1, countBuffer->GetDefaultSRV()->GetGpuHandle());
        context->SetDescriptorTable(2, sortDispatchArgBuffer->GetDefaultUAV()->GetGpuHandle());
        context->SetConstants(3, 0, sortAscending ? 0x7F7FFFFF : 0);
        context->Dispatch(1, 1, 1);

        // Pre-Sort the buffer up to k = 2048.  This also pads the list with invalid indices
        // that will drift to the end of the sorted list.
        context->TransitionBarrier(sortDispatchArgBuffer, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
        context->TransitionBarrier(keyIndexList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        context->InsertUAVBarrier(keyIndexList);
        context->FlushResourceBarriers();

        //context->SetComputeRootUnorderedAccessView(2, keyIndexList->GetGpuVirtualAddress());
        context->SetDescriptorTable(2, keyIndexList->GetDefaultRawUAV()->GetGpuHandle());

        if (!isPartiallyPreSorted)
        {
            context->FlushResourceBarriers();
            context->SetPipelineState(ElementSizeBytes == 4 ? PipelineStates::pBitonic32PreSortPSO.get() :
                                                              PipelineStates::pBitonic64PreSortPSO.get());

            context->DispatchIndirect(sortDispatchArgBuffer, 0);
            context->InsertUAVBarrier(keyIndexList);
        }

        uint32_t IndirectArgsOffset = 12;

        // We have already pre-sorted up through k = 2048 when first writing our list, so
        // we continue sorting with k = 4096.  For unnecessarily large values of k, these
        // indirect dispatches will be skipped over with thread counts of 0.

        for (uint32_t k = 4096; k <= AlignedMaxNumElements; k *= 2)
        {
            context->SetPipelineState(ElementSizeBytes == 4 ? PipelineStates::pBitonic32OuterSortPSO.get() :
                                                              PipelineStates::pBitonic64OuterSortPSO.get());

            for (uint32_t j = k / 2; j >= 2048; j /= 2)
            {
                context->SetConstant(0, j, k);
                context->DispatchIndirect(sortDispatchArgBuffer, IndirectArgsOffset);
                context->InsertUAVBarrier(keyIndexList);
                IndirectArgsOffset += 12;
            }

            context->SetPipelineState(ElementSizeBytes == 4 ? PipelineStates::pBitonic32InnerSortPSO.get() :
                                                             PipelineStates::pBitonic64InnerSortPSO.get());
            context->DispatchIndirect(sortDispatchArgBuffer, IndirectArgsOffset);
            context->InsertUAVBarrier(keyIndexList);
            IndirectArgsOffset += 12;
        }
    }

    void IndirectCullPass::grabObject(RHI::D3D12ComputeContext* context,
                                      RHI::D3D12Buffer*         renderDataPerDraw,
                                      RHI::D3D12Buffer*         indirectIndexBuffer,
                                      RHI::D3D12Buffer*         indirectSortBuffer,
                                      RHI::D3D12Buffer*         grabDispatchArgBuffer) // pSortDispatchArgs

    {
        {
            context->TransitionBarrier(indirectIndexBuffer->GetCounterBuffer().get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
            context->TransitionBarrier(indirectSortBuffer->GetCounterBuffer().get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
            context->TransitionBarrier(grabDispatchArgBuffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
            //context->InsertUAVBarrier(grabDispatchArgBuffer);
            context->FlushResourceBarriers();

            context->SetPipelineState(PipelineStates::pIndirectCullArgs.get());
            context->SetRootSignature(RootSignatures::pIndirectCullArgs.get());

            context->SetBufferSRV(0, indirectIndexBuffer->GetCounterBuffer().get());
            context->SetBufferUAV(1, grabDispatchArgBuffer);

            context->Dispatch(1, 1, 1);
        }
        {
            context->TransitionBarrier(renderDataPerDraw, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
            context->TransitionBarrier(indirectIndexBuffer->GetCounterBuffer().get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
            context->TransitionBarrier(indirectIndexBuffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
            context->TransitionBarrier(grabDispatchArgBuffer, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
            context->TransitionBarrier(indirectSortBuffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
            //context->InsertUAVBarrier(indirectSortBuffer);
            context->FlushResourceBarriers();

            context->SetPipelineState(PipelineStates::pIndirectCullGrab.get());
            context->SetRootSignature(RootSignatures::pIndirectCullGrab.get());

            context->SetBufferSRV(0, renderDataPerDraw);
            context->SetBufferSRV(1, indirectIndexBuffer->GetCounterBuffer().get());
            context->SetBufferSRV(2, indirectIndexBuffer);
            context->SetDescriptorTable(3, indirectSortBuffer->GetDefaultUAV()->GetGpuHandle());

            context->DispatchIndirect(grabDispatchArgBuffer, 0);

            // Transition to indirect argument state
            context->TransitionBarrier(indirectSortBuffer, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
            context->FlushResourceBarriers();
        }
    }

    void IndirectCullPass::update(RHI::RenderGraph& graph, IndirectCullOutput& cullOutput)
    {
        RHI::RgResourceHandle uploadFrameUniformHandle = GImport(graph, pUploadFrameUniformBuffer.get());
        RHI::RgResourceHandle uploadRenderDataPerDrawHandle = GImport(graph, pUploadRenderDataPerDrawBuffer.get());
        RHI::RgResourceHandle uploadPropertiesPerMaterialHandle = GImport(graph, pUploadPropertiesPerMaterialBuffer.get());

        RHI::RgResourceHandle sortDispatchArgsHandle = graph.Create<RHI::D3D12Buffer>(sortDispatchArgsBufferDesc);
        RHI::RgResourceHandle grabDispatchArgsHandle = graph.Create<RHI::D3D12Buffer>(grabDispatchArgsBufferDesc);

        // import buffers
        cullOutput.perframeBufferHandle = GImport(graph, pFrameUniformBuffer.get());
        cullOutput.renderDataPerDrawHandle = GImport(graph, pRenderDataPerDrawBuffer.get());
        cullOutput.propertiesPerMaterialHandle = GImport(graph, pPropertiesPerMaterialBuffer.get());

        cullOutput.opaqueDrawHandle = DrawCallCommandBufferHandle {
            GImport(graph, commandBufferForOpaqueDraw.p_IndirectIndexCommandBuffer.get()),
            GImport(graph, commandBufferForOpaqueDraw.p_IndirectSortCommandBuffer.get())};

        cullOutput.transparentDrawHandle = DrawCallCommandBufferHandle {
            GImport(graph, commandBufferForTransparentDraw.p_IndirectIndexCommandBuffer.get()),
            GImport(graph, commandBufferForTransparentDraw.p_IndirectSortCommandBuffer.get())};

        for (size_t i = 0; i < dirShadowmapCommandBuffers.m_DrawCallCommandBuffer.size(); i++)
        {
            DrawCallCommandBufferHandle bufferHandle = {
                GImport(graph, dirShadowmapCommandBuffers.m_DrawCallCommandBuffer[i].p_IndirectIndexCommandBuffer.get()),
                GImport(graph, dirShadowmapCommandBuffers.m_DrawCallCommandBuffer[i].p_IndirectSortCommandBuffer.get())};
            cullOutput.directionShadowmapHandles.push_back(bufferHandle);
        }

        for (size_t i = 0; i < spotShadowmapCommandBuffer.size(); i++)
        {
            DrawCallCommandBufferHandle bufferHandle = {
                GImport(graph, spotShadowmapCommandBuffer[i].m_DrawCallCommandBuffer.p_IndirectIndexCommandBuffer.get()),
                GImport(graph, spotShadowmapCommandBuffer[i].m_DrawCallCommandBuffer.p_IndirectSortCommandBuffer.get())};
            cullOutput.spotShadowmapHandles.push_back(bufferHandle);
        }

        // reset pass
        auto mPerframeBufferHandle = cullOutput.perframeBufferHandle;
        auto mRenderDataPerDrawHandle = cullOutput.renderDataPerDrawHandle;
        auto mPropertiesPerMaterialHandle = cullOutput.propertiesPerMaterialHandle;
        auto mOpaqueDrawHandle = cullOutput.opaqueDrawHandle;
        auto mTransparentDrawHandle = cullOutput.transparentDrawHandle;
        auto mDirShadowmapHandles(cullOutput.directionShadowmapHandles);
        auto mSpotShadowmapHandles(cullOutput.spotShadowmapHandles);

        {
            RHI::RenderPass& resetPass = graph.AddRenderPass("ResetPass");

            resetPass.Read(uploadFrameUniformHandle, true);
            resetPass.Read(uploadPropertiesPerMaterialHandle, true);
            resetPass.Read(uploadRenderDataPerDrawHandle, true);

            resetPass.Write(cullOutput.perframeBufferHandle, true);
            resetPass.Write(cullOutput.propertiesPerMaterialHandle, true);
            resetPass.Write(cullOutput.renderDataPerDrawHandle, true);
            resetPass.Write(cullOutput.opaqueDrawHandle.indirectIndexBufferHandle, true);
            resetPass.Write(cullOutput.opaqueDrawHandle.indirectSortBufferHandle, true);
            resetPass.Write(cullOutput.transparentDrawHandle.indirectIndexBufferHandle, true);
            resetPass.Write(cullOutput.transparentDrawHandle.indirectSortBufferHandle, true);
            
            for (size_t i = 0; i < cullOutput.directionShadowmapHandles.size(); i++)
            {
                resetPass.Write(cullOutput.directionShadowmapHandles[i].indirectIndexBufferHandle, true);
                resetPass.Write(cullOutput.directionShadowmapHandles[i].indirectSortBufferHandle, true);
            }
            for (size_t i = 0; i < cullOutput.spotShadowmapHandles.size(); i++)
            {
                resetPass.Write(cullOutput.spotShadowmapHandles[i].indirectIndexBufferHandle, true);
                resetPass.Write(cullOutput.spotShadowmapHandles[i].indirectSortBufferHandle, true);
            }

            resetPass.Execute([=](RHI::RenderGraphRegistry* registry, RHI::D3D12CommandContext* context) {
                RHI::D3D12ComputeContext* pCopyContext = context->GetComputeContext();

                pCopyContext->TransitionBarrier(RegGetBufCounter(mOpaqueDrawHandle.indirectIndexBufferHandle), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST);
                pCopyContext->TransitionBarrier(RegGetBufCounter(mOpaqueDrawHandle.indirectSortBufferHandle), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST);
                pCopyContext->TransitionBarrier(RegGetBufCounter(mTransparentDrawHandle.indirectIndexBufferHandle), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST);
                pCopyContext->TransitionBarrier(RegGetBufCounter(mTransparentDrawHandle.indirectSortBufferHandle), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST);
                for (size_t i = 0; i < mDirShadowmapHandles.size(); i++)
                {
                    pCopyContext->TransitionBarrier(RegGetBufCounter(mDirShadowmapHandles[i].indirectSortBufferHandle), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST);
                }
                for (size_t i = 0; i < mSpotShadowmapHandles.size(); i++)
                {
                    pCopyContext->TransitionBarrier(RegGetBufCounter(mSpotShadowmapHandles[i].indirectSortBufferHandle), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST);
                }
                pCopyContext->TransitionBarrier(RegGetBuf(mPerframeBufferHandle), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST);
                pCopyContext->TransitionBarrier(RegGetBuf(mPropertiesPerMaterialHandle), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST);
                pCopyContext->TransitionBarrier(RegGetBuf(mRenderDataPerDrawHandle), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST);
                pCopyContext->FlushResourceBarriers();

                pCopyContext->ResetCounter(RegGetBufCounter(mOpaqueDrawHandle.indirectIndexBufferHandle));
                pCopyContext->ResetCounter(RegGetBufCounter(mOpaqueDrawHandle.indirectSortBufferHandle));
                pCopyContext->ResetCounter(RegGetBufCounter(mTransparentDrawHandle.indirectIndexBufferHandle));
                pCopyContext->ResetCounter(RegGetBufCounter(mTransparentDrawHandle.indirectSortBufferHandle));

                for (size_t i = 0; i < mDirShadowmapHandles.size(); i++)
                {
                    pCopyContext->ResetCounter(RegGetBufCounter(mDirShadowmapHandles[i].indirectSortBufferHandle));
                }

                for (size_t i = 0; i < mSpotShadowmapHandles.size(); i++)
                {
                    pCopyContext->ResetCounter(RegGetBufCounter(mSpotShadowmapHandles[i].indirectSortBufferHandle));
                }

                pCopyContext->CopyBuffer(RegGetBuf(mPerframeBufferHandle), RegGetBuf(uploadFrameUniformHandle));
                pCopyContext->CopyBuffer(RegGetBuf(mPropertiesPerMaterialHandle), RegGetBuf(uploadPropertiesPerMaterialHandle));
                pCopyContext->CopyBuffer(RegGetBuf(mRenderDataPerDrawHandle), RegGetBuf(uploadRenderDataPerDrawHandle));

                // transition buffer state
                {
                    pCopyContext->TransitionBarrier(RegGetBuf(mPerframeBufferHandle), D3D12_RESOURCE_STATE_GENERIC_READ);
                    pCopyContext->TransitionBarrier(RegGetBuf(mPropertiesPerMaterialHandle), D3D12_RESOURCE_STATE_GENERIC_READ);
                    pCopyContext->TransitionBarrier(RegGetBuf(mRenderDataPerDrawHandle), D3D12_RESOURCE_STATE_GENERIC_READ);

                    // transition vertex and index buffer state
                    std::vector<CachedMeshRenderer>& _mesh_renderers = m_render_scene->m_mesh_renderers;
                    uint32_t numMeshes = _mesh_renderers.size();
                    for (size_t i = 0; i < numMeshes; i++)
                    {
                        InternalMeshRenderer& temp_mesh_renderer = _mesh_renderers[i].internalMeshRenderer;
                        InternalMesh& temp_ref_mesh = temp_mesh_renderer.ref_mesh;
                        pCopyContext->TransitionBarrier(temp_ref_mesh.index_buffer.index_buffer.get(), D3D12_RESOURCE_STATE_COMMON);
                        pCopyContext->TransitionBarrier(temp_ref_mesh.vertex_buffer.vertex_buffer.get(), D3D12_RESOURCE_STATE_COMMON);
                    }
                }
                pCopyContext->FlushResourceBarriers();


                {
                    // transition vertex and index buffer state
                    std::vector<CachedMeshRenderer>& _mesh_renderers = m_render_scene->m_mesh_renderers;
                    uint32_t numMeshes = _mesh_renderers.size();
                    for (size_t i = 0; i < numMeshes; i++)
                    {
                        InternalMeshRenderer& temp_mesh_renderer = _mesh_renderers[i].internalMeshRenderer;
                        InternalMesh& temp_ref_mesh = temp_mesh_renderer.ref_mesh;
                        pCopyContext->TransitionBarrier(temp_ref_mesh.index_buffer.index_buffer.get(), D3D12_RESOURCE_STATE_INDEX_BUFFER);
                        pCopyContext->TransitionBarrier(temp_ref_mesh.vertex_buffer.vertex_buffer.get(), D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
                    }
                }
                pCopyContext->FlushResourceBarriers();
            });
        }

        int numMeshes = m_render_scene->m_mesh_renderers.size();
        if (numMeshes > 0)
        {
            {
                RHI::RenderPass& cullingPass = graph.AddRenderPass("OpaqueTransCullingPass");

                cullingPass.Read(cullOutput.perframeBufferHandle, true);
                cullingPass.Read(cullOutput.renderDataPerDrawHandle, true);
                cullingPass.Read(cullOutput.propertiesPerMaterialHandle, true);

                cullingPass.Write(cullOutput.opaqueDrawHandle.indirectIndexBufferHandle, true);
                cullingPass.Write(cullOutput.transparentDrawHandle.indirectIndexBufferHandle, true);

                cullingPass.Execute([=](RHI::RenderGraphRegistry* registry, RHI::D3D12CommandContext* context) {
                    RHI::D3D12ComputeContext* pAsyncCompute = context->GetComputeContext();

                    pAsyncCompute->TransitionBarrier(RegGetBuf(mPerframeBufferHandle), D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
                    pAsyncCompute->TransitionBarrier(RegGetBuf(mRenderDataPerDrawHandle), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
                    pAsyncCompute->TransitionBarrier(RegGetBuf(mPropertiesPerMaterialHandle), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
                    pAsyncCompute->TransitionBarrier(RegGetBuf(mOpaqueDrawHandle.indirectIndexBufferHandle), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
                    pAsyncCompute->TransitionBarrier(RegGetBufCounter(mOpaqueDrawHandle.indirectIndexBufferHandle), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
                    pAsyncCompute->TransitionBarrier(RegGetBuf(mTransparentDrawHandle.indirectIndexBufferHandle), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
                    pAsyncCompute->TransitionBarrier(RegGetBufCounter(mTransparentDrawHandle.indirectIndexBufferHandle), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
                    pAsyncCompute->FlushResourceBarriers();

                    pAsyncCompute->SetRootSignature(RootSignatures::pIndirectCullForSort.get());
                    pAsyncCompute->SetPipelineState(PipelineStates::pIndirectCullForSort.get());

                    struct RootIndexBuffer
                    {
                        UINT perFrameBufferIndex;
                        UINT renderDataPerDrawIndex;
                        UINT propertiesPerMaterialIndex;
                        UINT opaqueSortIndexDisBufferIndex;
                        UINT transSortIndexDisBufferIndex;
                    };

                    RootIndexBuffer rootIndexBuffer =
                        RootIndexBuffer {RegGetBufDefCBVIdx(mPerframeBufferHandle),
                                         RegGetBufDefSRVIdx(mRenderDataPerDrawHandle),
                                         RegGetBufDefSRVIdx(mPropertiesPerMaterialHandle),
                                         RegGetBufDefUAVIdx(mOpaqueDrawHandle.indirectIndexBufferHandle),
                                         RegGetBufDefUAVIdx(mTransparentDrawHandle.indirectIndexBufferHandle)};

                    pAsyncCompute->SetConstantArray(0, sizeof(RootIndexBuffer) / sizeof(UINT), &rootIndexBuffer);
                    pAsyncCompute->Dispatch1D(numMeshes, 128);
                });
            }

            {
                RHI::RenderPass& opaqueSortPass = graph.AddRenderPass("OpaqueBitonicSortPass");

                opaqueSortPass.Read(cullOutput.opaqueDrawHandle.indirectIndexBufferHandle, true);

                opaqueSortPass.Write(sortDispatchArgsHandle, true);
                opaqueSortPass.Write(cullOutput.opaqueDrawHandle.indirectIndexBufferHandle, true);

                opaqueSortPass.Execute([=](RHI::RenderGraphRegistry* registry, RHI::D3D12CommandContext* context) {
                    RHI::D3D12ComputeContext* pAsyncCompute = context->GetComputeContext();

                    RHI::D3D12Buffer* bufferPtr           = RegGetBuf(mOpaqueDrawHandle.indirectIndexBufferHandle);
                    RHI::D3D12Buffer* bufferCouterPtr     = bufferPtr->GetCounterBuffer().get();
                    RHI::D3D12Buffer* sortDispatchArgsPtr = RegGetBuf(sortDispatchArgsHandle);

                    bitonicSort(pAsyncCompute, bufferPtr, bufferCouterPtr, sortDispatchArgsPtr, false, true);
                });
            }

            {
                RHI::RenderPass& transSortPass = graph.AddRenderPass("TransparentBitonicSortPass");

                transSortPass.Read(cullOutput.transparentDrawHandle.indirectIndexBufferHandle, true);

                transSortPass.Write(sortDispatchArgsHandle, true);
                transSortPass.Write(cullOutput.transparentDrawHandle.indirectIndexBufferHandle, true);

                transSortPass.Execute([=](RHI::RenderGraphRegistry* registry, RHI::D3D12CommandContext* context) {
                    RHI::D3D12ComputeContext* pAsyncCompute = context->GetComputeContext();

                    RHI::D3D12Buffer* bufferPtr           = RegGetBuf(mTransparentDrawHandle.indirectIndexBufferHandle);
                    RHI::D3D12Buffer* bufferCouterPtr     = bufferPtr->GetCounterBuffer().get();
                    RHI::D3D12Buffer* sortDispatchArgsPtr = RegGetBuf(sortDispatchArgsHandle);

                    bitonicSort(pAsyncCompute, bufferPtr, bufferCouterPtr, sortDispatchArgsPtr, false, false);
                });
            }

            {
                RHI::RenderPass& grabOpaquePass = graph.AddRenderPass("GrabOpaquePass");

                grabOpaquePass.Read(cullOutput.renderDataPerDrawHandle, true);
                grabOpaquePass.Read(cullOutput.opaqueDrawHandle.indirectIndexBufferHandle, true);

                grabOpaquePass.Write(grabDispatchArgsHandle, true);
                grabOpaquePass.Write(cullOutput.opaqueDrawHandle.indirectSortBufferHandle, true);

                grabOpaquePass.Execute([=](RHI::RenderGraphRegistry* registry, RHI::D3D12CommandContext* context) {
                    RHI::D3D12ComputeContext* pAsyncCompute = context->GetComputeContext();

                    RHI::D3D12Buffer* renderDataPerDrawPtr   = RegGetBuf(mRenderDataPerDrawHandle);
                    RHI::D3D12Buffer* indirectIndexBufferPtr = RegGetBuf(mOpaqueDrawHandle.indirectIndexBufferHandle);
                    RHI::D3D12Buffer* indirectSortBufferPtr  = RegGetBuf(mOpaqueDrawHandle.indirectSortBufferHandle);
                    RHI::D3D12Buffer* grabDispatchArgsPtr    = RegGetBuf(grabDispatchArgsHandle);

                    grabObject(pAsyncCompute,
                        renderDataPerDrawPtr,
                        indirectIndexBufferPtr,
                        indirectSortBufferPtr,
                        grabDispatchArgsPtr);
                });
            }

            {
                RHI::RenderPass& grabTransPass = graph.AddRenderPass("GrabTransPass");

                grabTransPass.Read(cullOutput.renderDataPerDrawHandle, true);
                grabTransPass.Read(cullOutput.transparentDrawHandle.indirectIndexBufferHandle, true);

                grabTransPass.Write(grabDispatchArgsHandle, true);
                grabTransPass.Write(cullOutput.transparentDrawHandle.indirectSortBufferHandle, true);

                grabTransPass.Execute([=](RHI::RenderGraphRegistry* registry, RHI::D3D12CommandContext* context) {
                    RHI::D3D12ComputeContext* pAsyncCompute = context->GetComputeContext();

                    RHI::D3D12Buffer* renderDataPerDrawPtr = RegGetBuf(mRenderDataPerDrawHandle);
                    RHI::D3D12Buffer* indirectIndexBufferPtr = RegGetBuf(mTransparentDrawHandle.indirectIndexBufferHandle);
                    RHI::D3D12Buffer* indirectSortBufferPtr = RegGetBuf(mTransparentDrawHandle.indirectSortBufferHandle);
                    RHI::D3D12Buffer* grabDispatchArgsPtr = RegGetBuf(grabDispatchArgsHandle);

                    grabObject(pAsyncCompute,
                        renderDataPerDrawPtr,
                        indirectIndexBufferPtr,
                        indirectSortBufferPtr,
                        grabDispatchArgsPtr);
                });
            }
        }

        if (mDirShadowmapHandles.size() != 0 && numMeshes > 0)
        {
            RHI::RenderPass& dirLightShadowCullPass = graph.AddRenderPass("DirectionLightShadowCullPass");

            dirLightShadowCullPass.Read(cullOutput.perframeBufferHandle, true);
            dirLightShadowCullPass.Read(cullOutput.renderDataPerDrawHandle, true);
            dirLightShadowCullPass.Read(cullOutput.propertiesPerMaterialHandle, true);

            for (size_t i = 0; i < cullOutput.directionShadowmapHandles.size(); i++)
            {
                dirLightShadowCullPass.Write(cullOutput.directionShadowmapHandles[i].indirectSortBufferHandle, true);
            }

            dirLightShadowCullPass.Execute([=](RHI::RenderGraphRegistry* registry, RHI::D3D12CommandContext* context) {
                RHI::D3D12ComputeContext* pAsyncCompute = context->GetComputeContext();

                pAsyncCompute->TransitionBarrier(RegGetBuf(mPerframeBufferHandle), D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
                pAsyncCompute->TransitionBarrier(RegGetBuf(mRenderDataPerDrawHandle), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
                pAsyncCompute->TransitionBarrier(RegGetBuf(mPropertiesPerMaterialHandle), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
                for (size_t i = 0; i < mDirShadowmapHandles.size(); i++)
                {
                    pAsyncCompute->TransitionBarrier(RegGetBuf(mDirShadowmapHandles[i].indirectSortBufferHandle), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
                    //pAsyncCompute->InsertUAVBarrier(RegGetBuf(mDirShadowmapHandles[i].indirectSortBufferHandle));
                    pAsyncCompute->TransitionBarrier(RegGetBufCounter(mDirShadowmapHandles[i].indirectSortBufferHandle), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
                    //pAsyncCompute->InsertUAVBarrier(RegGetBufCounter(mDirShadowmapHandles[i].indirectSortBufferHandle));
                }
                pAsyncCompute->FlushResourceBarriers();

                for (int i = 0; i < mDirShadowmapHandles.size(); i++)
                {
                    pAsyncCompute->SetPipelineState(PipelineStates::pIndirectCullDirectionShadowmap.get());
                    pAsyncCompute->SetRootSignature(RootSignatures::pIndirectCullDirectionShadowmap.get());


                    pAsyncCompute->SetConstant(0, 0, RHI::DWParam(i));
                    pAsyncCompute->SetConstantBuffer(1, RegGetBuf(mPerframeBufferHandle)->GetGpuVirtualAddress());
                    pAsyncCompute->SetBufferSRV(2, RegGetBuf(mRenderDataPerDrawHandle));
                    pAsyncCompute->SetBufferSRV(3, RegGetBuf(mPropertiesPerMaterialHandle));
                    pAsyncCompute->SetDescriptorTable(4, RegGetBuf(mDirShadowmapHandles[i].indirectSortBufferHandle)->GetDefaultUAV()->GetGpuHandle());
                
                    pAsyncCompute->Dispatch1D(numMeshes, 128);

                    pAsyncCompute->TransitionBarrier(RegGetBuf(mDirShadowmapHandles[i].indirectSortBufferHandle), D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
                    pAsyncCompute->TransitionBarrier(RegGetBufCounter(mDirShadowmapHandles[i].indirectSortBufferHandle), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
                    pAsyncCompute->FlushResourceBarriers();
                }
            });
        }

        if (spotShadowmapCommandBuffer.size() != 0 && numMeshes > 0)
        {
            RHI::RenderPass& spotLightShadowCullPass = graph.AddRenderPass("SpotLightShadowCullPass");

            spotLightShadowCullPass.Read(cullOutput.perframeBufferHandle, true);
            spotLightShadowCullPass.Read(cullOutput.renderDataPerDrawHandle, true);
            spotLightShadowCullPass.Read(cullOutput.propertiesPerMaterialHandle, true);

            std::vector<uint32_t> _spotlightIndex;
            for (size_t i = 0; i < cullOutput.spotShadowmapHandles.size(); i++)
            {
                spotLightShadowCullPass.Write(cullOutput.spotShadowmapHandles[i].indirectSortBufferHandle, true);
                _spotlightIndex.push_back(spotShadowmapCommandBuffer[i].m_lightIndex);
            }

            spotLightShadowCullPass.Execute([=](RHI::RenderGraphRegistry* registry, RHI::D3D12CommandContext* context) {
                RHI::D3D12ComputeContext* pAsyncCompute = context->GetComputeContext();

                pAsyncCompute->TransitionBarrier(RegGetBuf(mPerframeBufferHandle), D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
                pAsyncCompute->TransitionBarrier(RegGetBuf(mRenderDataPerDrawHandle), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
                pAsyncCompute->TransitionBarrier(RegGetBuf(mPropertiesPerMaterialHandle), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
                for (size_t i = 0; i < mSpotShadowmapHandles.size(); i++)
                {
                    pAsyncCompute->TransitionBarrier(RegGetBuf(mSpotShadowmapHandles[i].indirectSortBufferHandle), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
                    //pAsyncCompute->InsertUAVBarrier(RegGetBuf(mSpotShadowmapHandles[i].indirectSortBufferHandle));
                    pAsyncCompute->TransitionBarrier(RegGetBufCounter(mSpotShadowmapHandles[i].indirectSortBufferHandle), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
                    //pAsyncCompute->InsertUAVBarrier(RegGetBufCounter(mSpotShadowmapHandles[i].indirectSortBufferHandle));
                }
                pAsyncCompute->FlushResourceBarriers();

                for (size_t i = 0; i < mSpotShadowmapHandles.size(); i++)
                {
                    pAsyncCompute->SetPipelineState(PipelineStates::pIndirectCullSpotShadowmap.get());
                    pAsyncCompute->SetRootSignature(RootSignatures::pIndirectCullSpotShadowmap.get());

                    pAsyncCompute->SetConstant(0, 0, _spotlightIndex[i]);
                    pAsyncCompute->SetConstantBuffer(1, RegGetBuf(mPerframeBufferHandle)->GetGpuVirtualAddress());
                    pAsyncCompute->SetBufferSRV(2, RegGetBuf(mRenderDataPerDrawHandle));
                    pAsyncCompute->SetBufferSRV(3, RegGetBuf(mPropertiesPerMaterialHandle));
                    pAsyncCompute->SetDescriptorTable(4, RegGetBuf(mSpotShadowmapHandles[i].indirectSortBufferHandle)->GetDefaultUAV()->GetGpuHandle());

                    pAsyncCompute->Dispatch1D(numMeshes, 128);

                    pAsyncCompute->TransitionBarrier(RegGetBuf(mSpotShadowmapHandles[i].indirectSortBufferHandle), D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
                    pAsyncCompute->TransitionBarrier(RegGetBufCounter(mSpotShadowmapHandles[i].indirectSortBufferHandle), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
                    pAsyncCompute->FlushResourceBarriers();
                }
            });
        }
    }

    void IndirectCullPass::destroy()
    {
        pUploadFrameUniformBuffer      = nullptr;
        pUploadRenderDataPerDrawBuffer = nullptr;
        pUploadPropertiesPerMaterialBuffer = nullptr;

        pFrameUniformBuffer      = nullptr;
        pRenderDataPerDrawBuffer = nullptr;
        pPropertiesPerMaterialBuffer = nullptr;

        commandBufferForOpaqueDraw.ResetBuffer();
        commandBufferForTransparentDraw.ResetBuffer();
        dirShadowmapCommandBuffers.Reset(); 
        for (size_t i = 0; i < spotShadowmapCommandBuffer.size(); i++)
            spotShadowmapCommandBuffer[i].Reset();
        spotShadowmapCommandBuffer.clear();

    }

}
