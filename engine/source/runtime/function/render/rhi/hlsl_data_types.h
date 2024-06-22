#pragma once
#include "rhi.h"

#ifndef GLM_FORCE_RADIANS
#define GLM_FORCE_RADIANS 1
#endif

#ifndef GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_DEPTH_ZERO_TO_ONE 1
#endif

#include <glm/glm.hpp>

#ifndef _CPP_MACRO_
#define _CPP_MACRO_
#endif


// datas map to hlsl
namespace HLSL
{

#include "../shader/pipeline/Runtime/ShaderLibrary/API/D3D12.hlsl"
#include "../shader/pipeline/Runtime/ShaderLibrary/ShaderVariablesGlobal.hlsl"
#include "../shader/pipeline/Runtime/Material/Lit/LitProperties.hlsl"

    static constexpr size_t MaterialLimit = 2048;
    static constexpr size_t LightLimit    = 32;
    static constexpr size_t MeshLimit     = 2048;

    static constexpr uint32_t GeoClipMeshType = 5;

    static const uint32_t m_point_light_shadow_map_dimension       = 2048;
    static const uint32_t m_directional_light_shadow_map_dimension = 4096;

    static uint32_t const m_mesh_per_drawcall_max_instance_count = 64;
    static uint32_t const m_mesh_vertex_blending_max_joint_count = 1024;

    struct BitonicSortCommandSigParams
    {
        glm::uint  MeshIndex;
        float MeshToCamDis;
    };

    struct CommandSignatureParams
    {
        glm::uint                    MeshIndex;
        D3D12_VERTEX_BUFFER_VIEW     VertexBuffer;
        D3D12_INDEX_BUFFER_VIEW      IndexBuffer;
        D3D12_DRAW_INDEXED_ARGUMENTS DrawIndexedArguments;
    };
    
    const std::uint64_t totalCommandBufferSizeInBytes = MeshLimit * sizeof(CommandSignatureParams);

    const std::uint64_t totalIndexCommandBufferInBytes = MeshLimit * sizeof(BitonicSortCommandSigParams);

    /*
    struct ClipmapTransform
    {
        glm::float4x4 transform;
        int mesh_type;
    };

    struct ClipmapMeshCount
    {
        int tile_count;
        int filler_count;
        int trim_count;
        int seam_count;
        int cross_cunt;
        int total_count;
    };

    struct ClipMeshCommandSigParams
    {
        D3D12_VERTEX_BUFFER_VIEW     VertexBuffer;
        D3D12_INDEX_BUFFER_VIEW      IndexBuffer;
        D3D12_DRAW_INDEXED_ARGUMENTS DrawIndexedArguments;
        glm::int3                    _Padding_0;
        glm::float4x2                ClipBoundingBox;
    };

    struct ToDrawCommandSignatureParams
    {
        uint32_t                     ClipIndex;
        D3D12_VERTEX_BUFFER_VIEW     VertexBuffer;
        D3D12_INDEX_BUFFER_VIEW      IndexBuffer;
        D3D12_DRAW_INDEXED_ARGUMENTS DrawIndexedArguments;
    };
    */

    // 512 * 512
    #define MaxTerrainNodeCount 262144

} // namespace HLSL
