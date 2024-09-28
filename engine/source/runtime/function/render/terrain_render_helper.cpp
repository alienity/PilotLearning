#include "runtime/function/render/terrain_render_helper.h"
#include "runtime/function/render/render_resource.h"
#include "runtime/core/math/moyu_math2.h"
#include "runtime/core/log/log_system.h"

namespace MoYu
{
    
	TerrainRenderHelper::TerrainRenderHelper()
	{
        mIsHeightmapInit = false;
        mIsNaterialInit = false;
        mIsPatchMeshInit = false;
	}

	TerrainRenderHelper::~TerrainRenderHelper()
	{

	}

    void TerrainRenderHelper::InitTerrainRenderer(
        SceneTerrainRenderer& sceneTerrainRenderer,
        SceneTerrainRenderer& cachedSceneTerrainRenderer,
        InternalTerrainRenderer& internalTerrainRenderer,
        RenderResource* renderResource)
	{
        last_cam_xz = glm::float2(0, 0);

        InitTerrainBasicInfo(sceneTerrainRenderer, internalTerrainRenderer.ref_terrain);
        InitTerrainPatchMesh(sceneTerrainRenderer, internalTerrainRenderer.ref_terrain, renderResource);
        InitTerrainHeightAndNormalMap(sceneTerrainRenderer, cachedSceneTerrainRenderer, internalTerrainRenderer.ref_terrain, renderResource);
        InitTerrainBaseTextures(sceneTerrainRenderer, cachedSceneTerrainRenderer, internalTerrainRenderer.ref_material, renderResource);
	}

    void TerrainRenderHelper::InitTerrainBasicInfo(SceneTerrainRenderer& sceneTerrainRenderer, InternalTerrain& internalTerrain)
    {
        internalTerrain.terrain_size = sceneTerrainRenderer.m_scene_terrain_mesh.terrain_size;
        internalTerrain.max_terrain_lod = sceneTerrainRenderer.m_scene_terrain_mesh.max_terrain_lod;
        internalTerrain.max_lod_node_count = sceneTerrainRenderer.m_scene_terrain_mesh.max_lod_node_count;
        internalTerrain.patch_mesh_grid_count = sceneTerrainRenderer.m_scene_terrain_mesh.patch_mesh_grid_count;
        internalTerrain.patch_mesh_size = sceneTerrainRenderer.m_scene_terrain_mesh.patch_mesh_size;
        internalTerrain.patch_count_per_node = sceneTerrainRenderer.m_scene_terrain_mesh.patch_count_per_node;

        int max_lod_node_count = internalTerrain.max_lod_node_count;
        int max_terrain_lod = internalTerrain.max_terrain_lod;
        int lod_scale = 2 * 2;

        int patch_mesh_grid_count = internalTerrain.patch_mesh_grid_count;
        int patch_mesh_size = internalTerrain.patch_mesh_size;
        int patch_count_per_node = internalTerrain.patch_count_per_node;

        int terrain_width = internalTerrain.terrain_size.x;

        internalTerrain.max_node_id = max_lod_node_count * max_lod_node_count * (1 - glm::pow(lod_scale, max_terrain_lod + 1)) / (1 - lod_scale) - 1;
        internalTerrain.patch_mesh_grid_size = patch_mesh_size / patch_mesh_grid_count;
        internalTerrain.sector_count_terrain = terrain_width / (patch_mesh_size * patch_count_per_node);;
    }

