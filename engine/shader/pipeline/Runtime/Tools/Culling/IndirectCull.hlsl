#include "../../ShaderLibrary/Common.hlsl"
#include "../../ShaderLibrary/ShaderVariables.hlsl"
#include "../../Material/Lit/LitProperties.hlsl"

ConstantBuffer<FrameUniforms> g_FrameUniforms : register(b0, space0);

StructuredBuffer<RenderDataPerDraw> g_MeshesInstance : register(t0, space0);
StructuredBuffer<PropertiesPerMaterial> g_MaterialsInstance : register(t1, space0);

AppendStructuredBuffer<uint2> g_OpaqueSortIndexDisBuffer : register(u0, space0);
AppendStructuredBuffer<uint2> g_TransSortIndexDisBuffer : register(u1, space0);

[numthreads(128, 1, 1)]
void CSMain(uint3 GroupID : SV_GroupID, uint GroupIndex : SV_GroupIndex) {

    ConstantBuffer<FrameUniforms>                mMeshPerframeBuffer      = g_FrameUniforms;
    StructuredBuffer<RenderDataPerDraw>          meshesInstanceBuffer     = g_MeshesInstance;
    StructuredBuffer<PropertiesPerMaterial>      materialsInstance        = g_MaterialsInstance;
    AppendStructuredBuffer<uint2>                opaqueSortIndexDisBuffer = g_OpaqueSortIndexDisBuffer;
    AppendStructuredBuffer<uint2>                transSortIndexDisBuffer  = g_TransSortIndexDisBuffer;

    uint index = (GroupID.x * 128) + GroupIndex;
    if (index < mMeshPerframeBuffer.meshUniform.totalMeshCount)
    {
        RenderDataPerDraw renderData = meshesInstanceBuffer[index];
        uint lightPropertyBufferIndexOffset = GetLightPropertyBufferIndexOffset(renderData);
        BoundingBox aabb = GetRendererBounds(renderData);
        PropertiesPerMaterial propertiesPerMaterial = materialsInstance[lightPropertyBufferIndexOffset];

        aabb.Transform(renderData.objectToWorldMatrix);

        Frustum frustum = ExtractPlanesDX(mMeshPerframeBuffer.cameraUniform._CurFrameUniform.clipFromWorldMatrix);

        bool visible = FrustumContainsBoundingBox(frustum, aabb) != CONTAINMENT_DISJOINT;
        if (visible)
        {
            float3 cameraPos  = mMeshPerframeBuffer.cameraUniform._CurFrameUniform._WorldSpaceCameraPos;
            float3 aabbCenter = aabb.Center.xyz;

            float meshDistance = distance(cameraPos, aabbCenter);

            uint2 sortIndexDistance = uint2(index, asuint(meshDistance));

            if (propertiesPerMaterial._BlendMode != 0)
            {
                transSortIndexDisBuffer.Append(sortIndexDistance);
            }
            else
            {
                opaqueSortIndexDisBuffer.Append(sortIndexDistance);
            }
        }
    }
}
