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
	}

	TerrainRenderHelper::~TerrainRenderHelper()
	{
        mTerrain3D = nullptr;
	}

    void TerrainRenderHelper::InitTerrainRenderer(SceneTerrainRenderer*    sceneTerrainRenderer,
                                                  SceneTerrainRenderer*    cachedSceneTerrainRenderer,
                                                  InternalTerrain*         internalTerrain,
                                                  InternalTerrainMaterial* internalTerrainMaterial,
                                                  RenderResource*          renderResource)
	{
        if (mTerrain3D == nullptr)
        {
            mTerrain3D = std::make_shared<Terrain3D>();
        }
        mTerrain3D->snap(glm::float3(0, 0, 0));

        mSceneTerrainRenderer = sceneTerrainRenderer;
        mCachedSceneTerrainRenderer = cachedSceneTerrainRenderer;

        mInternalTerrain = internalTerrain;
        mInternalTerrainMaterial = internalTerrainMaterial;
        mRenderResource = renderResource;

        InitTerrainBasicInfo();
        InitInternalTerrainClipmap();
        InitTerrainHeightAndNormalMap();
        InitTerrainBaseTextures();
	}

    void TerrainRenderHelper::InitTerrainBasicInfo()
    {
        mInternalTerrain->terrain_size       = mSceneTerrainRenderer->m_scene_terrain_mesh.terrain_size;
        mInternalTerrain->terrain_max_height = mSceneTerrainRenderer->m_scene_terrain_mesh.terrian_max_height;
    }

    void TerrainRenderHelper::InitTerrainHeightAndNormalMap()
    {
        bool isHeightmapSame = mCachedSceneTerrainRenderer->m_scene_terrain_mesh == mSceneTerrainRenderer->m_scene_terrain_mesh;
        if (!isHeightmapSame || !mIsHeightmapInit)
        {
            bool isHeightmapSame = mCachedSceneTerrainRenderer->m_scene_terrain_mesh.m_terrain_height_map ==
                                   mSceneTerrainRenderer->m_scene_terrain_mesh.m_terrain_height_map;
            bool isNormalmapSame = mCachedSceneTerrainRenderer->m_scene_terrain_mesh.m_terrain_normal_map ==
                                   mSceneTerrainRenderer->m_scene_terrain_mesh.m_terrain_normal_map;

            if (!isHeightmapSame || !mIsHeightmapInit)
            {
                terrain_heightmap_scratch = mRenderResource->loadImage(
                    mSceneTerrainRenderer->m_scene_terrain_mesh.m_terrain_height_map.m_image_file);
            }
            if (isNormalmapSame || !mIsHeightmapInit)
            {
                terrain_normalmap_scratch = mRenderResource->loadImage(
                    mSceneTerrainRenderer->m_scene_terrain_mesh.m_terrain_normal_map.m_image_file);
            }

            if (!isHeightmapSame || !isNormalmapSame || !mIsHeightmapInit)
            {
                mRenderResource->startUploadBatch();
                if (!isHeightmapSame || !mIsHeightmapInit)
                {
                    SceneImage terrainSceneImage = mSceneTerrainRenderer->m_scene_terrain_mesh.m_terrain_height_map;
                    terrain_heightmap = mRenderResource->createTex2D(terrain_heightmap_scratch,
                                                                     DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM,
                                                                     terrainSceneImage.m_is_srgb,
                                                                     terrainSceneImage.m_auto_mips,
                                                                     false);
                }
                if (!isNormalmapSame || !mIsHeightmapInit)
                {
                    SceneImage terrainSceneImage = mSceneTerrainRenderer->m_scene_terrain_mesh.m_terrain_normal_map;
                    terrain_normalmap = mRenderResource->createTex2D(terrain_normalmap_scratch,
                                                                     DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM,
                                                                     terrainSceneImage.m_is_srgb,
                                                                     terrainSceneImage.m_auto_mips,
                                                                     false);
                }
                mRenderResource->endUploadBatch();
            }

            mCachedSceneTerrainRenderer->m_scene_terrain_mesh = mSceneTerrainRenderer->m_scene_terrain_mesh;

            mIsHeightmapInit = true;
        }

        mInternalTerrain->terrain_heightmap_scratch = terrain_heightmap_scratch;
        mInternalTerrain->terrain_normalmap_scratch = terrain_normalmap_scratch;

        mInternalTerrain->terrain_heightmap = terrain_heightmap;
        mInternalTerrain->terrain_normalmap = terrain_normalmap;

    }

    void TerrainRenderHelper::InitTerrainBaseTextures()
    {
        bool isMaterialSame = mCachedSceneTerrainRenderer->m_terrain_material == mSceneTerrainRenderer->m_terrain_material;

        if (!isMaterialSame || !mIsNaterialInit)
        {
            mRenderResource->startUploadBatch();

            MoYu::TerrainMaterial& m_terrain_material = mSceneTerrainRenderer->m_terrain_material;

            int base_tex_count = sizeof(m_terrain_material.m_base_texs) / sizeof(TerrainBaseTex);

            for (int i = 0; i < base_tex_count; i++)
            {
                InternalTerrainBaseTexture& _internalBaseTex = mInternalBaseTextures[i];

                TerrainBaseTex baseTex = m_terrain_material.m_base_texs[i];

                bool isSame = mCachedSceneTerrainRenderer->m_terrain_material.m_base_texs[i].m_albedo_file ==
                              mSceneTerrainRenderer->m_terrain_material.m_base_texs[i].m_albedo_file;
                if (!isSame || !mIsNaterialInit)
                {
                    auto _tex = mRenderResource->SceneImageToTexture(baseTex.m_albedo_file.m_image);
                    _internalBaseTex.albedo = _tex == nullptr ? mRenderResource->_Default2TexMap[BaseColor] : _tex;
                }
                _internalBaseTex.albedo_tilling = baseTex.m_albedo_file.m_tilling;

                isSame = mCachedSceneTerrainRenderer->m_terrain_material.m_base_texs[i].m_ao_roughness_metallic_file ==
                         mSceneTerrainRenderer->m_terrain_material.m_base_texs[i].m_ao_roughness_metallic_file;
                if (!isSame || !mIsNaterialInit)
                {
                    auto _tex = mRenderResource->SceneImageToTexture(baseTex.m_ao_roughness_metallic_file.m_image);
                    _internalBaseTex.ao_roughness_metallic = _tex == nullptr ? mRenderResource->_Default2TexMap[Black] : _tex;
                }
                _internalBaseTex.ao_roughness_metallic_tilling = baseTex.m_ao_roughness_metallic_file.m_tilling;

                isSame = mCachedSceneTerrainRenderer->m_terrain_material.m_base_texs[i].m_displacement_file ==
                         mSceneTerrainRenderer->m_terrain_material.m_base_texs[i].m_displacement_file;
                if (!isSame || !mIsNaterialInit)
                {
                    auto _tex = mRenderResource->SceneImageToTexture(baseTex.m_displacement_file.m_image);
                    _internalBaseTex.displacement = _tex == nullptr ? mRenderResource->_Default2TexMap[BaseColor] : _tex;
                }
                _internalBaseTex.displacement_tilling = baseTex.m_displacement_file.m_tilling;

                isSame = mCachedSceneTerrainRenderer->m_terrain_material.m_base_texs[i].m_normal_file ==
                         mSceneTerrainRenderer->m_terrain_material.m_base_texs[i].m_normal_file;
                if (!isSame || !mIsNaterialInit)
                {
                    auto _tex = mRenderResource->SceneImageToTexture(baseTex.m_normal_file.m_image);
                    _internalBaseTex.normal = _tex == nullptr ? mRenderResource->_Default2TexMap[Green] : _tex;
                }
                _internalBaseTex.normal_tilling = baseTex.m_normal_file.m_tilling;
            }

            mRenderResource->endUploadBatch();

            mCachedSceneTerrainRenderer->m_terrain_material == mSceneTerrainRenderer->m_terrain_material;

            mIsNaterialInit = true;
        }

        int base_tex_count = sizeof(mCachedSceneTerrainRenderer->m_terrain_material.m_base_texs) / sizeof(TerrainBaseTex);
        for (int i = 0; i < base_tex_count; i++)
        {
            mInternalTerrainMaterial->terrain_base_textures[i] = mInternalBaseTextures[i];
        }

    }

    void TerrainRenderHelper::InitInternalTerrainClipmap()
    {
        mInternalTerrain->mesh_size = mTerrain3D->get_mesh_size();
        mInternalTerrain->mesh_lods = mTerrain3D->get_mesh_lods();

        //********************************************************************************
        // update scratch buffer
        {
            InternalScratchClipmap* _internalScratchClipmap = &mInternalTerrain->terrain_scratch_clipmap;

            ClipmapInstanceScratchBuffer* _clipmapInstanceScratchBuffer = &_internalScratchClipmap->instance_scratch_buffer;

            int clip_counts = GetClipCount();
            _clipmapInstanceScratchBuffer->clip_index_counts = clip_counts;

            std::shared_ptr<MoYuScratchBuffer> _clip_buffer = _clipmapInstanceScratchBuffer->clip_buffer;
            if (_clip_buffer == nullptr)
            {
                uint32_t clip_total_size = clip_counts * sizeof(HLSL::ClipmapIndexInstance);
                _clip_buffer = std::make_shared<MoYuScratchBuffer>();
                _clip_buffer->Initialize(clip_total_size);
                _clipmapInstanceScratchBuffer->clip_buffer = _clip_buffer;
            }
            HLSL::ClipmapIndexInstance* _clipmapIndexInstance =
                (HLSL::ClipmapIndexInstance*)_clip_buffer->GetBufferPointer();
            UpdateClipBuffer(_clipmapIndexInstance);

            auto _clipPatches = mTerrain3D->get_clip_patch();
            for (int i = 0; i < 5; i++)
            {
                UpdateClipPatchScratchMesh(&_internalScratchClipmap->clipmap_scratch_mesh[i], _clipPatches[i]);
            }
        }
        //********************************************************************************

        //********************************************************************************
        // update d3d12 buffer
        {
            std::shared_ptr<MoYuScratchBuffer> _clip_scratch_buffer =
                mInternalTerrain->terrain_scratch_clipmap.instance_scratch_buffer.clip_buffer;

            InternalClipmap* _terrain_clipmap = &mInternalTerrain->terrain_clipmap;

            ClipmapInstanceBuffer* _clipmapInstanceBuffer = &_terrain_clipmap->instance_buffer;

            int clip_counts = GetClipCount();
            _clipmapInstanceBuffer->clip_index_counts = clip_counts;

            uint32_t clip_total_size = clip_counts * sizeof(HLSL::ClipmapIndexInstance);
            std::shared_ptr<RHI::D3D12Buffer> _clip_buffer = _clipmapInstanceBuffer->clip_buffer;
            if (_clip_buffer == nullptr)
            {
                _clip_buffer = mRenderResource->createDynamicBuffer(_clip_scratch_buffer);
                _clipmapInstanceBuffer->clip_buffer = _clip_buffer;
            }
            else
            {
                HLSL::ClipmapIndexInstance * _clipmapIndexInstance = _clip_buffer->GetCpuVirtualAddress<HLSL::ClipmapIndexInstance>();
                memcpy(_clipmapIndexInstance, _clip_scratch_buffer->GetBufferPointer(), clip_total_size);
            }
            auto _clipPatches = mTerrain3D->get_clip_patch();
            for (int i = 0; i < 5; i++)
            {
                UpdateClipPatchMesh(&_terrain_clipmap->clipmap_mesh[i],
                                    &mInternalTerrain->terrain_scratch_clipmap.clipmap_scratch_mesh[i]);
            }
        }
        //********************************************************************************
    }

    void TerrainRenderHelper::UpdateClipPatchMesh(InternalMesh* internalMesh, InternalScratchMesh* internalScratchMesh)
    {
        if (internalMesh->vertex_buffer.vertex_buffer == nullptr)
        {
            internalMesh->vertex_buffer.vertex_buffer =
                mRenderResource->createDynamicBuffer(internalScratchMesh->scratch_vertex_buffer.vertex_buffer);
        }
        else
        {
            uint32_t vertex_buffer_size = internalScratchMesh->scratch_vertex_buffer.vertex_buffer->GetBufferSize() *
                                          sizeof(D3D12MeshVertexPosition);
            memcpy(internalMesh->vertex_buffer.vertex_buffer->GetCpuVirtualAddress(),
                   internalScratchMesh->scratch_vertex_buffer.vertex_buffer->GetBufferPointer(),
                   vertex_buffer_size);
        }
        internalMesh->vertex_buffer.vertex_count = internalScratchMesh->scratch_vertex_buffer.vertex_count;

        if (internalMesh->index_buffer.index_buffer == nullptr)
        {
            internalMesh->index_buffer.index_buffer =
                mRenderResource->createDynamicBuffer(internalScratchMesh->scratch_index_buffer.index_buffer);
        }
        else
        {
            uint32_t index_buffer_size =
                internalScratchMesh->scratch_index_buffer.index_buffer->GetBufferSize() * sizeof(std::uint32_t);
            memcpy(internalMesh->index_buffer.index_buffer->GetCpuVirtualAddress(),
                   internalScratchMesh->scratch_index_buffer.index_buffer->GetBufferPointer(),
                   index_buffer_size);
        }
        internalMesh->index_buffer.index_count  = internalScratchMesh->scratch_index_buffer.index_count;
        internalMesh->index_buffer.index_stride = internalScratchMesh->scratch_index_buffer.index_stride;
    }

    void TerrainRenderHelper::UpdateClipPatchScratchMesh(InternalScratchMesh* internalScratchMesh, GeoClipPatch geoPatch)
    {
        InternalScratchVertexBuffer* scratch_vertex_buffer = &internalScratchMesh->scratch_vertex_buffer;
        std::uint32_t vertex_buffer_size = geoPatch.vertices.size() * sizeof(D3D12MeshVertexPosition);
        if (scratch_vertex_buffer->vertex_buffer == nullptr)
        {
            scratch_vertex_buffer->input_element_definition = D3D12MeshVertexPosition::InputElementDefinition;
            scratch_vertex_buffer->vertex_buffer = std::make_shared<MoYuScratchBuffer>();
            scratch_vertex_buffer->vertex_buffer->Initialize(vertex_buffer_size);
        }
        scratch_vertex_buffer->vertex_count = geoPatch.vertices.size();
        memcpy(scratch_vertex_buffer->vertex_buffer->GetBufferPointer(), geoPatch.vertices.data(), vertex_buffer_size);

        
        InternalScratchIndexBuffer* scratch_index_buffer = &internalScratchMesh->scratch_index_buffer;
        std::uint32_t index_buffer_size = geoPatch.indices.size() * sizeof(std::uint32_t);
        if (scratch_index_buffer->index_buffer == nullptr)
        {
            scratch_index_buffer->index_stride  = sizeof(uint32_t);
            scratch_index_buffer->index_buffer = std::make_shared<MoYuScratchBuffer>();
            scratch_index_buffer->index_buffer->Initialize(index_buffer_size);
        }
        scratch_index_buffer->index_count  = geoPatch.indices.size();
        memcpy(scratch_index_buffer->index_buffer->GetBufferPointer(), geoPatch.indices.data(), index_buffer_size);
    }

    uint32_t TerrainRenderHelper::GetClipCount()
    {
        const auto& transform_instance = mTerrain3D->get_transform_instance();

        int clip_counts = 1 + transform_instance.tiles.size() + transform_instance.fillers.size() +
                          transform_instance.trims.size() + transform_instance.seams.size();

        return clip_counts;
    }

    void TerrainRenderHelper::UpdateClipBuffer(HLSL::ClipmapIndexInstance* clipmapIdxInstance)
    {
        const auto& transform_instance = mTerrain3D->get_transform_instance();

        int clip_counts = 1 + transform_instance.tiles.size() + transform_instance.fillers.size() +
                          transform_instance.trims.size() + transform_instance.seams.size();

        uint32_t clip_count = 0;
        // store cross matrix and type
        clipmapIdxInstance[clip_count].mesh_type = GeoClipMap::CROSS;
        clipmapIdxInstance[clip_count].transform = transform_instance.cross.getMatrix();
        clip_count += 1;
        // store tiles
        for (int i = 0; i < transform_instance.tiles.size(); i++)
        {
            clipmapIdxInstance[clip_count].mesh_type = GeoClipMap::TILE;
            clipmapIdxInstance[clip_count].transform = transform_instance.tiles[i].getMatrix();
            clip_count += 1;
        }
        // store fillers
        for (int i = 0; i < transform_instance.fillers.size(); i++)
        {
            clipmapIdxInstance[clip_count].mesh_type = GeoClipMap::FILLER;
            clipmapIdxInstance[clip_count].transform = transform_instance.fillers[i].getMatrix();
            clip_count += 1;
        }
        // store trims
        for (int i = 0; i < transform_instance.trims.size(); i++)
        {
            clipmapIdxInstance[clip_count].mesh_type = GeoClipMap::TRIM;
            clipmapIdxInstance[clip_count].transform = transform_instance.trims[i].getMatrix();
            clip_count += 1;
        }
        // store seams
        for (int i = 0; i < transform_instance.seams.size(); i++)
        {
            clipmapIdxInstance[clip_count].mesh_type = GeoClipMap::SEAM;
            clipmapIdxInstance[clip_count].transform = transform_instance.seams[i].getMatrix();
            clip_count += 1;
        }
    }

	float TerrainRenderHelper::GetTerrainHeight(glm::float2 localXZ)
	{
        auto _terrain_height_map = mInternalTerrain->terrain_heightmap_scratch;

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

	glm::float3 TerrainRenderHelper::GetTerrainNormal(glm::float2 localXZ)
	{
        auto _terrain_normal_map = mInternalTerrain->terrain_normalmap_scratch;

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
