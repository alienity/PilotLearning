
#include "../../ShaderLibrary/Common.hlsl"
#include "../../ShaderLibrary/GeometricTools.hlsl"
#include "../../ShaderLibrary/ShaderVariables.hlsl"
#include "../../ShaderLibrary/VolumeRendering.hlsl"
#include "../../Tools/VolumeLighting/VolumetricMaterialUtils.hlsl"
#include "../../Tools/VolumeLighting/VolumetricLightingCommon.hlsl"
#include "../../../Runtime/Core/Utilities/GeometryUtils.c.hlsl"
#include "../../RenderPipeline/ShaderPass/FragInputs.hlsl"

uint _VolumetricFogGlobalIndex;
ConstantBuffer<FrameUniforms> _PerFrameBuffer;
ConstantBuffer<ShaderVariablesVolumetric> _ShaderVariablesVolumetric;
StructuredBuffer<LocalVolumetricFogDatas> _VolumetricFogData;
ByteAddressBuffer _VolumetricGlobalIndirectionBuffer;

// Jittered ray with screen-space derivatives.
struct JitteredRay
{
    float3 originWS;
    float3 centerDirWS;
    float3 jitterDirWS;
    float3 xDirDerivWS;
    float3 yDirDerivWS;
};

struct VertexToFragment
{
    float4 positionCS : SV_POSITION;
    float3 viewDirectionWS : TEXCOORD0;
    float3 positionOS : TEXCOORD1;
    nointerpolation uint depthSlice : SV_RenderTargetArrayIndex;
};

float3 GetCubeVertexPosition(uint vertexIndex)
{
    int index = _VolumetricGlobalIndirectionBuffer.Load(_VolumetricFogGlobalIndex << 2);
    return _VolumetricFogData[index].volumetricRenderData.obbVertexPositionWS[vertexIndex].xyz;
}

// VertexCubeSlicing needs GetCubeVertexPosition to be declared before
#define GET_CUBE_VERTEX_POSITION GetCubeVertexPosition
#include "../../Tools/VolumeLighting/VertexCubeSlicing.hlsl"

VertexToFragment Vert(uint instanceId : INSTANCEID_SEMANTIC, uint vertexId : VERTEXID_SEMANTIC)
{
    VertexToFragment output;

    CameraUniform _CameraUniform = _PerFrameBuffer.cameraUniform;
    float4 _ZBufferParams = _CameraUniform._ZBufferParams;
    
    int materialDataIndex = _VolumetricGlobalIndirectionBuffer.Load(_VolumetricFogGlobalIndex << 2);

    LocalVolumetricTransform _LocalTransformData = _VolumetricFogData[materialDataIndex].localTransformData;
    
    VolumetricMaterialRenderingData _VolumetricMaterialData =_VolumetricFogData[materialDataIndex].volumetricRenderData;

    uint sliceCount = _VolumetricMaterialData.sliceCount;
    
    uint sliceStartIndex = _VolumetricMaterialData.startSliceIndex;

    uint sliceIndex = sliceStartIndex + (instanceId % sliceCount);
    output.depthSlice = sliceIndex;

    float sliceDepth = VBufferDistanceToSliceIndex(sliceIndex);

#if USE_VERTEX_CUBE_SLICING

    float3 cameraForward = -UNITY_MATRIX_V[2].xyz;
    float3 sliceCubeVertexPosition = ComputeCubeSliceVertexPositionRWS(cameraForward, sliceDepth, vertexId);
    output.positionCS = TransformWorldToHClip(float4(sliceCubeVertexPosition, 1.0));
    output.viewDirectionWS = GetWorldSpaceViewDir(sliceCubeVertexPosition);
    output.positionOS = mul(UNITY_MATRIX_I_M, sliceCubeVertexPosition);

#else

    output.positionCS = GetQuadVertexPosition(vertexId);
    output.positionCS.xy = output.positionCS.xy * _VolumetricMaterialData.viewSpaceBounds.zw + _VolumetricMaterialData.viewSpaceBounds.xy;
    output.positionCS.z = EyeDepthToLinear(sliceDepth, _ZBufferParams);
    output.positionCS.w = 1;

    float3 positionWS = ComputeWorldSpacePosition(output.positionCS, UNITY_MATRIX_I_VP(_CameraUniform));
    output.viewDirectionWS = GetWorldSpaceViewDir(_PerFrameBuffer, positionWS);

    // Calculate object space position
    // output.positionOS = mul(UNITY_MATRIX_I_M, float4(positionWS, 1)).xyz;
    output.positionOS = mul(_LocalTransformData.worldToObjectMatrix, float4(positionWS, 1)).xyz;

#endif // USE_VERTEX_CUBE_SLICING

    return output;
}

