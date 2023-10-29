#pragma once

#include "runtime/function/framework/object/object_id_allocator.h"
#include "runtime/function/render/render_common.h"
#include "runtime/function/render/render_guid_allocator.h"
#include "runtime/resource/basic_geometry/mesh_tools.h"

#include <vector>

namespace MoYu
{
    struct TRect
    {
        float xMin;
        float yMin;
        float width;
        float height;

        bool Contains(glm::float2 center)
        {
            bool isInCenter = center.x > xMin;
            isInCenter &= center.y > yMin;
            isInCenter &= center.x < xMin + width;
            isInCenter &= center.y < yMin + height;
            return isInCenter;
        }

        glm ::float2 GetCenter()
        {
            return glm::float2(xMin + width * 0.5f, yMin + height * 0.5f);
        }
    };

    struct TNode
    {
        TRect rect;
        int mip;
        int index;
        float minHeight;
        float maxHeight;
        int neighbor;

        TNode() = default;

        TNode(TRect r, int m, float minHeight, float maxHeight)
        {
            rect     = r;
            mip      = m;
            neighbor = 0;
        }
    };

    class TNodePage
    {
    public:
        int mip;
        TRect rect;
        TNode node;
        std::vector<TNodePage> children;

        TNodePage() = default;

        TNodePage(TRect r)
        {
            rect = r;
            mip  = -1;
        }

        TNodePage(TRect r, int m)
        {
            rect = r;
            mip  = m;
            if (mip > 0)
            {
                children.reserve(4);
                children[0] = TNodePage(TRect {r.xMin, r.yMin, r.width * 0.5f, r.height * 0.5f}, m - 1);
                children[1] = TNodePage(TRect {r.xMin + r.width * 0.5f, r.yMin, r.width * 0.5f, r.height * 0.5f}, m - 1);
                children[2] = TNodePage(TRect {r.xMin, r.yMin + r.height * 0.5f, r.width * 0.5f, r.height * 0.5f}, m - 1);
                children[3] = TNodePage(TRect {r.xMin + r.width * 0.5f, r.yMin + r.height * 0.5f, r.width * 0.5f, r.height * 0.5f}, m - 1);
            }
            float _minHeight = 1000000.f;
            float _maxHeight = -1000000.f;
            for (size_t i = 0; i < children.size(); i++)
            {
                _minHeight = glm::min(_minHeight, children[i].node.minHeight);
                _maxHeight = glm::max(_maxHeight, children[i].node.maxHeight);
            }
            node = TNode(r, m, _minHeight, _maxHeight);
        }

    };

    class TerrainRenderer
    {
    public:
        TerrainRenderer();
        ~TerrainRenderer();

        void InitTerrainRenderer();
        void InitTerrainNodePage();

        void GenerateTerrainNodes();
        void GenerateTerrainNodes(TRect rect, int mip);

        float GetTerrainHeight(glm::float2 localXZ);
        glm::float3 GetTerrainNormal(glm::float2 localXZ);

        glm::float4x4 local2WorldMatrix;
        glm::float4x4 world2LocalMatrix;

        glm::uint terrainSize;      // 1024*1024
        glm::uint terrainMaxHeight; // 1024
        glm::uint rootPageSize; // 64
        glm::uint terrainPageMips; // 6

        std::vector<TNode> _TNodes; // 线性记录所有的图块

        std::shared_ptr<MoYu::MoYuScratchImage> _terrain_height_map;
        std::shared_ptr<MoYu::MoYuScratchImage> _terrain_normal_map;

        TerrainConfigs _terrainConfig;
    };
} // namespace MoYu
