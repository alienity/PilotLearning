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

        void InitTerrainRenderer(SceneTerrainRenderer*    sceneTerrainRenderer,
                                 SceneTerrainRenderer*    cachedSceneTerrainRenderer,
                                 InternalTerrain*         internalTerrain,
                                 InternalTerrainMaterial* internalTerrainMaterial,
                                 RenderResource*          renderResource);

        void InitTerrainBasicInfo(RenderResource* renderResource);
        void InitTerrainHeightAndNormalMap(RenderResource* renderResource);
        void InitTerrainBaseTextures(RenderResource* renderResource);
        void InitInternalTerrainClipmap(RenderResource* renderResource);

        uint32_t GetClipCount();
        void UpdateClipBuffer(HLSL::ClipmapIndexInstance* clipmapIdxInstance);
        void UpdateClipPatchScratchMesh(InternalScratchMesh* internalScratchMesh, GeoClipPatch geoPatch);
        void UpdateClipPatchMesh(RenderResource* renderResource, InternalMesh* internalMesh, InternalScratchMesh* internalScratchMesh);

        // update terrain clipmap mesh data
        void UpdateInternalTerrainClipmap(glm::float3 cameraPos, RenderResource* renderResource);

        float GetTerrainHeight(glm::float2 localXZ);
        glm::float3 GetTerrainNormal(glm::float2 localXZ);

        SceneTerrainRenderer* mSceneTerrainRenderer;
        SceneTerrainRenderer* mCachedSceneTerrainRenderer;
        InternalTerrain* mInternalTerrain;
        InternalTerrainMaterial* mInternalTerrainMaterial;

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