FragInputs BuildFragInputs(VertexToFragment v2f, float3 voxelPositionOS, float3 voxelClipSpace)
{
    FragInputs output;

    int index = _VolumetricGlobalIndirectionBuffer.Load(_VolumetricFogGlobalIndex << 2);
    float4x4 modelMatrix = _VolumetricFogData[index].localTransformData.objectToWorldMatrix;
    
    float3 positionWS = mul(modelMatrix, float4(voxelPositionOS, 1)).xyz;
    output.positionSS = v2f.positionCS;
    output.positionRWS = output.positionPredisplacementRWS = positionWS;
    output.positionPixel = uint2(v2f.positionCS.xy);
    output.texCoord0 = float4(saturate(voxelClipSpace * 0.5 + 0.5), 0);
    output.tangentToWorld = k_identity3x3;

    return output;
}

// float ComputeFadeFactor(float3 coordNDC, float distance)
// {
//     return ComputeVolumeFadeFactor(
//         coordNDC, distance,
//         _VolumetricMaterialRcpPosFaceFade.xyz,
//         _VolumetricMaterialRcpNegFaceFade.xyz,
//         _VolumetricMaterialInvertFade,
//         _VolumetricMaterialRcpDistFadeLen,
//         _VolumetricMaterialEndTimesRcpDistFadeLen,
//         exponential,
//         multiplyBlendMode
//     );
// }

void GetVolumeData(FragInputs fragInputs, float3 V, out float3 scatteringColor, out float density)
{
    
    
    // SurfaceDescriptionInputs surfaceDescriptionInputs = FragInputsToSurfaceDescriptionInputs(fragInputs, V);
    // SurfaceDescription surfaceDescription = SurfaceDescriptionFunction(surfaceDescriptionInputs);
    //
    // scatteringColor = surfaceDescription.BaseColor;
    // density = surfaceDescription.Alpha;
}

