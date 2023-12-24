#include "d3d12.hlsli"
#include "Shader.hlsli"
#include "CommonMath.hlsli"
#include "InputTypes.hlsli"

float Linear01Depth(float z, float4 ZBufferParams)
{
    return 1.0 / (ZBufferParams.x * z + ZBufferParams.y);
}

// Z buffer to linear depth
float LinearEyeDepth(float z, float4 ZBufferParams)
{
    return 1.0 / (ZBufferParams.z * z + ZBufferParams.w);
}

