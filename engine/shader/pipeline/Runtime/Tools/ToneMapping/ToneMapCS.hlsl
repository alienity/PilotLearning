//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
// Developed by Minigraph
//
// Author:  James Stanard 
//

#include "ToneMappingUtility.hlsli"
#include "../Common/PixelPacking.hlsli"

//StructuredBuffer<float> Exposure : register( t0 );
//Texture2D<float3> Bloom : register( t1 );
////#if SUPPORT_TYPED_UAV_LOADS
////RWTexture2D<float3> ColorRW : register( u0 );
////#else
//RWTexture2D<uint> DstColor : register( u0 );
//Texture2D<float3> SrcColor : register( t2 );
////#endif
//RWTexture2D<float> OutLuma : register( u1 );
SamplerState LinearSampler : register( s0 );

cbuffer CB0 : register(b0)
{
    float2 g_RcpBufferDim;
    float g_BloomStrength;
};

cbuffer CB1 : register(b1)
{
    uint m_ExposureIndex;
    uint m_BloomIndex;
    uint m_DstColorIndex;
    uint m_SrcColorIndex;
    uint m_OutLumaIndex;
};

//[RootSignature(PostEffects_RootSig)]
[numthreads( 8, 8, 1 )]
void main( uint3 DTid : SV_DispatchThreadID )
{
    StructuredBuffer<float> Exposure = ResourceDescriptorHeap[m_ExposureIndex];
    Texture2D<float3>       Bloom    = ResourceDescriptorHeap[m_BloomIndex];
    RWTexture2D<float3>     DstColor = ResourceDescriptorHeap[m_DstColorIndex];
    Texture2D<float3>       SrcColor = ResourceDescriptorHeap[m_SrcColorIndex];
    RWTexture2D<float>      OutLuma  = ResourceDescriptorHeap[m_OutLumaIndex];

    float2 TexCoord = (DTid.xy + 0.5) * g_RcpBufferDim;

    float3 hdrColor = SrcColor[DTid.xy];

    hdrColor += g_BloomStrength * Bloom.SampleLevel(LinearSampler, TexCoord, 0);
    hdrColor *= Exposure[0];

    // Tone map to SDR
    float3 sdrColor = TM_Stanard(hdrColor);

    DstColor[DTid.xy] = sdrColor;

    OutLuma[DTid.xy] = RGBToLogLuminance(sdrColor);
}
