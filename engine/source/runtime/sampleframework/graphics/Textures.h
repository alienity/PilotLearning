//=================================================================================================
//
//  MJP's DX11 Sample Framework
//  http://mynameismjp.wordpress.com/
//
//  All code licensed under the MIT license
//
//=================================================================================================

#pragma once

#include "runtime/core/math/moyu_math2.h"
#include "runtime/function/render/render_common.h"

namespace SampleFramework11
{
    // Decode a texture and copies it to the CPU
    void GetTextureData(ID3D12CommandQueue* pCommandQueue, _In_ ID3D12Resource* pSource, _In_ bool isCubeMap, MoYu::MoYuScratchImage& result);

    // void SaveTextureAsEXR(const MoYu::MoYuScratchImage& texture, const char* filePath);

    glm::float3 MapXYSToDirection(glm::uint64 x, glm::uint64 y, glm::uint64 s, glm::uint64 width, glm::uint64 height);

    // == Texture Sampling Functions ==================================================================

    static glm::float4 SampleTexture2D(glm::float2 uv, glm::uint32 arraySlice, const MoYu::MoYuScratchImage& texData);
    static glm::float4 SampleTexture2D(glm::float2 uv, const MoYu::MoYuScratchImage& texData);
    static glm::float4 SampleCubemap(glm::float3 direction, const MoYu::MoYuScratchImage& texData);

}