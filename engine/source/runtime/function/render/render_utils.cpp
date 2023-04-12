#include "runtime/function/render/render_utils.h"

namespace PScene
{
    std::shared_ptr<RHI::D3D12Buffer> GfxBufferMap::Get(uint32_t hash)
    {
        auto bufferItr = buffers.find(hash);
        if (bufferItr != buffers.end())
        {
            return bufferItr->second;
        }
        return nullptr;
    }

    std::shared_ptr<RHI::D3D12Texture> GfxTextureMap::Get(uint32_t hash)
    {
        auto textureItr = textures.find(hash);
        if (textureItr != textures.end())
        {
            return textureItr->second;
        }
        return nullptr;
    }


} // namespace PScene
