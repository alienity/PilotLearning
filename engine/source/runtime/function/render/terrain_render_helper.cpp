#include "runtime/function/render/terrain_render_helper.h"
#include "runtime/function/render/render_common.h"
#include "runtime/core/math/moyu_math2.h"
#include "runtime/core/log/log_system.h"

namespace MoYu
{
	TerrainRenderHelper::TerrainRenderHelper()
	{

	}

	TerrainRenderHelper::~TerrainRenderHelper()
	{

	}

	void TerrainRenderHelper::InitTerrainRenderer(InternalTerrain* internalTerrain)
	{
        mInternalTerrain = internalTerrain;

        InitTerrainNodePage();
	}

	void TerrainRenderHelper::InitTerrainNodePage()
	{
        /*
		if (_tPageRoot == nullptr)
		{
            float perSize = 64;
            TRect rect = TRect {0, 0, (float)terrainSize, (float)terrainSize};
			_tPageRoot = std::make_shared<TNodePage>(rect);
            for (size_t i = rect.xMin; i < rect.xMin + rect.width; i += perSize)
            {
                for (size_t j = rect.yMin; j < rect.yMin + rect.height; j += perSize)
                {
                    _tPageRoot->children.push_back(TNodePage(TRect {(float)i, (float)j, perSize, perSize}, terrainPageLevel));
                }
            }
		}
        */

        GenerateTerrainNodes();
	}

    /*
	void TerrainRenderHelper::GenerateTerrainNodes()
	{
        uint32_t rootPageCount = terrainSize / rootPageSize;

        uint32_t pageSize = rootPageCount * (1 - glm::pow(4, terrainPageMips)) / (1 - 4);
        _TNodes.reserve(pageSize);

        std::queue<TNode> _TNodeQueue;

		TRect rect = TRect {0, 0, (float)terrainSize, (float)terrainSize};
        for (size_t j = rect.yMin, int yIndex = 0; j < rect.yMin + rect.height; j += rootPageSize, yIndex += 1)
        {
            for (size_t i = rect.xMin, int xIndex = 0; i < rect.xMin + rect.width; i += rootPageSize, xIndex += 1)
            {
                uint32_t _patchIndex = yIndex * rootPageCount + xIndex;
                TNode _TNode = GenerateTerrainNodes(TRect {(float)i, (float)j, (float)rootPageSize, (float)rootPageSize}, _patchIndex, 0);
                _TNodeQueue.push(_TNode);
            }
        }

        while (!_TNodeQueue.empty())
        {
            TNode _TNode = _TNodeQueue.front();
            _TNodeQueue.pop();

            uint32_t _nodePatchIndex = _TNode.index;
            uint32_t _nodeMipIndex = _TNode.mip;

            uint32_t _arrayOffset = TNode::InArrayOffset(rootPageCount, _nodeMipIndex, _nodePatchIndex);
            _TNodes[_arrayOffset] = _TNode;

            TRect _rect = _TNode.rect;

            if (_nodeMipIndex < terrainPageMips - 1)
            {
                TNode _TNode00 = GenerateTerrainNodes(TRect {_rect.xMin, _rect.yMin, _rect.width * 0.5f, _rect.height * 0.5f},
                    _nodePatchIndex << 2 + 0, _nodeMipIndex + 1);
                TNode _TNode10 = GenerateTerrainNodes(TRect {_rect.xMin + _rect.width * 0.5f, _rect.yMin, _rect.width * 0.5f, _rect.height * 0.5f},
                    _nodePatchIndex << 2 + 1, _nodeMipIndex + 1);
                TNode _TNode01 = GenerateTerrainNodes(TRect {_rect.xMin, _rect.yMin + _rect.height * 0.5f, _rect.width * 0.5f, _rect.height * 0.5f},
                    _nodePatchIndex << 2 + 2, _nodeMipIndex + 1);
                TNode _TNode11 = GenerateTerrainNodes(TRect {_rect.xMin + _rect.width * 0.5f, _rect.yMin + _rect.height * 0.5f, _rect.width * 0.5f, _rect.height * 0.5f},
                    _nodePatchIndex << 2 + 3, _nodeMipIndex + 1);

                _TNodeQueue.push(_TNode00);
                _TNodeQueue.push(_TNode10);
                _TNodeQueue.push(_TNode01);
                _TNodeQueue.push(_TNode11);
            }
        }
	}
    */

