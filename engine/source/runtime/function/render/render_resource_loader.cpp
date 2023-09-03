#include "runtime/function/render/render_resource_loader.h"

#include "runtime/core/base/macro.h"

#include "runtime/resource/asset_manager/asset_manager.h"
#include "runtime/resource/config_manager/config_manager.h"
#include "runtime/resource/res_type/data/mesh_data.h"

#include "runtime/function/global/global_context.h"

#include "runtime/function/render/render_mesh_loader.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define TINYEXR_IMPLEMENTATION
#include <tinyexr.h>

#include <algorithm>
#include <filesystem>
#include <vector>

namespace MoYu
{
    void RenderResourceBase::iniUploadBatch(RHI::D3D12Device* device)
    {
        m_Device         = device;
        m_ResourceUpload = device->GetLinkedDevice()->GetResourceUploadBatch();
        m_GraphicsMemory = device->GetLinkedDevice()->GetGraphicsMemory();
    }

    void RenderResourceBase::startUploadBatch() 
    {
        m_ResourceUpload->Begin();
    }

    void RenderResourceBase::endUploadBatch()
    {
        // Upload the resources to the GPU.
        auto uploadResourcesFinished = m_ResourceUpload->End(
            m_Device->GetLinkedDevice()->GetCommandQueue(RHI::RHID3D12CommandQueueType::Direct)->GetCommandQueue());

        // Wait for the upload thread to terminate
        uploadResourcesFinished.wait();
    }

    void RenderResourceBase::commitUploadBatch()
    {
        RHI::D3D12SyncHandle syncHandle = m_Device->GetLinkedDevice()
                                              ->GetCommandQueue(RHI::RHID3D12CommandQueueType::Direct)
                                              ->ExecuteCommandLists({}, false);

        m_GraphicsMemory->Commit(syncHandle);
    }

    std::shared_ptr<MoYuScratchImage> RenderResourceBase::loadTextureHDR(std::string file)
    {
        std::shared_ptr<MoYuScratchImage> texture = _TextureData_Caches[file];

        if (texture == nullptr)
        {
            std::shared_ptr<AssetManager> asset_manager = g_runtime_global_context.m_asset_manager;
            ASSERT(asset_manager);

            bool is_hdr = stbi_is_hdr(file.c_str());

            std::string fileFullPath = asset_manager->getFullPath(file).generic_string();

            texture = std::make_shared<TextureData>();

            int iw, ih, in;
            in = 4;
            if (is_hdr)
            {
                texture->m_pixels = stbi_loadf(fileFullPath.c_str(), &iw, &ih, &in, 0);
            }
            else
            {
                float* out; // width * height * RGBA
                const char* err = nullptr;
                int ret = LoadEXR((float**)&out, &iw, &ih, fileFullPath.c_str(), &err);

                if (ret != TINYEXR_SUCCESS)
                {
                    if (err)
                    {
                        fprintf(stderr, "ERR : %s\n", err);
                        FreeEXRErrorMessage(err); // release memory of error message.
                    }
                }

                texture->m_pixels = out;
            }

            if (!texture->m_pixels)
                return nullptr;

            texture->m_width    = iw;
            texture->m_height   = ih;
            texture->m_channels = in;

            texture->m_depth        = 1;
            texture->m_array_layers = 1;
            texture->m_mip_levels   = 1;

            texture->m_is_hdr  = true;

            _TextureData_Caches[file] = texture;
        }

        return texture;
    }

    std::shared_ptr<MoYuScratchImage> RenderResourceBase::loadTexture(std::string file, int force_channel)
    {
        std::shared_ptr<TextureData> texture = _TextureData_Caches[file];

        if (texture == nullptr)
        {
            std::shared_ptr<AssetManager> asset_manager = g_runtime_global_context.m_asset_manager;
            ASSERT(asset_manager);

            texture = std::make_shared<TextureData>();

            std::string fileFullPath = asset_manager->getFullPath(file).generic_string();

            int iw, ih, in;
            texture->m_pixels = stbi_load(fileFullPath.c_str(), &iw, &ih, &in, force_channel);

            if (!texture->m_pixels)
                return nullptr;

            texture->m_width  = iw;
            texture->m_height = ih;
            texture->m_channels = in;
            
            texture->m_depth  = 1;
            texture->m_mip_levels   = 0;
            texture->m_array_layers = 0;

            texture->m_is_hdr = false;
            texture->m_need_free = true;

            _TextureData_Caches[file] = texture;
        }

        return texture;
    }

    RenderMeshData RenderResourceBase::loadMeshData(std::string mesh_file)
    {
        if (_MeshData_Caches.find(mesh_file) == _MeshData_Caches.end())
        {
            std::shared_ptr<AssetManager> asset_manager = g_runtime_global_context.m_asset_manager;
            ASSERT(asset_manager);

            RenderMeshData ret {};

            if (std::filesystem::path(mesh_file).extension() == ".json")
            {
                std::shared_ptr<MeshData> bind_data = std::make_shared<MeshData>();
                asset_manager->loadAsset<MeshData>(mesh_file, *bind_data);

                ret.m_static_mesh_data.m_InputElementDefinition =
                    D3D12MeshVertexPositionNormalTangentTextureJointBinding::InputElementDefinition;

                // vertex buffer
                size_t vertex_size = bind_data->vertex_buffer.size() * sizeof(Vertex);
                ret.m_static_mesh_data.m_vertex_buffer = std::make_shared<BufferData>(vertex_size);
                memcpy(ret.m_static_mesh_data.m_vertex_buffer.get(), bind_data->vertex_buffer.data(), vertex_size);

                // index buffer
                size_t index_size = bind_data->index_buffer.size() * sizeof(int);
                ret.m_static_mesh_data.m_index_buffer = std::make_shared<BufferData>(index_size);
                memcpy(ret.m_static_mesh_data.m_index_buffer.get(), bind_data->index_buffer.data(), index_size);

                // skeleton binding buffer
                size_t skeleton_size = bind_data->skeleton_bind.size() * sizeof(SkeletonBinding);
                ret.m_skeleton_binding_buffer = std::make_shared<BufferData>(skeleton_size);
                memcpy(ret.m_skeleton_binding_buffer.get(), bind_data->skeleton_bind.data(), skeleton_size);

                MoYu::AxisAlignedBox bounding_box;
                for (size_t i = 0; i < bind_data->vertex_buffer.size(); i++)
                {
                    Vertex v = bind_data->vertex_buffer[i];
                    bounding_box.merge(Vector3(v.px, v.py, v.pz));
                }
                ret.m_axis_aligned_box = bounding_box;
            }
            else
            {
                MoYu::AxisAlignedBox bounding_box;
                ret.m_static_mesh_data = LoadModel(mesh_file, bounding_box);
                ret.m_axis_aligned_box = bounding_box;
            }

            _MeshData_Caches[mesh_file] = ret;

            return ret;
        }
        else
        {
            return _MeshData_Caches[mesh_file];
        }
    }

} // namespace MoYu
