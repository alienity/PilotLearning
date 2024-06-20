#include "../../ShaderLibrary/Common.hlsl"
#include "../../ShaderLibrary/ShaderVariables.hlsl"
#include "../../Material/Lit/LitProperties.hlsl"

cbuffer RootConstants : register(b0, space0)
{
    uint perFrameBufferIndex;
    uint renderDataPerDrawIndex;
    uint propertiesPerMaterialIndex;
    uint opaqueSortIndexDisBufferIndex;
    uint transSortIndexDisBufferIndex;
};

[numthreads(128, 1, 1)]
void CSMain(uint GroupIndex : SV_GroupIndex, uint3 GroupID : SV_GroupID) {
    // Each thread processes one mesh instance
    // Compute index and ensure is within bounds
    ConstantBuffer<FrameUniforms> mFrameUniforms = ResourceDescriptorHeap[perFrameBufferIndex];
    StructuredBuffer<RenderDataPerDraw> renderDataPerDrawBuffer = ResourceDescriptorHeap[renderDataPerDrawIndex];
    StructuredBuffer<PropertiesPerMaterial> propertiesPerMaterialBuffer = ResourceDescriptorHeap[propertiesPerMaterialIndex];
    AppendStructuredBuffer<uint2> opaqueSortIndexDisBuffer = ResourceDescriptorHeap[opaqueSortIndexDisBufferIndex];
    AppendStructuredBuffer<uint2> transSortIndexDisBuffer = ResourceDescriptorHeap[transSortIndexDisBufferIndex];

    uint index = (GroupID.x * 128) + GroupIndex;
    if (index < mFrameUniforms.meshUniform.totalMeshCount)
    {
        RenderDataPerDraw renderData = renderDataPerDrawBuffer[index];
        // uint lightPropertyBufferIndex = GetLightPropertyBufferIndex(renderData);
        uint lightPropertyBufferIndexOffset = GetLightPropertyBufferIndexOffset(renderData);
        BoundingBox aabb = GetRendererBounds(renderData);
        
        PropertiesPerMaterial propertiesPerMaterial = propertiesPerMaterialBuffer[lightPropertyBufferIndexOffset];

        BoundingBox worldAABB = aabb.Transform(renderData.objectToWorldMatrix);

        Frustum frustum = ExtractPlanesDX(mFrameUniforms.cameraUniform._CurFrameUniform.clipFromWorldMatrix);

        bool visible = FrustumContainsBoundingBox(frustum, worldAABB) != CONTAINMENT_DISJOINT;
        if (visible)
        {
            float3 cameraPos  = mFrameUniforms.cameraUniform._CurFrameUniform._WorldSpaceCameraPos;
            float3 aabbCenter = worldAABB.Center.xyz;

            float meshDistance = distance(cameraPos, aabbCenter);

            uint2 sortIndexDistance = uint2(index, asuint(meshDistance));

            if (propertiesPerMaterial._BlendMode == 1)
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
