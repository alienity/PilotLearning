#pragma once

#include "runtime/function/render/render_pass.h"
#include "runtime/function/global/global_context.h"
#include "runtime/resource/config_manager/config_manager.h"
#include "runtime/function/render/rhi/hlsl_data_types.h"
#include "runtime/function/render/utility/HDUtils.h"

namespace MoYu
{
	enum class LocalVolumetricFogFalloffMode
	{
		/// <summary>Fade using a linear function.</summary>
		Linear,
		/// <summary>Fade using an exponential function.</summary>
		Exponential,
	};

	enum class LocalVolumetricFogBlendingMode
	{
		/// <summary>Replace the current fog, it is similar to disabling the blending.</summary>
		Overwrite = 0,
		/// <summary>Additively blend fog volumes. This is the default behavior.</summary>
		Additive = 1,
		/// <summary>Multiply the fog values when doing the blending. This is useful to make the fog density relative to other fog volumes.</summary>
		Multiply = 2,
		/// <summary>Performs a minimum operation when blending the volumes.</summary>
		Min = 3,
		/// <summary>Performs a maximum operation when blending the volumes.</summary>
		Max = 4,
	};

	HLSL::LocalVolumetricFogEngineData GetNeutralValues();

	struct HeightFogVoxelizationPassData
	{
		glm::float4 resolution;
		HLSL::ShaderVariablesVolumetric volumetricCB;
	};

	struct VolumeRenderingUtils
	{
		static float MeanFreePathFromExtinction(float extinction);
		static float ExtinctionFromMeanFreePath(float meanFreePath);
		static glm::vec3 AbsorptionFromExtinctionAndScattering(float extinction, glm::vec3 scattering);
		static glm::vec3 ScatteringFromExtinctionAndAlbedo(float extinction, glm::vec3 albedo);
		static glm::vec3 AlbedoFromMeanFreePathAndScattering(float meanFreePath, glm::vec3 scattering);
	};

	struct VBufferParameters
	{
		glm::ivec3 viewportSize;
		float voxelSize;
		glm::vec4 depthEncodingParams;
		glm::vec4 depthDecodingParams;

		VBufferParameters() {};

		VBufferParameters(glm::ivec3 viewportSize, float depthExtent, float camNear,
			float camFar, float camVFoV, float sliceDistributionUniformity, float voxelSize);
		glm::vec3 ComputeViewportScale(glm::ivec3 bufferSize);
		glm::vec3 ComputeViewportLimit(glm::ivec3 bufferSize);
		float ComputeLastSliceDistance(glm::uint sliceCount);
		float EncodeLogarithmicDepthGeneralized(float z, glm::vec4 encodingParams);
		float DecodeLogarithmicDepthGeneralized(float d, glm::vec4 decodingParams);
		int ComputeSliceIndexFromDistance(float distance, int maxSliceCount);
		static glm::vec4 ComputeLogarithmicDepthEncodingParams(float nearPlane, float farPlane, float c);
		static glm::vec4 ComputeLogarithmicDepthDecodingParams(float nearPlane, float farPlane, float c);
	};

	/*
	class VolumeRenderPipeline
	{
	public:
		glm::vec4 m_PackedCoeffs[7];
		//ZonalHarmonicsL2 m_PhaseZH;
		glm::vec2 m_xySeq[7];

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
		float m_zSeq[7] = { 7.0f / 14.0f, 3.0f / 14.0f, 11.0f / 14.0f, 5.0f / 14.0f, 9.0f / 14.0f, 1.0f / 14.0f, 13.0f / 14.0f };
	};
	*/

	float CornetteShanksPhasePartConstant(float anisotropy);

	//// Ref: https://en.wikipedia.org/wiki/Close-packing_of_equal_spheres
	//// The returned {x, y} coordinates (and all spheres) are all within the (-0.5, 0.5)^2 range.
	//// The pattern has been rotated by 15 degrees to maximize the resolution along X and Y:
	//// https://www.desmos.com/calculator/kcpfvltz7c
	void GetHexagonalClosePackedSpheres7(glm::float2(&coords)[7]);

	glm::uint VolumetricFrameIndex(int frameIndex);
	void ComputeVolumetricFogSliceCountAndScreenFraction(EngineConfig::FogConfig fog, int& sliceCount, float& screenFraction);
	glm::ivec3 ComputeVolumetricViewportSize(int viewportWidth, int viewportHeight, float voxelSize);
	VBufferParameters ComputeVolumetricBufferParameters(float width, float height, float nearZ, float farZ, float fovy, EngineConfig::FogConfig fog);


}

