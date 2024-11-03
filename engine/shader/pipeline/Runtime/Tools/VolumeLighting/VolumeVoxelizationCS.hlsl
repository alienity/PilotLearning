//--------------------------------------------------------------------------------------------------
// Definitions
//--------------------------------------------------------------------------------------------------

// #pragma enable_d3d11_debug_symbols

#define GROUP_SIZE_1D     8

//--------------------------------------------------------------------------------------------------
// Included headers
//--------------------------------------------------------------------------------------------------

#include "../../ShaderLibrary/Common.hlsl"
#include "../../ShaderLibrary/GeometricTools.hlsl"
#include "../../ShaderLibrary/VolumeRendering.hlsl"
#include "../../ShaderLibrary/ShaderVariables.hlsl"
#include "../../Core/Utilities/GeometryUtils.c.hlsl"
#include "../../Tools/VolumeLighting/VolumetricLightingCommon.hlsl"
#include "../../Lighting/LightLoop/LightLoopDef.hlsl"

//--------------------------------------------------------------------------------------------------
// Inputs & outputs
//--------------------------------------------------------------------------------------------------

// StructuredBuffer<OrientedBBox>            _VolumeBounds;
// StructuredBuffer<LocalVolumetricFogEngineData> _VolumeData;
//
// RW_TEXTURE3D(float4, _VBufferDensity); // RGB = sqrt(scattering), A = sqrt(extinction)

cbuffer RootConstants : register(b0, space0)
{
    uint perFrameBufferIndex;
    uint shaderVariablesVolumetricIndex;
    uint _VBufferDensityIndex;  // RGB = sqrt(scattering), A = sqrt(extinction)
};

//--------------------------------------------------------------------------------------------------
// Implementation
//--------------------------------------------------------------------------------------------------

// Jittered ray with screen-space derivatives.
struct JitteredRay
{
    float3 originWS;
    float3 centerDirWS;
    float3 jitterDirWS;
    float3 xDirDerivWS;
    float3 yDirDerivWS;
};

void FillVolumetricDensityBuffer(
    inout RWTexture3D<float4> _VBufferDensity,
    const VolumetricLightingUniform volumetricUniform,
    const VBufferUniform vBufferUniform,
    uint2 voxelCoord2D, JitteredRay ray)
{
    float t0 = DecodeLogarithmicDepthGeneralized(0, vBufferUniform._VBufferDistanceDecodingParams);
    float de = vBufferUniform._VBufferRcpSliceCount; // Log-encoded distance between slices

    for (uint slice = 0; slice < vBufferUniform._VBufferSliceCount; slice++)
    {
        uint3 voxelCoord = uint3(voxelCoord2D, slice);

        float e1 = slice * de + de; // (slice + 1) / sliceCount
        float t1 = DecodeLogarithmicDepthGeneralized(e1, vBufferUniform._VBufferDistanceDecodingParams);
        float dt = t1 - t0;
        float t  = t0 + 0.5 * dt;

        float3 voxelCenterWS = ray.originWS + t * ray.centerDirWS;

        // TODO: the fog value at the center is likely different from the average value across the voxel.
        // Compute the average value.
        float fragmentHeight   = voxelCenterWS.y;
        float heightMultiplier = ComputeHeightFogMultiplier(fragmentHeight, volumetricUniform._HeightFogBaseHeight, volumetricUniform._HeightFogExponents);

        // Start by sampling the height fog.
        float3 voxelScattering = volumetricUniform._HeightFogBaseScattering.xyz * heightMultiplier;
        float  voxelExtinction = volumetricUniform._HeightFogBaseExtinction * heightMultiplier;

        _VBufferDensity[voxelCoord] = float4(voxelScattering, voxelExtinction);

        t0 = t1;
    }
}

[numthreads(GROUP_SIZE_1D, GROUP_SIZE_1D, 1)]
void VolumeVoxelization(uint3 dispatchThreadId : SV_DispatchThreadID,
                        uint2 groupId          : SV_GroupID,
                        uint2 groupThreadId    : SV_GroupThreadID)
{
    // Reminder: our voxels are sphere-capped right frustums (truncated right pyramids).
    // The curvature of the front and back faces is quite gentle, so we can use
    // the right frustum approximation (thus the front and the back faces are squares).
    // Note, that since we still rely on the perspective camera model, pixels at the center
    // of the screen correspond to larger solid angles than those at the edges.
    // Basically, sizes of front and back faces depend on the XY coordinate.
    // https://www.desmos.com/calculator/i3rkesvidk

    ConstantBuffer<FrameUniforms> mFrameUniforms = ResourceDescriptorHeap[perFrameBufferIndex];
    ConstantBuffer<ShaderVariablesVolumetric> shaderVariablesVolumetric = ResourceDescriptorHeap[shaderVariablesVolumetricIndex];
    RWTexture3D<float4> _VBufferDensity = ResourceDescriptorHeap[_VBufferDensityIndex];

    float4x4 _VBufferCoordToViewDirWS = shaderVariablesVolumetric._VBufferCoordToViewDirWS;
    float _VBufferUnitDepthTexelSpacing = shaderVariablesVolumetric._VBufferUnitDepthTexelSpacing;
    
    float3 F = GetViewForwardDir(mFrameUniforms);
    float3 U = GetViewUpDir(mFrameUniforms);

    uint2 voxelCoord = dispatchThreadId.xy;
    float2 centerCoord = voxelCoord + float2(0.5, 0.5);

    uint voxelWidth, voxelHeight, voxelDepth;
    _VBufferDensity.GetDimensions(voxelWidth, voxelHeight, voxelDepth);
    float2 voxelUV = centerCoord / float2(voxelWidth, voxelHeight);
    // float3 rayDirWS = mul(float4(voxelUV * 2 - 1, 0, 0), UNITY_MATRIX_I_VP(mFrameUniforms.cameraUniform)).xyz;
    float3 rayDirWS = mul(UNITY_MATRIX_I_P(mFrameUniforms.cameraUniform), float4(voxelUV * 2 - 1, 0, 1)).xyz;
    
    // Compute a ray direction s.t. ViewSpace(rayDirWS).z = 1.
    // float3 rayDirWS       = mul(-float4(centerCoord, 1, 1), _VBufferCoordToViewDirWS).xyz; // 目前unity的计算无法参考
    
    float3 rightDirWS     = cross(rayDirWS, U);
    float  rcpLenRayDir   = rsqrt(dot(rayDirWS, rayDirWS));
    float  rcpLenRightDir = rsqrt(dot(rightDirWS, rightDirWS));

    JitteredRay ray;
    ray.originWS    = GetCurrentViewPosition(mFrameUniforms);
    ray.centerDirWS = rayDirWS * rcpLenRayDir; // Normalize

    float FdotD = dot(F, ray.centerDirWS);
    float unitDistFaceSize = _VBufferUnitDepthTexelSpacing * FdotD * rcpLenRayDir;

    ray.xDirDerivWS = rightDirWS * (rcpLenRightDir * unitDistFaceSize); // Normalize & rescale
    ray.yDirDerivWS = cross(ray.xDirDerivWS, ray.centerDirWS); // Will have the length of 'unitDistFaceSize' by construction
    ray.jitterDirWS = ray.centerDirWS; // TODO

    FillVolumetricDensityBuffer(_VBufferDensity, mFrameUniforms.volumetricLightingUniform, mFrameUniforms.vBufferUniform, voxelCoord, ray);
}
