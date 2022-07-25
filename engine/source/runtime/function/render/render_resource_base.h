#pragma once

#include "runtime/function/render/render_scene.h"
#include "runtime/function/render/render_swap_context.h"
#include "runtime/function/render/render_type.h"

#include <memory>
#include <string>
#include <unordered_map>

#include "BufferHelpers.h"
#include "ResourceUploadBatch.h"

namespace Pilot
{
    class RenderScene;
    class RenderCamera;

    class RenderResourceBase
    {
    public:
        RenderResourceBase(RHI::D3D12Device* device) : m_device(device), m_resourceUpload(device->GetD3D12Device()) {}

        virtual ~RenderResourceBase() {}

        void startUploadBatch();
        void endUploadBatch();

        virtual void uploadGlobalRenderResource(LevelResourceDesc level_resource_desc) = 0;

        virtual void uploadGameObjectRenderResource(RenderEntity       render_entity,
                                                    RenderMeshData     mesh_data,
                                                    RenderMaterialData material_data) = 0;

        virtual void uploadGameObjectRenderResource(RenderEntity render_entity, RenderMeshData mesh_data) = 0;

        virtual void uploadGameObjectRenderResource(RenderEntity render_entity, RenderMaterialData material_data) = 0;

        virtual void updatePerFrameBuffer(std::shared_ptr<RenderScene>  render_scene,
                                          std::shared_ptr<RenderCamera> camera) = 0;

        // TODO: data caching
        std::shared_ptr<TextureData> loadTextureHDR(std::string file, int desired_channels = 4);
        std::shared_ptr<TextureData> loadTexture(std::string file, bool is_srgb = false);
        RenderMeshData               loadMeshData(const MeshSourceDesc& source, AxisAlignedBox& bounding_box);
        RenderMaterialData           loadMaterialData(const MaterialSourceDesc& source);
        AxisAlignedBox               getCachedBoudingBox(const MeshSourceDesc& source) const;

    protected:
        RHI::D3D12Device* m_device;
        DirectX::ResourceUploadBatch m_resourceUpload;

    private:
        StaticMeshData loadStaticMesh(std::string mesh_file, AxisAlignedBox& bounding_box);

        std::unordered_map<MeshSourceDesc, AxisAlignedBox> m_bounding_box_cache_map;
    };
} // namespace Pilot
