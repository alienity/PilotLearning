#pragma once

#include "runtime/function/render/render_pass.h"
#include "runtime/function/global/global_context.h"
#include "runtime/resource/config_manager/config_manager.h"
#include "runtime/function/render/rhi/hlsl_data_types.h"

namespace MoYu
{
	struct HeightFogVoxelizationPassData
	{
		glm::float4 resolution;
		int viewCount;
		HLSL::ShaderVariablesVolumetric volumetricCB;
		RHI::RgResourceHandle densityBuffer;
		RHI::RgResourceHandle volumetricAmbientProbeBuffer;

	};

	class VolumetriLighting
	{
	public:



		// This size is shared between all cameras to create the volumetric 3D textures
		static glm::uvec3 s_CurrentVolumetricBufferSize;

		Shader VoxelizationCS;



	};


}

