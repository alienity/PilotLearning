#pragma once

//------------------------------------------------------------------------------
// Uniforms access
//------------------------------------------------------------------------------

struct FrameUniforms
{
    float4x4 viewFromWorldMatrix;
    float4x4 worldFromViewMatrix;
    float4x4 clipFromViewMatrix;
    float4x4 viewFromClipMatrix;
    float4x4 clipFromWorldMatrix;
    float4x4 worldFromClipMatrix;
    float4x4 userWorldFromWorldMatrix;
    float4   resolution;
    float4   userTime;
    float    time;
    float    exposure;
    float    ev100;
};

float4x4 getViewFromWorldMatrix(in FrameUniforms frameUniforms) { return frameUniforms.viewFromWorldMatrix; }

float4x4 getWorldFromViewMatrix(in FrameUniforms frameUniforms) { return frameUniforms.worldFromViewMatrix; }

float4x4 getClipFromViewMatrix(in FrameUniforms frameUniforms) { return frameUniforms.clipFromViewMatrix; }

float4x4 getViewFromClipMatrix(in FrameUniforms frameUniforms) { return frameUniforms.viewFromClipMatrix; }

float4x4 getClipFromWorldMatrix(in FrameUniforms frameUniforms) { return frameUniforms.clipFromWorldMatrix; }

float4x4 getWorldFromClipMatrix(in FrameUniforms frameUniforms) { return frameUniforms.worldFromClipMatrix; }

float4x4 getUserWorldFromWorldMatrix(in FrameUniforms frameUniforms) { return frameUniforms.userWorldFromWorldMatrix; }

float getTime(in FrameUniforms frameUniforms) { return frameUniforms.time; }

float4 getUserTime(in FrameUniforms frameUniforms) { return frameUniforms.userTime; }

float getUserTimeMod(in FrameUniforms frameUniforms, float m)
{
    return fmod(fmod(frameUniforms.userTime.x, m) + fmod(frameUniforms.userTime.y, m), m);
}

#define FILAMENT_OBJECT_SKINNING_ENABLED_BIT 0x100u
#define FILAMENT_OBJECT_MORPHING_ENABLED_BIT 0x200u
#define FILAMENT_OBJECT_CONTACT_SHADOWS_BIT 0x400u

float4 getResolution(in FrameUniforms frameUniforms) { return frameUniforms.resolution; }

float3 getWorldCameraPosition(in FrameUniforms frameUniforms) { return frameUniforms.worldFromViewMatrix[3].xyz; }

float getExposure(in FrameUniforms frameUniforms) { return frameUniforms.exposure; }

float getEV100(in FrameUniforms frameUniforms) { return frameUniforms.ev100; }
