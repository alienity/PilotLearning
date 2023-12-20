#pragma once
#include "runtime/function/render/render_common.h"
#include "runtime/function/render/terrain/terrain_3d.h"
#include <vector>

namespace MoYu
{
    class RenderResource;

    class TerrainRenderHelper
    {
    public:
        TerrainRenderHelper();
        ~TerrainRenderHelper();

        void InitTerrainRenderer(SceneTerrainRenderer* sceneTerrainRenderer, SceneTerrainRenderer* cachedSceneTerrainRenderer, 
            InternalTerrain* internalTerrain, InternalTerrainMaterial* internalTerrainMaterial, RenderResource* renderResource);

        // update terrain clipmap mesh data
        void UpdateInternalTerrainClipmap(InternalTerrain* internalTerrain,
                                          glm::float3      cameraPos,
                                          RenderResource*  renderResource);

        bool updateInternalTerrainRenderer(RenderResource*          renderResource,
                                           SceneTerrainRenderer     scene_terrain_renderer,
                                           SceneTerrainRenderer&    cached_terrain_renderer,
                                           InternalTerrainRenderer& internal_terrain_renderer,
                                           glm::float4x4            model_matrix,
                                           glm::float4x4            model_matrix_inv,
                                           glm::float4x4            prev_model_matrix,
                                           glm::float4x4            prev_model_matrix_inv,
                                           bool                     has_initialized = false);

        float       GetTerrainHeight(InternalTerrain* internalTerrain, glm::float2 localXZ);
        glm::float3 GetTerrainNormal(InternalTerrain* internalTerrain, glm::float2 localXZ);

    private:
        void InitTerrainBasicInfo(SceneTerrainRenderer* sceneTerrainRenderer, InternalTerrain* internalTerrain);

        void InitTerrainHeightAndNormalMap(SceneTerrainRenderer* sceneTerrainRenderer,
                                           SceneTerrainRenderer* cachedSceneTerrainRenderer,
                                           InternalTerrain*      internalTerrain,
                                           RenderResource*       renderResource);
        void InitInternalTerrainClipmap(InternalTerrain* internalTerrain, RenderResource* renderResource);
        void InitTerrainBaseTextures(SceneTerrainRenderer*    sceneTerrainRenderer,
                                     SceneTerrainRenderer*    cachedSceneTerrainRenderer,
                                     InternalTerrainMaterial* internalTerrainMaterial,
                                     RenderResource*          renderResource);

        uint32_t GetClipCount();
        void UpdateClipBuffer(HLSL::ClipmapIndexInstance* clipmapIdxInstance);
        void UpdateClipPatchScratchMesh(InternalScratchMesh* internalScratchMesh, GeoClipPatch geoPatch);
        void UpdateClipPatchMesh(RenderResource* renderResource, InternalMesh* internalMesh, InternalScratchMesh* internalScratchMesh);

    private:
        glm::float2 last_cam_xz;

        bool mIsHeightmapInit;
        bool mIsNaterialInit;

        std::shared_ptr<Terrain3D> mTerrain3D;
        InternalTerrainBaseTexture mInternalBaseTextures[2];

        std::shared_ptr<MoYu::MoYuScratchImage> terrain_heightmap_scratch;
        std::shared_ptr<MoYu::MoYuScratchImage> terrain_normalmap_scratch;

        std::shared_ptr<RHI::D3D12Texture> terrain_heightmap;
        std::shared_ptr<RHI::D3D12Texture> terrain_normalmap;
    };


} // namespace MoYu
