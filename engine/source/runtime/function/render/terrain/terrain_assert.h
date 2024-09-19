
#ifndef TERRAIN_ASSERT_CLASS_H
#define TERRAIN_ASSERT_CLASS_H

#ifndef __FLT_MAX__
#define __FLT_MAX__ FLT_MAX
#endif

#include "runtime/core/math/moyu_math2.h"
#include "runtime/resource/basic_geometry/mesh_tools.h"

namespace MoYu
{
	class TerrainAssert
	{
	public:
		const uint32_t MAX_NODE_ID = 34124; //5x5+10x10+20x20+40x40+80x80+160x160 - 1
		const int MAX_LOD = 5;
		const int MAX_LOD_NODE_COUNT = 5; // MAX LOD下，世界由5x5个区块组成

		glm::float3 WorldSize = glm::float3(10240, 2048, 10240);

		std::shared_ptr<RHI::D3D12Texture> pAlbedoMap;
		std::shared_ptr<RHI::D3D12Texture> pHeightMap;
		std::shared_ptr<RHI::D3D12Texture> pNormalMap;

		//std::shared_ptr<RHI::D3D12Texture> pMinMaxHeightMap; // RG32
		//std::shared_ptr<RHI::D3D12Texture> pQuadTreeMap; // R16

		Geometry::TerrainPlane terrainPlane;
	};

} // namespace MoYu

#endif // TERRAIN_ASSERT_CLASS_H
