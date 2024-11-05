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

	enum class FogColorMode
	{
		/// <summary>Fog is a constant color.</summary>
		ConstantColor,
		/// <summary>Fog uses the current sky to determine its color.</summary>
		SkyColor,
	};

	enum class FogDenoisingMode
	{
		/// <summary>
		/// Use this mode to not filter the volumetric fog.
		/// </summary>
		None = 0,
		/// <summary>
		/// Use this mode to reproject data from previous frames to denoise the signal. This is effective for static lighting, but it can lead to severe ghosting artifacts for highly dynamic lighting.
		/// </summary>
		Reprojection = 1 << 0,
		/// <summary>
		/// Use this mode to reduce the aliasing patterns that can appear on the volumetric fog.
		/// </summary>
		Gaussian = 1 << 1,
		/// <summary>
		/// Use this mode to use both Reprojection and Gaussian filtering techniques. This produces high visual quality, but significantly increases the resource intensity of the effect.
		/// </summary>
		Both = Reprojection | Gaussian
	};

	struct ColorParameter
	{
		glm::float4 color;
		bool hdr = false; // Is this color HDR?
		bool showAlpha = true; // Should the alpha channel be editable in the editor?
		bool showEyeDropper = true; // Should the eye dropper be visible in the editor?
	};

	// Limit parameters for the fog quality
	const float minFogScreenResolutionPercentage = (1.0f / 16.0f) * 100;
	const float optimalFogScreenResolutionPercentage = (1.0f / 8.0f) * 100;
	const float maxFogScreenResolutionPercentage = 0.5f * 100;
	const int maxFogSliceCount = 512;

	struct FogVolume
	{
		// Enable fog.
		bool enabled = false;

		// Fog color mode.
		FogColorMode colorMode = FogColorMode::SkyColor;
		// Specifies the constant color of the fog.
		ColorParameter color = ColorParameter{ glm::float4(0.5f, 0.5f, 0.5f, 1.0f), true, false, true };
		// Specifies the tint of the fog when using Sky Color.
		ColorParameter tint = ColorParameter{ glm::float4(1.0f, 1.0f, 1.0f, 1.0f), true, false, true };
		// Sets the maximum fog distance HDRP uses when it shades the skybox or the Far Clipping Plane of the Camera.
		float maxFogDistance = 5000.0f;
		// Controls the maximum mip map HDRP uses for mip fog (0 is the lowest mip and 1 is the highest mip).
		float mipFogMaxMip = 0.5f;
		// Sets the distance at which HDRP uses the minimum mip image of the blurred sky texture as the fog color.
		float mipFogNear = 0.0f;
		// Sets the distance at which HDRP uses the maximum mip image of the blurred sky texture as the fog color.
		float mipFogFar = 1000.0f;

		// Height Fog
		// Height fog base height.
		float baseHeight = 0.0f;
		// Height fog maximum height.
		float maximumHeight = 50.0f;
		// Fog Attenuation Distance
		float meanFreePath = 400.0f;

		// Optional Volumetric Fog
		// Enable volumetric fog.
		bool enableVolumetricFog = true;
		// Common Fog Parameters (Exponential/Volumetric)
		// Stores the fog albedo. This defines the color of the fog.
		ColorParameter albedo = ColorParameter{ glm::float4(1.0f, 1.0f, 1.0f, 1.0f), false, true, true };
		/// <summary>Multiplier for global illumination (APV or ambient probe).</summary>
		float globalLightProbeDimmer = float(1.0f);
		/// <summary>Sets the distance (in meters) from the Camera's Near Clipping Plane to the back of the Camera's volumetric lighting buffer. The lower the distance is, the higher the fog quality is.</summary>
		float depthExtent = float(64.0f);

		/// <summary>Controls which denoising technique to use for the volumetric effect.</summary>
		/// <remarks>Reprojection mode is effective for static lighting but can lead to severe ghosting artifacts with highly dynamic lighting. Gaussian mode is effective with dynamic lighting. You can also use both modes together which produces high-quality results, but increases the resource intensity of processing the effect.</remarks>
		FogDenoisingMode denoisingMode = FogDenoisingMode::Gaussian;

		// Advanced parameters
		/// <summary>Controls the angular distribution of scattered light. 0 is isotropic, 1 is forward scattering, and -1 is backward scattering.</summary>
		float anisotropy = 0.0f;

		/// <summary>Controls the distribution of slices along the Camera's focal axis. 0 is exponential distribution and 1 is linear distribution.</summary>
		float sliceDistributionUniformity = 0.75f;

		/// <summary>Controls how much the multiple-scattering will affect the scene. Directly controls the amount of blur depending on the fog density.</summary>
		float multipleScatteringIntensity = 0.0f;

		/// <summary>Stores the resolution of the volumetric buffer (3D texture) along the x-axis and y-axis relative to the resolution of the screen.</summary>
		float screenResolutionPercentage = optimalFogScreenResolutionPercentage;
		/// <summary>Number of slices of the volumetric buffer (3D texture) along the camera's focal axis.</summary>
		int volumeSliceCount = 64;

		/// <summary>Defines the performance to quality ratio of the volumetric fog. A value of 0 being the least resource-intensive and a value of 1 being the highest quality.</summary>
		float m_VolumetricFogBudget = 0.25f;

		/// <summary>Controls how Unity shares resources between Screen (XY) and Depth (Z) resolutions.</summary>
		float m_ResolutionDepthRatio = 0.5f;

		/// <summary>Indicates whether Unity includes or excludes non-directional light types when it evaluates the volumetric fog. Including non-directional lights increases the resource intensity of the effect.</summary>
		bool directionalLightsOnly = false;
	};

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


	struct LocalVolumetricFogArtistParameters
	{
		/// <summary>Single scattering albedo: [0, 1]. Alpha is ignored.</summary>
		glm::vec3 albedo;
		/// <summary>Mean free path, in meters: [1, inf].</summary>
		float meanFreePath; // Should be chromatic - this is an optimization!

		/// <summary>
		/// Specifies how the fog in the volume will interact with the fog.
		/// </summary>
		LocalVolumetricFogBlendingMode blendingMode;

		/// <summary>
		/// Rendering priority of the volume, higher priority will be rendered first.
		/// </summary>
		int priority;

		/// <summary>Anisotropy of the phase function: [-1, 1]. Positive values result in forward scattering, and negative values - in backward scattering.</summary>
		float anisotropy;   // . Not currently available for Local Volumetric Fog

		/// <summary>Texture containing density values.</summary>
		std::shared_ptr<RHI::D3D12Texture> volumeMask;
		/// <summary>Scrolling speed of the density texture.</summary>
		glm::vec3 textureScrollingSpeed;
		/// <summary>Tiling rate of the density texture.</summary>
		glm::vec3 textureTiling;

		/// <summary>Edge fade factor along the positive X, Y and Z axes.</summary>
		glm::vec3 positiveFade;
		/// <summary>Edge fade factor along the negative X, Y and Z axes.</summary>
		glm::vec3 negativeFade;

		/// <summary>Dimensions of the volume.</summary>
		glm::vec3 size;
		/// <summary>Inverts the fade gradient.</summary>
		bool invertFade;

		/// <summary>Distance at which density fading starts.</summary>
		float distanceFadeStart;
		/// <summary>Distance at which density fading ends.</summary>
		float distanceFadeEnd;
		/// <summary>Allows translation of the tiling density texture.</summary>
		glm::vec3 textureOffset;

		/// <summary>When Blend Distance is above 0, controls which kind of falloff is applied to the transition area.</summary>
		LocalVolumetricFogFalloffMode falloffMode;

		///// <summary>Minimum fog distance you can set in the meanFreePath parameter</summary>
		//const float kMinFogDistance = 0.05f;

		/// <summary>Constructor.</summary>
		/// <param name="color">Single scattering albedo.</param>
		/// <param name="_meanFreePath">Mean free path.</param>
		/// <param name="_anisotropy">Anisotropy.</param>
		LocalVolumetricFogArtistParameters(glm::vec3 color, float _meanFreePath, float _anisotropy);

		void Update(float time);

		HLSL::LocalVolumetricFogEngineData ConvertToEngineData();
	};

	// 初始化参数变量
	void InitializeFogConstants();

	const FogVolume& GetFogVolume();

	HLSL::LocalVolumetricFogEngineData GetNeutralValues();

	float CornetteShanksPhasePartConstant(float anisotropy);

	//// Ref: https://en.wikipedia.org/wiki/Close-packing_of_equal_spheres
	//// The returned {x, y} coordinates (and all spheres) are all within the (-0.5, 0.5)^2 range.
	//// The pattern has been rotated by 15 degrees to maximize the resolution along X and Y:
	//// https://www.desmos.com/calculator/kcpfvltz7c
	void GetHexagonalClosePackedSpheres7(glm::float2(&coords)[7]);

	void GetXYSeq(glm::vec2(&xySeq)[7]);
	void GetZSeq(float(&zSeq)[7]);

	glm::uint VolumetricFrameIndex(int frameIndex);
	void ComputeVolumetricFogSliceCountAndScreenFraction(const FogVolume& fog, int& sliceCount, float& screenFraction);
	glm::ivec3 ComputeVolumetricViewportSize(const FogVolume& fog, int viewportWidth, int viewportHeight, float& voxelSize);
	VBufferParameters ComputeVolumetricBufferParameters(float width, float height, float nearZ, float farZ, float fovy, const FogVolume& fog);

	float ScaleHeightFromLayerDepth(float d);


}

