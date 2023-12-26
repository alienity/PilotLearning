#include "d3d12.hlsli"
#include "Shader.hlsli"
#include "CommonMath.hlsli"
#include "InputTypes.hlsli"

// Values used to linearize the Z buffer (http://www.humus.name/temp/Linearize%20depth.txt)
// x = 1-far/near
// y = far/near
// z = x/far
// w = y/far
// or in case of a reversed depth buffer (UNITY_REVERSED_Z is 1)
// x = -1+far/near
// y = 1
// z = x/far
// w = 1/far
// https://forum.unity.com/threads/decodedepthnormal-linear01depth-lineareyedepth-explanations.608452/
// float4 ZBufferParams

float Linear01Depth(float z, float4 ZBufferParams)
{
    return 1.0 / (ZBufferParams.x * z + ZBufferParams.y);
}

// Z buffer to linear depth
float LinearEyeDepth(float z, float4 ZBufferParams)
{
    return 1.0 / (ZBufferParams.z * z + ZBufferParams.w);
}