    void TerrainRenderHelper::InitTerrainPatchMesh(SceneTerrainRenderer& sceneTerrainRenderer, InternalTerrain& internalTerrain, RenderResource* renderResource)
    {
        if (mIsPatchMeshInit)
        {
            return;
        }
        mIsPatchMeshInit = true;

        terrainPatch = MoYu::Geometry::TerrainPlane::ToBasicMesh();

        {
            InternalScratchMesh* patch_scratch_mesh = &internalTerrain.patch_scratch_mesh;

            InternalScratchVertexBuffer* scratch_vertex_buffer = &patch_scratch_mesh->scratch_vertex_buffer;
            std::uint32_t vertex_buffer_size = terrainPatch.vertices.size() * sizeof(D3D12MeshVertexStandard);
            if (scratch_vertex_buffer->vertex_buffer == nullptr)
            {
                scratch_vertex_buffer->input_element_definition = D3D12MeshVertexPosition::InputElementDefinition;
                scratch_vertex_buffer->vertex_buffer = std::make_shared<MoYuScratchBuffer>();
                scratch_vertex_buffer->vertex_buffer->Initialize(vertex_buffer_size);
            }
            scratch_vertex_buffer->vertex_count = terrainPatch.vertices.size();
            memcpy(scratch_vertex_buffer->vertex_buffer->GetBufferPointer(), terrainPatch.vertices.data(), vertex_buffer_size);


            InternalScratchIndexBuffer* scratch_index_buffer = &patch_scratch_mesh->scratch_index_buffer;
            std::uint32_t index_buffer_size = terrainPatch.indices.size() * sizeof(std::uint32_t);
            if (scratch_index_buffer->index_buffer == nullptr)
            {
                scratch_index_buffer->index_stride = sizeof(uint32_t);
                scratch_index_buffer->index_buffer = std::make_shared<MoYuScratchBuffer>();
                scratch_index_buffer->index_buffer->Initialize(index_buffer_size);
            }
            scratch_index_buffer->index_count = terrainPatch.indices.size();
            memcpy(scratch_index_buffer->index_buffer->GetBufferPointer(), terrainPatch.indices.data(), index_buffer_size);
        }

        //************************************************************************

        {
            InternalScratchMesh* patch_scratch_mesh = &internalTerrain.patch_scratch_mesh;
            InternalMesh* patch_mesh = &internalTerrain.patch_mesh;

            uint32_t vertex_buffer_stride = sizeof(D3D12MeshVertexPosition);
            uint32_t vertex_buffer_size = patch_scratch_mesh->scratch_vertex_buffer.vertex_buffer->GetBufferSize();
            if (patch_mesh->vertex_buffer.vertex_buffer == nullptr)
            {
                void* vertex_buffer_data = patch_scratch_mesh->scratch_vertex_buffer.vertex_buffer->GetBufferPointer();
                patch_mesh->vertex_buffer.vertex_buffer =
                    renderResource->createDynamicBuffer(vertex_buffer_data, vertex_buffer_size, vertex_buffer_stride);
            }
            else
            {
                memcpy(patch_mesh->vertex_buffer.vertex_buffer->GetCpuVirtualAddress(),
                    patch_scratch_mesh->scratch_vertex_buffer.vertex_buffer->GetBufferPointer(),
                    vertex_buffer_size);
            }
            patch_mesh->vertex_buffer.vertex_count = patch_scratch_mesh->scratch_vertex_buffer.vertex_count;
            patch_mesh->vertex_buffer.input_element_definition = patch_scratch_mesh->scratch_vertex_buffer.input_element_definition;


            uint32_t index_buffer_stride = sizeof(std::uint32_t);
            uint32_t index_buffer_size = patch_scratch_mesh->scratch_index_buffer.index_buffer->GetBufferSize();
            if (patch_mesh->index_buffer.index_buffer == nullptr)
            {
                void* index_buffer_data = patch_scratch_mesh->scratch_index_buffer.index_buffer->GetBufferPointer();
                patch_mesh->index_buffer.index_buffer =
                    renderResource->createDynamicBuffer(index_buffer_data, index_buffer_size, index_buffer_stride);
            }
            else
            {
                memcpy(patch_mesh->index_buffer.index_buffer->GetCpuVirtualAddress(),
                    patch_scratch_mesh->scratch_index_buffer.index_buffer->GetBufferPointer(),
                    index_buffer_size);
            }
            patch_mesh->index_buffer.index_count = patch_scratch_mesh->scratch_index_buffer.index_count;
            patch_mesh->index_buffer.index_stride = patch_scratch_mesh->scratch_index_buffer.index_stride;
        }

    }

