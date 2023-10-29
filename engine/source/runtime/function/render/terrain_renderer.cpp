#include "runtime/function/render/terrain_renderer.h"
#include "runtime/function/render/render_resource.h"
#include "runtime/core/math/moyu_math2.h"

namespace MoYu
{
	TerrainRenderer::TerrainRenderer()
	{

	}

	TerrainRenderer::~TerrainRenderer()
	{

	}

	void TerrainRenderer::InitTerrainRenderer()
	{
        local2WorldMatrix = glm::float4x4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1);
        world2LocalMatrix = glm::float4x4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1);

		terrainPageMips = 6;
		rootPageSize = 64;

		terrainSize = 1024;
        terrainMaxHeight = 1024;

        //InitTerrainNodePage();
	}

	void TerrainRenderer::InitTerrainNodePage()
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
	}

	void TerrainRenderer::GenerateTerrainNodes()
	{
        glm::uint pageSize = rootPageSize * (1 - glm::pow(4, terrainPageMips)) / (1 - 4);
        _TNodes.reserve(pageSize);

		TRect rect = TRect {0, 0, (float)terrainSize, (float)terrainSize};
        for (size_t i = rect.xMin; i < rect.xMin + rect.width; i += rootPageSize)
        {
            for (size_t j = rect.yMin; j < rect.yMin + rect.height; j += rootPageSize)
            {
                GenerateTerrainNodes(TRect {(float)i, (float)j, (float)rootPageSize, (float)rootPageSize}, terrainPageMips);
            }
        }
	}

	void TerrainRenderer::GenerateTerrainNodes(TRect rect, int mip)
	{
        //rect = r;
        //mip  = m;
        if (mip > 0)
        {
            GenerateTerrainNodes(TRect {rect.xMin, rect.yMin, rect.width * 0.5f, rect.height * 0.5f}, mip - 1);
            GenerateTerrainNodes(TRect {rect.xMin + rect.width * 0.5f, rect.yMin, rect.width * 0.5f, rect.height * 0.5f}, mip - 1);
            GenerateTerrainNodes(TRect {rect.xMin, rect.yMin + rect.height * 0.5f, rect.width * 0.5f, rect.height * 0.5f}, mip - 1);
            GenerateTerrainNodes(TRect {rect.xMin + rect.width * 0.5f, rect.yMin + rect.height * 0.5f, rect.width * 0.5f, rect.height * 0.5f}, mip - 1);
        }
        float _minHeight = 1000000.f;
        float _maxHeight = -1000000.f;
        if (mip == 0)
        {

        }
        else
        {
            for (size_t i = 0; i < 4; i++)
            {
                _minHeight = glm::min(_minHeight, children[i].node.minHeight);
                _maxHeight = glm::max(_maxHeight, children[i].node.maxHeight);
            }
        }
        node = TNode(r, m, _minHeight, _maxHeight);



	}

	float TerrainRenderer::GetTerrainHeight(glm::float2 localXZ)
	{
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

	glm::float3 TerrainRenderer::GetTerrainNormal(glm::float2 localXZ)
	{
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
