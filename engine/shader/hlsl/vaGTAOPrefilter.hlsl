///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2016-2021, Intel Corporation 
// 
// SPDX-License-Identifier: MIT
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// XeGTAO is based on GTAO/GTSO "Jimenez et al. / Practical Real-Time Strategies for Accurate Indirect Occlusion", 
// https://www.activision.com/cdn/research/Practical_Real_Time_Strategies_for_Accurate_Indirect_Occlusion_NEW%20VERSION_COLOR.pdf
// 
// Implementation:  Filip Strugar (filip.strugar@intel.com), Steve Mccalla <stephen.mccalla@intel.com>         (\_/)
// Version:         (see XeGTAO.h)                                                                            (='.'=)
// Details:         https://github.com/GameTechDev/XeGTAO                                                     (")_(")
//
// Version history: see XeGTAO.h
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "CommonMath.hlsli"
#include "vaNoise.hlsl"
#include "XeGTAO.hlsli"

cbuffer GTAOConstantBuffer : register( b0 )
{
    uint gitao_consts_index;
    // input output textures for the first pass (XeGTAO_PrefilterDepths16x16)
    uint srcRawDepth_index;           // source depth buffer data (in NDC space in DirectX)
    uint outWorkingDepthMIP0_index;   // output viewspace depth MIP (these are views into g_srcWorkingDepth MIP levels)
    uint outWorkingDepthMIP1_index;   // output viewspace depth MIP (these are views into g_srcWorkingDepth MIP levels)
    uint outWorkingDepthMIP2_index;   // output viewspace depth MIP (these are views into g_srcWorkingDepth MIP levels)
    uint outWorkingDepthMIP3_index;   // output viewspace depth MIP (these are views into g_srcWorkingDepth MIP levels)
    uint outWorkingDepthMIP4_index;   // output viewspace depth MIP (these are views into g_srcWorkingDepth MIP levels)
}

SamplerState g_samplerPointClamp : register(s10);

// Engine-specific entry point for the first pass
[numthreads(8, 8, 1)]   // <- hard coded to 8x8; each thread computes 2x2 blocks so processing 16x16 block: Dispatch needs to be called with (width + 16-1) / 16, (height + 16-1) / 16
void CSPrefilterDepths16x16( uint2 dispatchThreadID : SV_DispatchThreadID, uint2 groupThreadID : SV_GroupThreadID )
{
    ConstantBuffer<GTAOConstants> g_GTAOConsts = ResourceDescriptorHeap[gitao_consts_index];
    Texture2D<float>     g_srcRawDepth = ResourceDescriptorHeap[srcRawDepth_index];
    RWTexture2D<lpfloat> g_outWorkingDepthMIP0 = ResourceDescriptorHeap[outWorkingDepthMIP0_index];
    RWTexture2D<lpfloat> g_outWorkingDepthMIP1 = ResourceDescriptorHeap[outWorkingDepthMIP1_index];
    RWTexture2D<lpfloat> g_outWorkingDepthMIP2 = ResourceDescriptorHeap[outWorkingDepthMIP2_index];
    RWTexture2D<lpfloat> g_outWorkingDepthMIP3 = ResourceDescriptorHeap[outWorkingDepthMIP3_index];
    RWTexture2D<lpfloat> g_outWorkingDepthMIP4 = ResourceDescriptorHeap[outWorkingDepthMIP4_index];

    XeGTAO_PrefilterDepths16x16( dispatchThreadID, groupThreadID, g_GTAOConsts, g_srcRawDepth, g_samplerPointClamp, 
        g_outWorkingDepthMIP0, g_outWorkingDepthMIP1, g_outWorkingDepthMIP2, g_outWorkingDepthMIP3, g_outWorkingDepthMIP4 );
}

///
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
