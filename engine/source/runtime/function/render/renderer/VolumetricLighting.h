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

	float CornetteShanksPhasePartConstant(float anisotropy);
	// Ref: https://en.wikipedia.org/wiki/Close-packing_of_equal_spheres
	// The returned {x, y} coordinates (and all spheres) are all within the (-0.5, 0.5)^2 range.
	// The pattern has been rotated by 15 degrees to maximize the resolution along X and Y:
	// https://www.desmos.com/calculator/kcpfvltz7c
	void GetHexagonalClosePackedSpheres7(std::vector<glm::float2>& coords);

	class VolumetriLighting : public RenderPass
	{
	public:
		~VolumetriLighting() { destroy(); }

		void updateShaderVariableslVolumetrics(HLSL::ShaderVariablesVolumetric& cb, std::shared_ptr<RenderResource> render_resource);
		void prepareBuffer(std::shared_ptr<RenderResource> render_resource);


		void initialize(const VolumetriLightingInitInfo& init_info);

		void clearAndHeightFogVoxelizationPass(RHI::RenderGraph& graph, ClearPassInputStruct& passInput, ClearPassOutputStruct& passOutput);

		// This size is shared between all cameras to create the volumetric 3D textures
		static glm::uvec3 s_CurrentVolumetricBufferSize;

		int k_MaxVisibleLocalVolumetricFogCount = 1024;

		// DrawProceduralIndirect with an index buffer takes 5 parameters instead of 4
		const int k_VolumetricMaterialIndirectArgumentCount = 5;
		const int k_VolumetricMaterialIndirectArgumentByteSize = k_VolumetricMaterialIndirectArgumentCount * sizeof(glm::uint);
		const int k_VolumetricFogPriorityMaxValue = 1048576; // 2^20 because there are 20 bits in the volumetric fog sort key

		// Is the feature globally disabled?
		bool m_SupportVolumetrics = false;

		std::vector<glm::float4> m_PackedCoeffs;
		//ZonalHarmonicsL2 m_PhaseZH;
		std::vector<glm::float2> m_xySeq;

		// This is a sequence of 7 equidistant numbers from 1/14 to 13/14.
		// Each of them is the centroid of the interval of length 2/14.
		// They've been rearranged in a sequence of pairs {small, large}, s.t. (small + large) = 1.
		// That way, the running average position is close to 0.5.
		// | 6 | 2 | 4 | 1 | 5 | 3 | 7 |
		// |   |   |   | o |   |   |   |
		// |   | o |   | x |   |   |   |
		// |   | x |   | x |   | o |   |
		// |   | x | o | x |   | x |   |
		// |   | x | x | x | o | x |   |
		// | o | x | x | x | x | x |   |
		// | x | x | x | x | x | x | o |
		// | x | x | x | x | x | x | x |
		std::vector<float> m_zSeq = { 7.0f / 14.0f, 3.0f / 14.0f, 11.0f / 14.0f, 5.0f / 14.0f, 9.0f / 14.0f, 1.0f / 14.0f, 13.0f / 14.0f };

		std::vector<glm::float4x4> m_PixelCoordToViewDirWS;

	private:
		Shader VoxelizationCS;


	};


}

