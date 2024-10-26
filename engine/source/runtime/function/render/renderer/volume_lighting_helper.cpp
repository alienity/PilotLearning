#include "volume_lighting_helper.h"

namespace MoYu
{

	HLSL::LocalVolumetricFogEngineData GetNeutralValues()
	{
		HLSL::LocalVolumetricFogEngineData data;

		data.scattering = glm::vec3(0);
		data.textureTiling = glm::vec3(1);
		data.textureScroll = glm::vec3(0);
		data.rcpPosFaceFade = glm::vec3(FLT_MAX, FLT_MAX, FLT_MAX);
		data.rcpNegFaceFade = glm::vec3(FLT_MAX, FLT_MAX, FLT_MAX);
		data.invertFade = 0;
		data.rcpDistFadeLen = 0;
		data.endTimesRcpDistFadeLen = 1;
		data.falloffMode = (int)LocalVolumetricFogFalloffMode::Linear;
		data.blendingMode = (int)LocalVolumetricFogBlendingMode::Additive;

		return data;
	}

	float VolumeRenderingUtils::MeanFreePathFromExtinction(float extinction)
	{
		return 1.0f / extinction;
	}

	float VolumeRenderingUtils::ExtinctionFromMeanFreePath(float meanFreePath)
	{
		return 1.0f / meanFreePath;
	}

	glm::vec3 VolumeRenderingUtils::AbsorptionFromExtinctionAndScattering(float extinction, glm::vec3 scattering)
	{
		return glm::vec3(extinction, extinction, extinction) - scattering;
	}

	glm::vec3 VolumeRenderingUtils::ScatteringFromExtinctionAndAlbedo(float extinction, glm::vec3 albedo)
	{
		return extinction * albedo;
	}

	glm::vec3 VolumeRenderingUtils::AlbedoFromMeanFreePathAndScattering(float meanFreePath, glm::vec3 scattering)
	{
		return meanFreePath * scattering;
	}

    VBufferParameters::VBufferParameters(glm::ivec3 viewportSize, float depthExtent, float camNear,
        float camFar, float camVFoV, float sliceDistributionUniformity, float voxelSize)
    {
        this->viewportSize = viewportSize;
        this->voxelSize = voxelSize;

        // The V-Buffer is sphere-capped, while the camera frustum is not.
        // We always start from the near plane of the camera.

        float aspectRatio = viewportSize.x / (float)viewportSize.y;
        float farPlaneHeight = 2.0f * glm::tan(0.5f * camVFoV) * camFar;
        float farPlaneWidth = farPlaneHeight * aspectRatio;
        float farPlaneMaxDim = glm::max(farPlaneWidth, farPlaneHeight);
        float farPlaneDist = glm::sqrt(camFar * camFar + 0.25f * farPlaneMaxDim * farPlaneMaxDim);

        float nearDist = camNear;
        float farDist = glm::min(nearDist + depthExtent, farPlaneDist);

        float c = 2 - 2 * sliceDistributionUniformity; // remap [0, 1] -> [2, 0]
        c = glm::max(c, 0.001f);                // Avoid NaNs

        depthEncodingParams = ComputeLogarithmicDepthEncodingParams(nearDist, farDist, c);
        depthDecodingParams = ComputeLogarithmicDepthDecodingParams(nearDist, farDist, c);
    }

    glm::vec3 VBufferParameters::ComputeViewportScale(glm::ivec3 bufferSize)
    {
        return glm::vec3(
            HDUtils::ComputeViewportScale(viewportSize.x, bufferSize.x),
            HDUtils::ComputeViewportScale(viewportSize.y, bufferSize.y),
            HDUtils::ComputeViewportScale(viewportSize.z, bufferSize.z));
    }

    glm::vec3 VBufferParameters::ComputeViewportLimit(glm::ivec3 bufferSize)
    {
        return glm::vec3(
            HDUtils::ComputeViewportLimit(viewportSize.x, bufferSize.x),
            HDUtils::ComputeViewportLimit(viewportSize.y, bufferSize.y),
            HDUtils::ComputeViewportLimit(viewportSize.z, bufferSize.z));
    }

    float VBufferParameters::ComputeLastSliceDistance(glm::uint sliceCount)
    {
        float d = 1.0f - 0.5f / sliceCount;
        float ln2 = 0.69314718f;

        // DecodeLogarithmicDepthGeneralized(1 - 0.5 / sliceCount)
        return depthDecodingParams.x * glm::exp(ln2 * d * depthDecodingParams.y) + depthDecodingParams.z;
    }

    float VBufferParameters::EncodeLogarithmicDepthGeneralized(float z, glm::vec4 encodingParams)
    {
        return encodingParams.x + encodingParams.y * glm::log2(glm::max(0.0f, z - encodingParams.z));
    }

    float VBufferParameters::DecodeLogarithmicDepthGeneralized(float d, glm::vec4 decodingParams)
    {
        return decodingParams.x * glm::pow(2, d * decodingParams.y) + decodingParams.z;
    }

