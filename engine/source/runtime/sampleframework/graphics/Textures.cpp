//=================================================================================================
//
//  MJP's DX11 Sample Framework
//  http://mynameismjp.wordpress.com/
//
//  All code licensed under the MIT license
//
//=================================================================================================

#include "Textures.h"
#include "DDSTextureLoader.h"
#include "WICTextureLoader.h"
#include "TinyEXR.h"

namespace SampleFramework11
{
    void GetTextureData(ID3D12CommandQueue* pCommandQueue, _In_ ID3D12Resource* pSource, _In_ bool isCubeMap, MoYu::MoYuScratchImage& result)
    {
        
    }
    
    glm::float3 MapXYSToDirection(glm::uint64 x, glm::uint64 y, glm::uint64 s, glm::uint64 width, glm::uint64 height)
    {
        float u = ((x + 0.5f) / float(width)) * 2.0f - 1.0f;
        float v = ((y + 0.5f) / float(height)) * 2.0f - 1.0f;
        v *= -1.0f;

        glm::float3 dir = glm::float3(0.0f);

        // +x, -x, +y, -y, +z, -z
        switch(s) {
        case 0:
            dir = glm::normalize(glm::float3(1.0f, v, -u));
            break;
        case 1:
            dir = glm::normalize(glm::float3(-1.0f, v, u));
            break;
        case 2:
            dir = glm::normalize(glm::float3(u, 1.0f, -v));
            break;
        case 3:
            dir = glm::normalize(glm::float3(u, -1.0f, v));
            break;
        case 4:
            dir = glm::normalize(glm::float3(u, v, 1.0f));
            break;
        case 5:
            dir = glm::normalize(glm::float3(-u, v, -1.0f));
            break;
        }

        return dir;
    }

    glm::float4 U32ToFloat4(glm::u8vec4 InColor, bool IsSRGB = false)
    {
        glm::float4 finalColor;
        finalColor.x = InColor.x / 255.0f;
        finalColor.y = InColor.y / 255.0f;
        finalColor.z = InColor.z / 255.0f;
        finalColor.w = InColor.w / 255.0f;
        if(IsSRGB)
        {
            finalColor.x = glm::pow(finalColor.x, 2.2);
            finalColor.y = glm::pow(finalColor.y, 2.2);
            finalColor.z = glm::pow(finalColor.z, 2.2);
            finalColor.w = glm::pow(finalColor.w, 2.2);
        }
        return finalColor;
    }
    
    glm::float4 SampleTexture2D(glm::float2 uv, glm::uint32 arraySlice, const MoYu::MoYuScratchImage& texData)
    {
        const DirectX::TexMetadata& meta_data = texData.GetMetadata();
        
        glm::float2 texSize = glm::float2(float(meta_data.width), float(meta_data.height));
        glm::float2 halfTexelSize(0.5f / texSize.x, 0.5f / texSize.y);
        glm::float2 samplePos = glm::fract(uv - halfTexelSize);
        if(samplePos.x < 0.0f)
            samplePos.x = 1.0f + samplePos.x;
        if(samplePos.y < 0.0f)
            samplePos.y = 1.0f + samplePos.y;
        samplePos *= texSize;
        glm::uint32 samplePosX = MoYu::Min(samplePos.x, meta_data.width - 1.0f);
        glm::uint32 samplePosY = MoYu::Min(samplePos.y, meta_data.height - 1.0f);
        glm::uint32 samplePosXNext = MoYu::Min(samplePosX + 1.0f, meta_data.width - 1.0f);
        glm::uint32 samplePosYNext = MoYu::Min(samplePosY + 1.0f, meta_data.height - 1.0f);

        glm::float2 lerpAmts = glm::float2(glm::fract(samplePos.x), glm::fract(samplePos.y));

        const glm::uint32 sliceOffset = arraySlice * meta_data.width * meta_data.height;

        glm::float4 finalColor = glm::float4(0);

        const DirectX::Image* image_data = texData.GetImage(0, arraySlice, 0);

        assert(meta_data.format == DXGI_FORMAT::DXGI_FORMAT_B8G8R8A8_UNORM || meta_data.format == DXGI_FORMAT::DXGI_FORMAT_B8G8R8A8_UNORM_SRGB);

        bool isSRGB = meta_data.format == DXGI_FORMAT::DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
        
        glm::u8vec4* pixel_data = (glm::u8vec4*)image_data->pixels;
        
        glm::u8vec4 samples[4];
        samples[0] = pixel_data[samplePosY * meta_data.width + samplePosX];
        samples[1] = pixel_data[samplePosY * meta_data.width + samplePosXNext];
        samples[2] = pixel_data[samplePosYNext * meta_data.width + samplePosX];
        samples[3] = pixel_data[samplePosYNext * meta_data.width + samplePosXNext];

        glm::float4 sampleVal[4];
        sampleVal[0] = U32ToFloat4(samples[0], isSRGB);
        sampleVal[1] = U32ToFloat4(samples[1], isSRGB);
        sampleVal[2] = U32ToFloat4(samples[2], isSRGB);
        sampleVal[3] = U32ToFloat4(samples[3], isSRGB);

        finalColor = glm::lerp(glm::lerp(sampleVal[0], sampleVal[1], lerpAmts.x),
            glm::lerp(sampleVal[2], sampleVal[3], lerpAmts.x), lerpAmts.y);

        return finalColor;
    }

    glm::float4 SampleTexture2D(glm::float2 uv, const MoYu::MoYuScratchImage& texData)
    {
        return SampleTexture2D(uv, 0, texData);
    }

    glm::float4 SampleCubemap(glm::float3 direction, const MoYu::MoYuScratchImage& texData)
    {
        float maxComponent = std::max(std::max(std::abs(direction.x), std::abs(direction.y)), std::abs(direction.z));
        glm::uint32 faceIdx = 0;
        glm::float2 uv = glm::float2(direction.y, direction.z);
        if(direction.x == maxComponent)
        {
            faceIdx = 0;
            uv = glm::float2(-direction.z, -direction.y) / direction.x;
        }
        else if(-direction.x == maxComponent)
        {
            faceIdx = 1;
            uv = glm::float2(direction.z, -direction.y) / -direction.x;
        }
        else if(direction.y == maxComponent)
        {
            faceIdx = 2;
            uv = glm::float2(direction.x, direction.z) / direction.y;
        }
        else if(-direction.y == maxComponent)
        {
            faceIdx = 3;
            uv = glm::float2(direction.x, -direction.z) / -direction.y;
        }
        else if(direction.z == maxComponent)
        {
            faceIdx = 4;
            uv = glm::float2(direction.x, -direction.y) / direction.z;
        }
        else if(-direction.z == maxComponent)
        {
            faceIdx = 5;
            uv = glm::float2(-direction.x, -direction.y) / -direction.z;
        }
    
        uv = uv * glm::float2(0.5f, 0.5f) + glm::float2(0.5f, 0.5f);
        return SampleTexture2D(uv, faceIdx, texData);
    }
    
}