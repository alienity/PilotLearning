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

	struct ClearPassInputStruct
	{

	};

	struct ClearPassOutputStruct
	{

	};

	struct VolumetriLightingInitInfo : public RenderPassInitInfo
	{
		RHI::RgTextureDesc m_ColorTexDesc;
		ShaderCompiler* m_ShaderCompiler;
		std::filesystem::path m_ShaderRootPath;
	};

	class VolumetriLighting : public RenderPass
	{
	public:
		~VolumetriLighting() { destroy(); }

		void prepareBuffer(std::shared_ptr<RenderResource> render_resource);

		void initialize(const VolumetriLightingInitInfo& init_info);

		void ClearAndHeightFogVoxelizationPass(RHI::RenderGraph& graph, ClearPassInputStruct& passInput, ClearPassOutputStruct& passOutput);

		// This size is shared between all cameras to create the volumetric 3D textures
		static glm::uvec3 s_CurrentVolumetricBufferSize;

		Shader VoxelizationCS;



	};


}

