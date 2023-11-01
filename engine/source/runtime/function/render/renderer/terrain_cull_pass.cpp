#include "runtime/function/render/renderer/terrain_cull_pass.h"
#include "runtime/resource/config_manager/config_manager.h"
#include "runtime/function/render/window_system.h"
#include "runtime/function/render/rhi/rhi_core.h"
#include "runtime/core/math/moyu_math2.h"
#include "runtime/function/render/rhi/d3d12/d3d12_graphicsCommon.h"

#include "fmt/core.h"
#include <cassert>

namespace MoYu
{
#define RegGetBuf(h) registry->GetD3D12Buffer(h)
#define RegGetBufCounter(h) registry->GetD3D12Buffer(h)->GetCounterBuffer().get()
#define RegGetTex(h) registry->GetD3D12Texture(h)
#define RegGetBufDefCBVIdx(h) registry->GetD3D12Buffer(h)->GetDefaultCBV()->GetIndex()
#define RegGetBufDefSRVIdx(h) registry->GetD3D12Buffer(h)->GetDefaultSRV()->GetIndex()
#define RegGetBufDefUAVIdx(h) registry->GetD3D12Buffer(h)->GetDefaultUAV()->GetIndex()

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

    void IndirectTerrainCullPass::initialize(const RenderPassInitInfo& init_info)
    {
        /*
        // create upload buffer
        grabDispatchArgsBufferDesc = CreateArgBufferDesc("GrabDispatchArgs", 22 * 23 / 2);

        // buffer for opaque draw
        terrainCommandBuffer = CreateIndexBuffer(L"OpaqueIndexBuffer");
        */
    }

    void IndirectTerrainCullPass::prepareMeshData(std::shared_ptr<RenderResource> render_resource)
    {
        /*
        // FrameUniforms
        HLSL::FrameUniforms* _frameUniforms = &render_resource->m_FrameUniforms;

        auto _HeightMap = m_render_scene->m_terrain_map.m_HeightMap.get();
        auto _NormalMap = m_render_scene->m_terrain_map.m_NormalMap.get();

        auto _HeightMapDesc = RHI::D3D12ShaderResourceView::GetDesc(_HeightMap, false, 0, 1);
        _HeightMapDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        auto _NormalMapDesc = RHI::D3D12ShaderResourceView::GetDesc(_NormalMap, false, 0, 1);
        _NormalMapDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

        auto _HeightMapSRV = _HeightMap->CreateSRV(_HeightMapDesc);
        auto _NormalMapSRV = _NormalMap->CreateSRV(_NormalMapDesc);

        HLSL::TerrainUniform _terrainUniform = {};
        _terrainUniform.terrainSize       = 1024;
        _terrainUniform.terrainMaxHeight  = 1024;
        _terrainUniform.heightMapIndex    = _HeightMapSRV->GetIndex();
        _terrainUniform.normalMapIndex    = _NormalMapSRV->GetIndex();
        _terrainUniform.local2WorldMatrix = glm::float4x4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1);

        _frameUniforms->terrainUniform = _terrainUniform;
        */
    }

    void IndirectTerrainCullPass::update(RHI::RenderGraph& graph, IndirectCullOutput& cullOutput)
    {

    }

    void IndirectTerrainCullPass::destroy()
    {


    }

}
