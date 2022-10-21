#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include "runtime/function/render/rhi/rhi.h"
#include "runtime/core/math/moyu_math.h"

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

    struct TextureData
    {
    public:
        uint32_t m_width {0};
        uint32_t m_height {0};
        uint32_t m_depth {0};
        uint32_t m_mip_levels {0};
        uint32_t m_array_layers {0};
        bool     m_is_srgb {false};
        bool     m_need_release {false};
        void*    m_pixels {nullptr};

        DXGI_FORMAT m_format {DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM};

        TextureData() = default;
        ~TextureData()
        {
            if (m_pixels && m_need_release)
            {
                free(m_pixels);
            }
        }
        bool isValid() const { return m_pixels != nullptr; }
        size_t getHashValue() const 
        {
            size_t h0 = std::hash<uint32_t> {}(m_width);
            h0 = (h0 ^ (std::hash<uint32_t> {}(m_height) << 1));
            h0 = (h0 ^ (std::hash<uint32_t> {}(m_depth) << 1));
            h0 = (h0 ^ (std::hash<uint32_t> {}(m_mip_levels) << 1));
            h0 = (h0 ^ (std::hash<uint32_t> {}(m_array_layers) << 1));
            h0 = (h0 ^ (std::hash<bool> {}(m_is_srgb) << 1));
            h0 = (h0 ^ (std::hash<bool> {}(m_need_release) << 1));
            h0 = (h0 ^ (std::hash<void*> {}(m_pixels) << 1));
            return h0;
        }
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
        bool   operator!=(const MeshSourceDesc& rhs) const { return m_mesh_file != rhs.m_mesh_file; }
        size_t getHashValue() const { return std::hash<std::string> {}(m_mesh_file); }
    };

    struct MaterialSourceDesc
    {
        bool m_blend;
        bool m_double_sided;

        Vector4 m_base_color_factor;
        float   m_metallic_factor;
        float   m_roughness_factor;
        float   m_normal_scale;
        float   m_occlusion_strength;
        Vector3 m_emissive_factor;

        std::string m_base_color_file;
        std::string m_metallic_roughness_file;
        std::string m_normal_file;
        std::string m_occlusion_file;
        std::string m_emissive_file;

        bool operator==(const MaterialSourceDesc& rhs) const
        {
            bool is_same = m_blend == rhs.m_blend;
            is_same &= m_double_sided == rhs.m_double_sided;

            is_same &= m_base_color_factor == rhs.m_base_color_factor;
            is_same &= m_metallic_factor == rhs.m_metallic_factor;
            is_same &= m_roughness_factor == rhs.m_roughness_factor;
            is_same &= m_normal_scale == rhs.m_normal_scale;
            is_same &= m_occlusion_strength == rhs.m_occlusion_strength;
            is_same &= m_emissive_factor == rhs.m_emissive_factor;

            is_same &= m_base_color_file == rhs.m_base_color_file;
            is_same &= m_metallic_roughness_file == rhs.m_metallic_roughness_file;
            is_same &= m_normal_file == rhs.m_normal_file;
            is_same &= m_occlusion_file == rhs.m_occlusion_file;
            is_same &= m_emissive_file == rhs.m_emissive_file;

            return is_same;
        }

        bool operator!=(const MaterialSourceDesc& rhs) const { return !(*this == rhs); }

        size_t getHashValue() const
        {
            size_t h0 = std::hash<bool> {}(m_blend);
            h0 = (h0 ^ (std::hash<bool> {}(m_double_sided) << 1));

            h0 = (h0 ^ (std::hash<float> {}(m_base_color_factor.x) << 1));
            h0 = (h0 ^ (std::hash<float> {}(m_base_color_factor.y) << 1));
            h0 = (h0 ^ (std::hash<float> {}(m_base_color_factor.z) << 1));
            h0 = (h0 ^ (std::hash<float> {}(m_base_color_factor.w) << 1));

            h0 = (h0 ^ (std::hash<float> {}(m_metallic_factor) << 1));
            h0 = (h0 ^ (std::hash<float> {}(m_roughness_factor) << 1));
            h0 = (h0 ^ (std::hash<float> {}(m_normal_scale) << 1));
            h0 = (h0 ^ (std::hash<float> {}(m_occlusion_strength) << 1));

            h0 = (h0 ^ (std::hash<float> {}(m_emissive_factor.x) << 1));
            h0 = (h0 ^ (std::hash<float> {}(m_emissive_factor.y) << 1));
            h0 = (h0 ^ (std::hash<float> {}(m_emissive_factor.z) << 1));

            h0 = (h0 ^ (std::hash<std::string> {}(m_base_color_file) << 1));
            h0 = (h0 ^ (std::hash<std::string> {}(m_metallic_roughness_file) << 1));
            h0 = (h0 ^ (std::hash<std::string> {}(m_normal_file) << 1));
            h0 = (h0 ^ (std::hash<std::string> {}(m_occlusion_file) << 1));
            h0 = (h0 ^ (std::hash<std::string> {}(m_emissive_file) << 1));

            return h0;
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
        bool m_blend;
        bool m_double_sided;

        Vector4 m_base_color_factor;
        float   m_metallic_factor;
        float   m_roughness_factor;
        float   m_normal_scale;
        float   m_occlusion_strength;
        Vector3 m_emissive_factor;

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
