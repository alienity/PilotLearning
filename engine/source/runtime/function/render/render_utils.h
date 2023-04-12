#pragma once

#include "runtime/core/math/moyu_math.h"
#include "runtime/function/render/render_scenes.h"
#include "runtime/function/render/rhi/d3d12/d3d12_resource.h"

namespace PScene
{
    class GfxBufferMap;
    class GfxTextureMap;
    class GfxBufferSerializer;
    class GfxTextureSerializer;

    class GfxBufferMap
    {
        std::shared_ptr<RHI::D3D12Buffer> Get(uint32_t hash);
    private:
        std::map<uint32_t, std::shared_ptr<RHI::D3D12Buffer>> buffers;
    };

    class GfxTextureMap
    {
        std::shared_ptr<RHI::D3D12Texture> Get(uint32_t hash);
    private:
        std::map<uint32_t, std::shared_ptr<RHI::D3D12Texture>> textures;
    };

    class GfxBufferSerializer
    {

    };

    class GfxTextureSerializer
    {

    };


} // namespace PScene

