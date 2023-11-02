#pragma once
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


    // mip0层级的节点边缘数量位宽
    #define RootLevelNodeWidthBit 3
    // mip1之后层级的节点边缘数量位宽
    #define SubLevelNodeWidthBit 1

    #define RootLevelNodeCountBit 6
    #define SubLevelNodeCountBit 2


    struct TNode
    {
        int mip;
        int index;
        TRect rect;
        float minHeight;
        float maxHeight;
        int neighbor;

        TNode() = default;

        static uint32_t SpecificMipLevelWidth(uint32_t curMipLevel)
        {
            uint32_t totalNodeCountWidth = 1 << RootLevelNodeWidthBit;
            for (size_t i = 1; i <= curMipLevel; i++)
            {
                totalNodeCountWidth = totalNodeCountWidth << SubLevelNodeWidthBit;
            }
            return totalNodeCountWidth;
        }

        // 特定MipLevel的Node节点数量
        static uint32_t SpecificMipLevelNodeCount(uint32_t curMipLevel)
        {
            uint32_t totalNodeCount = 1 << RootLevelNodeCountBit;
            for (size_t i = 1; i <= curMipLevel; i++)
            {
                totalNodeCount = totalNodeCount << SubLevelNodeCountBit;
            }
            return totalNodeCount;
        }

        // 到达指定MipLevel的Node节点数量
        static uint32_t ToMipLevelNodeCount(uint32_t curMipLevel)
        {
            uint32_t totalNodeCount = 0;
            for (size_t i = 0; i <= curMipLevel; i++)
            {
                totalNodeCount += SpecificMipLevelNodeCount(i);
            }
            return totalNodeCount;
        }

        // 当前MipLevel的Index节点在全局Array中的位置
        static uint32_t SpecificMipLevelNodeInArrayOffset(uint32_t curMipLevel, uint32_t curLevelIndex)
        {
            uint32_t _parentMipLevelOffset = ToMipLevelNodeCount(curMipLevel - 1);
            uint32_t _curNodeOffset = _parentMipLevelOffset + curLevelIndex;
            return _curNodeOffset;
        }

    };

    #define TerrainMipLevel 6

    class TerrainRenderHelper
    {
    public:
        TerrainRenderHelper();
        ~TerrainRenderHelper();

        void InitTerrainRenderer(InternalTerrain* internalTerrain);
        void InitTerrainNodePage();

        void GenerateTerrainNodes();
        TNode GenerateTerrainNodes(TRect rect, uint32_t patchIndex, int mip);

        float GetTerrainHeight(glm::float2 localXZ);
        glm::float3 GetTerrainNormal(glm::float2 localXZ);

        std::vector<TNode> _TNodes; // 线性记录所有的图块

        InternalTerrain* mInternalTerrain;
    };
} // namespace MoYu
