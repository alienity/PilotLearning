#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include "runtime/function/render/rhi/rhi.h"

namespace Pilot
{
    
    class BufferData
    {
    public:
        size_t m_size {0};
        void*  m_data {nullptr};

        BufferData() = delete;
        BufferData(size_t size)
        {
            m_size = size;
            m_data = malloc(size);
        }
        BufferData(void* data, size_t size)
        {
            m_size = size;
            m_data = malloc(size);
            memcpy(m_data, data, size);
        }
        ~BufferData()
        {
            if (m_data)
            {
                free(m_data);
            }
        }
        bool isValid() const { return m_data != nullptr; }
    };

    class TextureData
    {
    public:
        uint32_t m_width {0};
        uint32_t m_height {0};
        uint32_t m_depth {0};
        uint32_t m_mip_levels {0};
        uint32_t m_array_layers {0};
        bool     m_is_srgb {false};
        void*    m_pixels {nullptr};

        DXGI_FORMAT m_format {DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM};

        TextureData() = default;
        ~TextureData()
        {
            if (m_pixels)
            {
                free(m_pixels);
            }
        }
        bool isValid() const { return m_pixels != nullptr; }
    };

    struct MeshVertexDataDefinition
    {
        float x, y, z;        // position
        float nx, ny, nz;     // normal
        float tx, ty, tz, tw; // tangent
        float u, v;           // UV coordinates
    };

    struct MeshVertexBindingDataDefinition
    {
        int m_index0 {0};
        int m_index1 {0};
        int m_index2 {0};
        int m_index3 {0};

        float m_weight0 {0.f};
        float m_weight1 {0.f};
        float m_weight2 {0.f};
        float m_weight3 {0.f};
    };

    struct MeshSourceDesc
    {
        std::string m_mesh_file;

        bool   operator==(const MeshSourceDesc& rhs) const { return m_mesh_file == rhs.m_mesh_file; }
        size_t getHashValue() const { return std::hash<std::string> {}(m_mesh_file); }
    };

    struct MaterialSourceDesc
    {
        std::string m_base_color_file;
        std::string m_metallic_roughness_file;
        std::string m_normal_file;
        std::string m_occlusion_file;
        std::string m_emissive_file;

        bool operator==(const MaterialSourceDesc& rhs) const
        {
            return m_base_color_file == rhs.m_base_color_file &&
                   m_metallic_roughness_file == rhs.m_metallic_roughness_file && m_normal_file == rhs.m_normal_file &&
                   m_occlusion_file == rhs.m_occlusion_file && m_emissive_file == rhs.m_emissive_file;
        }
        size_t getHashValue() const
        {
            size_t h0 = std::hash<std::string> {}(m_base_color_file);
            size_t h1 = std::hash<std::string> {}(m_metallic_roughness_file);
            size_t h2 = std::hash<std::string> {}(m_normal_file);
            size_t h3 = std::hash<std::string> {}(m_occlusion_file);
            size_t h4 = std::hash<std::string> {}(m_emissive_file);
            return (((h0 ^ (h1 << 1)) ^ (h2 << 1)) ^ (h3 << 1)) ^ (h4 << 1);
        }
    };

    struct StaticMeshData
    {
        std::shared_ptr<BufferData> m_vertex_buffer;
        std::shared_ptr<BufferData> m_index_buffer;
    };

    struct RenderMeshData
    {
        StaticMeshData              m_static_mesh_data;
        std::shared_ptr<BufferData> m_skeleton_binding_buffer;
    };

    struct RenderMaterialData
    {
        std::shared_ptr<TextureData> m_base_color_texture;
        std::shared_ptr<TextureData> m_metallic_roughness_texture;
        std::shared_ptr<TextureData> m_normal_texture;
        std::shared_ptr<TextureData> m_occlusion_texture;
        std::shared_ptr<TextureData> m_emissive_texture;
    };
} // namespace Pilot

template<>
struct std::hash<Pilot::MeshSourceDesc>
{
    size_t operator()(const Pilot::MeshSourceDesc& rhs) const noexcept { return rhs.getHashValue(); }
};
template<>
struct std::hash<Pilot::MaterialSourceDesc>
{
    size_t operator()(const Pilot::MaterialSourceDesc& rhs) const noexcept { return rhs.getHashValue(); }
};
