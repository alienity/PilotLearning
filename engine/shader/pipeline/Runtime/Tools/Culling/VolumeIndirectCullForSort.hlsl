#include "../../ShaderLibrary/Common.hlsl"
#include "../../ShaderLibrary/ShaderVariables.hlsl"
#include "../../Tools/VolumeLighting/VolumetricLightingCommon.hlsl"

cbuffer RootConstants : register(b0, space0)
{
    uint volumeCount;
};

ConstantBuffer<FrameUniforms> frameUniform : register(b1, space0);
StructuredBuffer<LocalVolumetricFogDatas> localVolumetricDatas : register(t0, space0);
AppendStructuredBuffer<uint2> volumeSortIndexDisBuffer : register(u0, space0);

[numthreads(128, 1, 1)]
void CSMain(uint GroupIndex : SV_GroupIndex, uint3 GroupID : SV_GroupID) {
    uint index = (GroupID.x * 128) + GroupIndex;
    if (index < volumeCount)
    {
        LocalVolumetricFogDatas renderData = localVolumetricDatas[index];

        BoundingBox aabb = (BoundingBox) 0;
        aabb.Center = float4(renderData.localTransformData.objectToWorldMatrix._m03_m13_m23, 0);
        aabb.Extents = renderData.localTransformData.volumeSize;
        
        aabb.Transform(renderData.localTransformData.objectToWorldMatrix);

        Frustum frustum = ExtractPlanesDX(UNITY_MATRIX_VP(frameUniform.cameraUniform));

        bool visible = FrustumContainsBoundingBox(frustum, aabb) != CONTAINMENT_DISJOINT;
        if (visible)
        {
            float3 cameraPos  = GetCameraPositionWS(frameUniform);
            float3 aabbCenter = aabb.Center.xyz;
            float meshDistance = distance(cameraPos, aabbCenter);
            uint2 sortIndexDistance = uint2(index, asuint(meshDistance));
            volumeSortIndexDisBuffer.Append(sortIndexDistance);
        }
    }
}
