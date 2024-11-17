#pragma once

#include "runtime/function/render/render_scene.h"
#include "runtime/function/render/render_swap_context.h"
#include "runtime/function/render/render_common.h"

#include "runtime/function/render/rhi/d3d12/d3d12_graphicsMemory.h"
#include "runtime/function/render/rhi/d3d12/d3d12_resourceUploadBatch.h"

#include <memory>
#include <string>
#include <unordered_map>

namespace MoYu
{
    class RenderScene;
    class RenderCamera;

    class RenderResourceBase
    {
    public:
        robin_hood::unordered_map<std::string, std::shared_ptr<MoYuScratchImage>> _TextureData_Caches;
        robin_hood::unordered_map<std::string, std::shared_ptr<MoYuScratchBuffer>> _BufferData_Caches;

        robin_hood::unordered_map<std::string, RenderMeshData> _MeshData_Caches;

    public:
        RenderResourceBase() = default;

        virtual ~RenderResourceBase() {}

        void iniUploadBatch(RHI::D3D12Device* device);

        void startUploadBatch();
        void endUploadBatch();
        void commitUploadBatch();

        //std::shared_ptr<MoYuScratchImage> loadTextureHDR(std::string file);
        //std::shared_ptr<MoYuScratchImage> loadTexture(std::string file);
        std::shared_ptr<MoYuScratchImage> loadImage(std::string file);

        RenderMeshData loadMeshData(std::string mesh_file);

    //protected:
        RHI::D3D12Device*         m_Device;

    protected:
        RHI::ResourceUploadBatch* m_ResourceUpload;
        RHI::GraphicsMemory*      m_GraphicsMemory;
    };
} // namespace MoYu