    int VBufferParameters::ComputeSliceIndexFromDistance(float distance, int maxSliceCount)
    {
        // Avoid out of bounds access
        distance = glm::clamp(distance, 0.0f, ComputeLastSliceDistance((glm::uint)maxSliceCount));

        float vBufferNearPlane = DecodeLogarithmicDepthGeneralized(0, depthDecodingParams);

        // float dt = (distance - vBufferNearPlane) * 2;
        float dt = distance + vBufferNearPlane;
        float e1 = EncodeLogarithmicDepthGeneralized(dt, depthEncodingParams);
        float rcpSliceCount = 1.0f / (float)maxSliceCount;

        float slice = (e1 - rcpSliceCount) / rcpSliceCount;

        return (int)slice;
    }

    glm::vec4 VBufferParameters::ComputeLogarithmicDepthEncodingParams(float nearPlane, float farPlane, float c)
    {
        glm::vec4 depthParams;;

        float n = nearPlane;
        float f = farPlane;

        depthParams.y = 1.0f / glm::log2(c * (f - n) + 1);
        depthParams.x = glm::log2(c) * depthParams.y;
        depthParams.z = n - 1.0f / c; // Same
        depthParams.w = 0.0f;

        return depthParams;
    }

    glm::vec4 VBufferParameters::ComputeLogarithmicDepthDecodingParams(float nearPlane, float farPlane, float c)
    {
        glm::vec4 depthParams;

        float n = nearPlane;
        float f = farPlane;

        depthParams.x = 1.0f / c;
        depthParams.y = glm::log2(c * (f - n) + 1);
        depthParams.z = n - 1.0f / c; // Same
        depthParams.w = 0.0f;

        return depthParams;
    }

    float CornetteShanksPhasePartConstant(float anisotropy)
    {
        float g = anisotropy;
        return (3.0f / (8.0f * MoYu::f::PI)) * (1.0f - g * g) / (2.0f + g * g);
    }

    void GetHexagonalClosePackedSpheres7(glm::float2(&coords)[7])
    {
        float r = 0.17054068870105443882f;
        float d = 2 * r;
        float s = r * glm::sqrt(3);

        // Try to keep the weighted average as close to the center (0.5) as possible.
        //  (7)(5)    ( )( )    ( )( )    ( )( )    ( )( )    ( )(o)    ( )(x)    (o)(x)    (x)(x)
        // (2)(1)(3) ( )(o)( ) (o)(x)( ) (x)(x)(o) (x)(x)(x) (x)(x)(x) (x)(x)(x) (x)(x)(x) (x)(x)(x)
        //  (4)(6)    ( )( )    ( )( )    ( )( )    (o)( )    (x)( )    (x)(o)    (x)(x)    (x)(x)
        coords[0] = glm::float2(0, 0);
        coords[1] = glm::float2(-d, 0);
        coords[2] = glm::float2(d, 0);
        coords[3] = glm::float2(-r, -s);
        coords[4] = glm::float2(r, s);
        coords[5] = glm::float2(r, -s);
        coords[6] = glm::float2(-r, s);

        // Rotate the sampling pattern by 15 degrees.
        const float cos15 = 0.96592582628906828675f;
        const float sin15 = 0.25881904510252076235f;

        for (int i = 0; i < 7; i++)
        {
            glm::float2 coord = coords[i];

            coords[i].x = coord.x * cos15 - coord.y * sin15;
            coords[i].y = coord.x * sin15 + coord.y * cos15;
        }
    }


    glm::uint VolumetricFrameIndex(int frameIndex)
    {
        // Here we do modulo 14 because we need the enable to detect a change every frame, but the accumulation is done on 7 frames (7x2=14)
        return frameIndex % 14;
    }

    void ComputeVolumetricFogSliceCountAndScreenFraction(EngineConfig::FogConfig fog, int& sliceCount, float& screenFraction)
    {
        screenFraction = fog.screenResolutionPercentage * 0.01f;
        sliceCount = fog.volumeSliceCount;
    }

    glm::ivec3 ComputeVolumetricViewportSize(int viewportWidth, int viewportHeight, float voxelSize)
    {
        int sliceCount;
        float screenFraction;
        ComputeVolumetricFogSliceCountAndScreenFraction(EngineConfig::g_FogConfig, sliceCount, screenFraction);

        voxelSize = 1.0f / screenFraction; // Does not account for rounding (same function, above)

        int w = glm::round(viewportWidth * screenFraction);
        int h = glm::round(viewportHeight * screenFraction);

        // Round to nearest multiple of viewCount so that each views have the exact same number of slices (important for XR)
        int d = glm::ceil(sliceCount);

        return glm::ivec3(w, h, d);
    }

    VBufferParameters ComputeVolumetricBufferParameters(float width, float height, float nearZ, float farZ, float fovy, EngineConfig::FogConfig fog)
    {
        float voxelSize = 0;
        glm::ivec3 viewportSize = ComputeVolumetricViewportSize(width, height, voxelSize);

        VBufferParameters vBufferParams = VBufferParameters(viewportSize, fog.depthExtent, nearZ, farZ, fovy, fog.sliceDistributionUniformity, voxelSize);

        return vBufferParams;
    }


}