    void TerrainRenderHelper::InitTerrainHeightAndNormalMap(SceneTerrainRenderer& sceneTerrainRenderer,
                                                            SceneTerrainRenderer& cachedSceneTerrainRenderer,
                                                            InternalTerrain&      internalTerrain,
                                                            RenderResource*       renderResource)
    {
        bool isHeightmapSame = cachedSceneTerrainRenderer.m_scene_terrain_mesh == sceneTerrainRenderer.m_scene_terrain_mesh;
        if (!isHeightmapSame || !mIsHeightmapInit)
        {
            bool isHeightmapSame = cachedSceneTerrainRenderer.m_scene_terrain_mesh.m_terrain_height_map ==
                                   sceneTerrainRenderer.m_scene_terrain_mesh.m_terrain_height_map;
            bool isNormalmapSame = cachedSceneTerrainRenderer.m_scene_terrain_mesh.m_terrain_normal_map ==
                                   sceneTerrainRenderer.m_scene_terrain_mesh.m_terrain_normal_map;

            if (!isHeightmapSame || !mIsHeightmapInit)
            {
                terrain_heightmap_scratch = renderResource->loadImage(
                    sceneTerrainRenderer.m_scene_terrain_mesh.m_terrain_height_map.m_image_file);
            }
            if (isNormalmapSame || !mIsHeightmapInit)
            {
                terrain_normalmap_scratch = renderResource->loadImage(
                    sceneTerrainRenderer.m_scene_terrain_mesh.m_terrain_normal_map.m_image_file);
            }

            if (!isHeightmapSame || !isNormalmapSame || !mIsHeightmapInit)
            {
                renderResource->startUploadBatch();
                if (!isHeightmapSame || !mIsHeightmapInit)
                {
                    SceneImage terrainSceneImage = sceneTerrainRenderer.m_scene_terrain_mesh.m_terrain_height_map;
                    terrain_heightmap = renderResource->createTex2D(terrain_heightmap_scratch,
                                                                     DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM,
                                                                     terrainSceneImage.m_is_srgb,
                                                                     terrainSceneImage.m_auto_mips,
                                                                     false);
                }
                if (!isNormalmapSame || !mIsHeightmapInit)
                {
                    SceneImage terrainSceneImage = sceneTerrainRenderer.m_scene_terrain_mesh.m_terrain_normal_map;
                    terrain_normalmap = renderResource->createTex2D(terrain_normalmap_scratch,
                                                                     DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM,
                                                                     terrainSceneImage.m_is_srgb,
                                                                     terrainSceneImage.m_auto_mips,
                                                                     false);
                }
                renderResource->endUploadBatch();
            }

            cachedSceneTerrainRenderer.m_scene_terrain_mesh = sceneTerrainRenderer.m_scene_terrain_mesh;

            mIsHeightmapInit = true;
        }

        internalTerrain.terrain_heightmap_scratch = terrain_heightmap_scratch;
        internalTerrain.terrain_normalmap_scratch = terrain_normalmap_scratch;

        internalTerrain.terrain_heightmap = terrain_heightmap;
        internalTerrain.terrain_normalmap = terrain_normalmap;

    }

    void TerrainRenderHelper::InitTerrainBaseTextures(
        SceneTerrainRenderer& sceneTerrainRenderer,
        SceneTerrainRenderer& cachedSceneTerrainRenderer,
        InternalMaterial& internalTerrainMaterial,
        RenderResource* renderResource)
    {
        bool isMaterialSame = cachedSceneTerrainRenderer.m_terrain_material == sceneTerrainRenderer.m_terrain_material;

        if (!isMaterialSame || !mIsNaterialInit)
        {
            renderResource->updateInternalMaterial(
                sceneTerrainRenderer.m_terrain_material,
                cachedSceneTerrainRenderer.m_terrain_material,
                internalTerrainMaterial, mIsNaterialInit);

            mIsNaterialInit = true;
        }
    }

    bool TerrainRenderHelper::updateInternalTerrainRenderer(
        RenderResource* renderResource,
        SceneTerrainRenderer& scene_terrain_renderer,
        SceneTerrainRenderer& cached_terrain_renderer,
        InternalTerrainRenderer& internal_terrain_renderer,
        glm::float4x4            model_matrix,
        glm::float4x4            model_matrix_inv,
        glm::float4x4            prev_model_matrix,
        glm::float4x4            prev_model_matrix_inv,
        bool                     has_initialized)
    {
        internal_terrain_renderer.model_matrix              = model_matrix;
        internal_terrain_renderer.model_matrix_inverse      = model_matrix_inv;
        internal_terrain_renderer.prev_model_matrix         = prev_model_matrix;
        internal_terrain_renderer.prev_model_matrix_inverse = prev_model_matrix_inv;

        bool is_terrain_same =
            scene_terrain_renderer.m_scene_terrain_mesh == cached_terrain_renderer.m_scene_terrain_mesh;
        is_terrain_same &= scene_terrain_renderer.m_terrain_material == cached_terrain_renderer.m_terrain_material;

        if (!is_terrain_same || !has_initialized)
        {
            internal_terrain_renderer.m_identifier = scene_terrain_renderer.m_identifier;
            this->InitTerrainRenderer(scene_terrain_renderer,
                                      cached_terrain_renderer,
                                      internal_terrain_renderer,
                                      renderResource);
        }

        cached_terrain_renderer = scene_terrain_renderer;

        return true;
    }


