#include "volume_lighting_helper.h"

namespace MoYu
{
    //***********************************************************************************
    // 
    // Parametes
    // 
    //***********************************************************************************

    glm::vec4 m_PackedCoeffs[7];
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

    FogVolume m_FogVolume;

    //***********************************************************************************
    // 
    // Functions
    // 
    //***********************************************************************************

    void InitializeFogConstants()
    {
        GetHexagonalClosePackedSpheres7(m_xySeq);
        
        m_FogVolume = FogVolume();

    }

    const FogVolume& GetFogVolume()
    {
        return m_FogVolume;
    }

    void GetXYSeq(glm::vec2(&xySeq)[7])
    {
        for (int i = 0; i < 7; i++)
        {
            xySeq[i] = m_xySeq[i];
        }
    }

    void GetZSeq(float(&zSeq)[7])
    {
        for (int i = 0; i < 7; i++)
        {
            zSeq[i] = m_zSeq[i];
        }
    }


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

    void ComputeVolumetricFogSliceCountAndScreenFraction(const FogVolume& fog, int& sliceCount, float& screenFraction)
    {
        screenFraction = fog.screenResolutionPercentage * 0.01f;
        sliceCount = fog.volumeSliceCount;
    }

    glm::ivec3 ComputeVolumetricViewportSize(const FogVolume& fog, int viewportWidth, int viewportHeight, float& voxelSize)
    {
        int sliceCount;
        float screenFraction;
        ComputeVolumetricFogSliceCountAndScreenFraction(fog, sliceCount, screenFraction);

        voxelSize = 1.0f / screenFraction; // Does not account for rounding (same function, above)

        int w = glm::round(viewportWidth * screenFraction);
        int h = glm::round(viewportHeight * screenFraction);

        // Round to nearest multiple of viewCount so that each views have the exact same number of slices (important for XR)
        int d = glm::ceil(sliceCount);

        return glm::ivec3(w, h, d);
    }

    VBufferParameters ComputeVolumetricBufferParameters(float width, float height, float nearZ, float farZ, float fovy, const FogVolume& fog)
    {
        float voxelSize = 0;
        glm::ivec3 viewportSize = ComputeVolumetricViewportSize(fog, width, height, voxelSize);

        VBufferParameters vBufferParams = VBufferParameters(viewportSize, fog.depthExtent, nearZ, farZ, fovy, fog.sliceDistributionUniformity, voxelSize);

        return vBufferParams;
    }

    LocalVolumetricFogArtistParameters::LocalVolumetricFogArtistParameters(glm::vec3 color, float _meanFreePath, float _anisotropy)
    {
        albedo = color;
        meanFreePath = _meanFreePath;
        blendingMode = LocalVolumetricFogBlendingMode::Additive;
        priority = 0;
        anisotropy = _anisotropy;

        volumeMask = nullptr;
        textureScrollingSpeed = glm::vec3(0);
        textureTiling = glm::vec3(1);
        textureOffset = textureScrollingSpeed;

        size = glm::vec3(1);

        positiveFade = glm::vec3(1) * 0.1f;
        negativeFade = glm::vec3(1) * 0.1f;
        invertFade = false;

        distanceFadeStart = 10000;
        distanceFadeEnd = 10000;

        falloffMode = LocalVolumetricFogFalloffMode::Linear;
    }

    void LocalVolumetricFogArtistParameters::Update(float time)
    {
        //Update scrolling based on deltaTime
        if (volumeMask != nullptr)
        {
            // Switch from right-handed to left-handed coordinate system.
            textureOffset = -(textureScrollingSpeed * time);
        }
    }

    HLSL::LocalVolumetricFogEngineData LocalVolumetricFogArtistParameters::ConvertToEngineData()
    {
        HLSL::LocalVolumetricFogEngineData data;

        data.scattering = VolumeRenderingUtils::ScatteringFromExtinctionAndAlbedo(
            VolumeRenderingUtils::ExtinctionFromMeanFreePath(meanFreePath), albedo);

        data.blendingMode = (int)blendingMode;

        data.textureScroll = textureOffset;
        data.textureTiling = textureTiling;

        data.rcpPosFaceFade.x = glm::min(1.0f / positiveFade.x, FLT_MAX);
        data.rcpPosFaceFade.y = glm::min(1.0f / positiveFade.y, FLT_MAX);
        data.rcpPosFaceFade.z = glm::min(1.0f / positiveFade.z, FLT_MAX);

        data.rcpNegFaceFade.y = glm::min(1.0f / negativeFade.y, FLT_MAX);
        data.rcpNegFaceFade.x = glm::min(1.0f / negativeFade.x, FLT_MAX);
        data.rcpNegFaceFade.z = glm::min(1.0f / negativeFade.z, FLT_MAX);

        data.invertFade = invertFade ? 1 : 0;
        data.falloffMode = (int)falloffMode;

        float distFadeLen = glm::max(distanceFadeEnd - distanceFadeStart, 0.00001526f);

        data.rcpDistFadeLen = 1.0f / distFadeLen;
        data.endTimesRcpDistFadeLen = distanceFadeEnd * data.rcpDistFadeLen;

        return data;
    }

    float ScaleHeightFromLayerDepth(float d)
    {
        // Exp[-d / H] = 0.001
        // -d / H = Log[0.001]
        // H = d / -Log[0.001]
        return d * 0.144765f;
    }

}