    void TerrainRenderHelper::GenerateTerrainNodes()
    {
        int terrainSize = mInternalTerrain->terrain_size.x;
        int terrain_root_patch_number = mInternalTerrain->terrain_root_patch_number;

        float _rootLevelNodeWidth = terrainSize / (float)MoYu::TNode::SpecificMipLevelWidth(0);

        uint32_t _totalNodeCount = TNode::ToMipLevelNodeCount(TerrainMipLevel);
        _TNodes.resize(_totalNodeCount);

        int yIndex = 0;
        int xIndex = 0;
        TRect rect = TRect {0, 0, (float)terrainSize, (float)terrainSize};
        for (size_t j = rect.yMin; j < rect.yMin + rect.height; j += _rootLevelNodeWidth, yIndex += 1)
        {
            for (size_t i = rect.xMin; i < rect.xMin + rect.width; i += _rootLevelNodeWidth, xIndex += 1)
            {
                uint32_t _patchIndex = yIndex * _rootLevelNodeWidth + xIndex;
                TNode _tNode = GenerateTerrainNodes(TRect {(float)i, (float)j, (float)_rootLevelNodeWidth, (float)_rootLevelNodeWidth}, _patchIndex, 0);
                uint32_t _arrayOffset = TNode::SpecificMipLevelNodeInArrayOffset(0, _patchIndex);
                _TNodes[_arrayOffset] = _tNode;
            }
        }

        std::wstring _logStr = fmt::format(L"_TNodes count = {}", _TNodes.size());
        MoYu::LogSystem::Instance()->log(MoYu::LogSystem::LogLevel::Info, _logStr);
    }

	TNode TerrainRenderHelper::GenerateTerrainNodes(TRect rect, uint32_t pathcIndex, int mip)
	{

        float _minHeight = std::numeric_limits<float>::max();
        float _maxHeight = -std::numeric_limits<float>::max();

        if (mip < TerrainMipLevel - 1)
        {
            TNode _TNode00 = GenerateTerrainNodes(TRect {rect.xMin, rect.yMin, rect.width * 0.5f, rect.height * 0.5f}, pathcIndex << 2 + 0, mip + 1);
            TNode _TNode01 = GenerateTerrainNodes(TRect {rect.xMin + rect.width * 0.5f, rect.yMin, rect.width * 0.5f, rect.height * 0.5f}, pathcIndex << 2 + 1, mip + 1);
            TNode _TNode10 = GenerateTerrainNodes(TRect {rect.xMin, rect.yMin + rect.height * 0.5f, rect.width * 0.5f, rect.height * 0.5f}, pathcIndex << 2 + 2, mip + 1);
            TNode _TNode11 = GenerateTerrainNodes(TRect {rect.xMin + rect.width * 0.5f, rect.yMin + rect.height * 0.5f, rect.width * 0.5f, rect.height * 0.5f}, pathcIndex << 2 + 3, mip + 1);

            _TNodes[MoYu::TNode::SpecificMipLevelNodeInArrayOffset(_TNode00.mip, _TNode00.index)] = _TNode00;
            _TNodes[MoYu::TNode::SpecificMipLevelNodeInArrayOffset(_TNode01.mip, _TNode01.index)] = _TNode01;
            _TNodes[MoYu::TNode::SpecificMipLevelNodeInArrayOffset(_TNode10.mip, _TNode10.index)] = _TNode10;
            _TNodes[MoYu::TNode::SpecificMipLevelNodeInArrayOffset(_TNode11.mip, _TNode11.index)] = _TNode11;

            _minHeight = glm::min(glm::min(_TNode00.minHeight, _TNode01.minHeight), glm::min(_TNode10.minHeight, _TNode11.minHeight));
            _maxHeight = glm::max(glm::max(_TNode00.maxHeight, _TNode01.maxHeight), glm::max(_TNode10.maxHeight, _TNode11.maxHeight));
        }
        else if (mip == TerrainMipLevel - 1)
        {
            for (float i = rect.xMin; i < rect.xMin + rect.width; i += 1.0f)
            {
                for (float j = rect.yMin; j < rect.yMin + rect.height; j += 1.0f)
                {
                    float _curHeight = GetTerrainHeight(glm::float2(i, j));
                    _minHeight = glm::min(_minHeight, _curHeight);
                    _maxHeight = glm::max(_maxHeight, _curHeight);
                }
            }
        }
        
        TNode _TNode     = {};
        _TNode.mip       = mip;
        _TNode.index     = pathcIndex;
        _TNode.rect      = rect;
        _TNode.minHeight = _maxHeight;
        _TNode.maxHeight = _minHeight;

        return _TNode;
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


}