	float TerrainRenderHelper::GetTerrainHeight(InternalTerrain* internalTerrain, glm::float2 localXZ)
	{
        auto _terrain_height_map = internalTerrain->terrain_heightmap_scratch;

		auto _heightMetaData = _terrain_height_map->GetMetadata();
        auto _heightMap = _terrain_height_map->GetImage(0, 0, 0);

		if (localXZ.x < 0 || localXZ.y < 0 || localXZ.x >= _heightMetaData.width - 1 || localXZ.y >= _heightMetaData.height - 1)
            return -1;

		uint32_t x0 = glm::floor(localXZ.x);
		uint32_t x1 = glm::ceil(localXZ.x);
        uint32_t z0 = glm::floor(localXZ.y);
        uint32_t z1 = glm::ceil(localXZ.y);

        float xfrac = glm::fract(localXZ.x);
        float zfrac = glm::fract(localXZ.y);

		float _x0z0Height = (*(glm::uvec4*)(_heightMap->pixels + (uint32_t)(sizeof(uint8_t) * 4 * (x0 + z0 * _heightMetaData.width)))).z;
		float _x1z0Height = (*(glm::uvec4*)(_heightMap->pixels + (uint32_t)(sizeof(uint8_t) * 4 * (x1 + z0 * _heightMetaData.width)))).z;
		float _x0z1Height = (*(glm::uvec4*)(_heightMap->pixels + (uint32_t)(sizeof(uint8_t) * 4 * (x0 + z1 * _heightMetaData.width)))).z;
		float _x1z1Height = (*(glm::uvec4*)(_heightMap->pixels + (uint32_t)(sizeof(uint8_t) * 4 * (x1 + z1 * _heightMetaData.width)))).z;

		float _Height = glm::lerp(glm::lerp(_x0z0Height, _x1z0Height, xfrac), glm::lerp(_x0z1Height, _x1z1Height, xfrac), zfrac);

		return _Height;
	}

	glm::float3 TerrainRenderHelper::GetTerrainNormal(InternalTerrain* internalTerrain, glm::float2 localXZ)
	{
        auto _terrain_normal_map = internalTerrain->terrain_normalmap_scratch;

		auto _normalMetaData = _terrain_normal_map->GetMetadata();
        auto _normalMap = _terrain_normal_map->GetImage(0, 0, 0);

		if (localXZ.x < 0 || localXZ.y < 0 || localXZ.x >= _normalMetaData.width - 1 || localXZ.y >= _normalMetaData.height - 1)
            return glm::float3(-1);

		uint32_t x0 = glm::floor(localXZ.x);
		uint32_t x1 = glm::ceil(localXZ.x);
        uint32_t z0 = glm::floor(localXZ.y);
        uint32_t z1 = glm::ceil(localXZ.y);

        float xfrac = glm::fract(localXZ.x);
        float zfrac = glm::fract(localXZ.y);

		glm::uvec3 _x0z0NormalU = (*(glm::uvec4*)(_normalMap->pixels + (uint32_t)(sizeof(uint8_t) * 4 * (x0 + z0 * _normalMetaData.width))));
		glm::uvec3 _x1z0NormalU = (*(glm::uvec4*)(_normalMap->pixels + (uint32_t)(sizeof(uint8_t) * 4 * (x1 + z0 * _normalMetaData.width))));
		glm::uvec3 _x0z1NormalU = (*(glm::uvec4*)(_normalMap->pixels + (uint32_t)(sizeof(uint8_t) * 4 * (x0 + z1 * _normalMetaData.width))));
		glm::uvec3 _x1z1NormalU = (*(glm::uvec4*)(_normalMap->pixels + (uint32_t)(sizeof(uint8_t) * 4 * (x1 + z1 * _normalMetaData.width))));

		glm::float3 _x0z0Normal = glm::float3(_x0z0NormalU.x, _x0z0NormalU.y, _x0z0NormalU.z);
        glm::float3 _x1z0Normal = glm::float3(_x1z0NormalU.x, _x1z0NormalU.y, _x1z0NormalU.z);
        glm::float3 _x0z1Normal = glm::float3(_x0z1NormalU.x, _x0z1NormalU.y, _x0z1NormalU.z);
        glm::float3 _x1z1Normal = glm::float3(_x1z1NormalU.x, _x1z1NormalU.y, _x1z1NormalU.z);

		glm::float3 _Normal = glm::lerp(glm::lerp(_x0z0Normal, _x1z0Normal, xfrac), glm::lerp(_x0z1Normal, _x1z1Normal, xfrac), zfrac);

		return _Normal;
	}

    D3D12TerrainPatch CreatePatch(glm::float3 position, glm::float2 coord, glm::float4 color)
    {
        D3D12TerrainPatch _t;
        _t.position  = position;
        _t.texcoord0 = coord;
        _t.color     = color;
        _t.normal    = glm::vec3(0, 1, 0);
        _t.tangent   = glm::vec4(1, 0, 0, 1);
        return _t;
    }

}
