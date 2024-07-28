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

#include "../../ShaderLibrary/Common.hlsl"
#include "vaNoise.hlsl"
#include "XeGTAO.hlsli"

cbuffer GTAOConstantBuffer : register( b0 )
{
    uint gitao_consts_index;
    // input output textures for the third pass (XeGTAO_Denoise)
    uint g_srcWorkingAOTerm_index;  // coming from previous pass
    uint g_srcWorkingEdges_index;   // coming from previous pass
    uint g_outFinalAOTerm_index;    // final AO term - just 'visibility' or 'visibility + bent normals'
}

SamplerState g_samplerPointClamp : register(s10);

// Engine-specific entry point for the third pass
[numthreads(XE_GTAO_NUMTHREADS_X, XE_GTAO_NUMTHREADS_Y, 1)]
void CSDenoisePass( const uint2 dispatchThreadID : SV_DispatchThreadID )
{
    ConstantBuffer<GTAOConstants> g_GTAOConsts = ResourceDescriptorHeap[gitao_consts_index];
    Texture2D<uint>    g_srcWorkingAOTerm  = ResourceDescriptorHeap[g_srcWorkingAOTerm_index];
    Texture2D<lpfloat> g_srcWorkingEdges   = ResourceDescriptorHeap[g_srcWorkingEdges_index];
    RWTexture2D<float>  g_outFinalAOTerm    = ResourceDescriptorHeap[g_outFinalAOTerm_index];

    const uint2 pixCoordBase = dispatchThreadID * uint2( 2, 1 );    // we're computing 2 horizontal pixels at a time (performance optimization)
    // g_samplerPointClamp is a sampler with D3D12_FILTER_MIN_MAG_MIP_POINT filter and D3D12_TEXTURE_ADDRESS_MODE_CLAMP addressing mode
    XeGTAO_Denoise( pixCoordBase, g_GTAOConsts, g_srcWorkingAOTerm, g_srcWorkingEdges, g_samplerPointClamp, g_outFinalAOTerm, false );
}

///
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