void Frag(VertexToFragment v2f, out float4 outColor : SV_Target0)
{
    int index = _VolumetricGlobalIndirectionBuffer.Load(_VolumetricFogGlobalIndex << 2);
    LocalVolumetricFogDatas _LocalVolumetricFogData = _VolumetricFogData[index];
    VolumetricMaterialDataCBuffer _VolumeMaterialDataCBuffer = _LocalVolumetricFogData.volumeMaterialDataCBuffer;

    float4 _VolumetricMaterialObbRight = _VolumeMaterialDataCBuffer._VolumetricMaterialObbRight;
    float4 _VolumetricMaterialObbUp = _VolumeMaterialDataCBuffer._VolumetricMaterialObbUp;
    float4 _VolumetricMaterialObbCenter = _VolumeMaterialDataCBuffer._VolumetricMaterialObbCenter;
    float4 _VolumetricMaterialObbExtents = _VolumeMaterialDataCBuffer._VolumetricMaterialObbExtents;
    
    float4 _VolumetricMaterialRcpPosFaceFade = _VolumeMaterialDataCBuffer._VolumetricMaterialRcpPosFaceFade;
    float4 _VolumetricMaterialRcpNegFaceFade = _VolumeMaterialDataCBuffer._VolumetricMaterialRcpNegFaceFade;
    float4 _VolumetricMaterialInvertFade = _VolumeMaterialDataCBuffer._VolumetricMaterialInvertFade;
    float4 _VolumetricMaterialRcpDistFadeLen = _VolumeMaterialDataCBuffer._VolumetricMaterialRcpDistFadeLen;
    float4 _VolumetricMaterialEndTimesRcpDistFadeLen = _VolumeMaterialDataCBuffer._VolumetricMaterialEndTimesRcpDistFadeLen;
    float4 _VolumetricMaterialFalloffMode = _VolumeMaterialDataCBuffer._VolumetricMaterialFalloffMode;

    LocalFogCustomData localFogCustomData = _LocalVolumetricFogData.localFogCustomData;
    float _FogVolumeFogDistanceProperty = localFogCustomData._FogVolumeFogDistanceProperty;
    float3 _FogVolumeSingleScatteringAlbedo = localFogCustomData._FogVolumeSingleScatteringAlbedo;
    float4 _FogVolumeBlendMode = localFogCustomData._FogVolumeBlendMode;

    
    float3 _WorldSpaceCameraPos = _PerFrameBuffer.cameraUniform._WorldSpaceCameraPos;

    float3 albedo;
    float extinction;

    VBufferUniform _VBufferUniform = _PerFrameBuffer.vBufferUniform;
    
    int _VBufferSliceCount = _VBufferUniform._VBufferSliceCount;
    
    float sliceDistance = VBufferDistanceToSliceIndex(v2f.depthSlice % _VBufferSliceCount);

    // Compute voxel center position and test against volume OBB
    float3 raycenterDirWS = normalize(-v2f.viewDirectionWS); // Normalize
    float3 rayoriginWS    = GetCurrentViewPosition(_PerFrameBuffer);
    float3 voxelCenterWS = rayoriginWS + sliceDistance * raycenterDirWS;

    float3x3 obbFrame = float3x3(_VolumetricMaterialObbRight.xyz, _VolumetricMaterialObbUp.xyz, cross(_VolumetricMaterialObbRight.xyz, _VolumetricMaterialObbUp.xyz));

    float3 voxelCenterBS = mul(voxelCenterWS - _VolumetricMaterialObbCenter.xyz + _WorldSpaceCameraPos.xyz, transpose(obbFrame));
    float3 voxelCenterCS = (voxelCenterBS * rcp(_VolumetricMaterialObbExtents.xyz));

    // Still need to clip pixels outside of the box because of the froxel buffer shape
    bool overlap = Max3(abs(voxelCenterCS.x), abs(voxelCenterCS.y), abs(voxelCenterCS.z)) <= 1;
    if (!overlap)
        clip(-1);

    FragInputs fragInputs = BuildFragInputs(v2f, voxelCenterBS, voxelCenterCS);
    GetVolumeData(fragInputs, v2f.viewDirectionWS, albedo, extinction);

    // Accumulate volume parameters
    extinction *= ExtinctionFromMeanFreePath(_FogVolumeFogDistanceProperty);
    albedo *= _FogVolumeSingleScatteringAlbedo.rgb;


    
    float3 voxelCenterNDC = saturate(voxelCenterCS * 0.5 + 0.5);
    // float fade = ComputeFadeFactor(voxelCenterNDC, sliceDistance);

    bool exponential = uint(_VolumetricMaterialFalloffMode) == LOCALVOLUMETRICFOGFALLOFFMODE_EXPONENTIAL;
    bool multiplyBlendMode = _FogVolumeBlendMode == LOCALVOLUMETRICFOGBLENDINGMODE_MULTIPLY;
    
    float fade = ComputeVolumeFadeFactor(
        voxelCenterNDC, sliceDistance,
        _VolumetricMaterialRcpPosFaceFade.xyz,
        _VolumetricMaterialRcpNegFaceFade.xyz,
        _VolumetricMaterialInvertFade,
        _VolumetricMaterialRcpDistFadeLen,
        _VolumetricMaterialEndTimesRcpDistFadeLen,
        exponential,
        multiplyBlendMode
    );

    // When multiplying fog, we need to handle specifically the blend area to avoid creating gaps in the fog
    if (_FogVolumeBlendMode == LOCALVOLUMETRICFOGBLENDINGMODE_MULTIPLY)
    {
        outColor = max(0, lerp(float4(1.0, 1.0, 1.0, 1.0), float4(albedo * extinction, extinction), fade.xxxx));
    }
    else
    {
        extinction *= fade;
        outColor = max(0, float4(saturate(albedo * extinction), extinction));
    }
}
