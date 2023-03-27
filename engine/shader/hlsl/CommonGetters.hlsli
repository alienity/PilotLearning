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

float4x4 getViewFromWorldMatrix(FrameUniforms frameUniforms) { return frameUniforms.viewFromWorldMatrix; }

float4x4 getWorldFromViewMatrix(FrameUniforms frameUniforms) { return frameUniforms.worldFromViewMatrix; }

float4x4 getClipFromViewMatrix(FrameUniforms frameUniforms) { return frameUniforms.clipFromViewMatrix; }

float4x4 getViewFromClipMatrix(FrameUniforms frameUniforms) { return frameUniforms.viewFromClipMatrix; }

float4x4 getClipFromWorldMatrix(FrameUniforms frameUniforms) { return frameUniforms.clipFromWorldMatrix; }

float4x4 getWorldFromClipMatrix(FrameUniforms frameUniforms) { return frameUniforms.worldFromClipMatrix; }

float4x4 getUserWorldFromWorldMatrix(FrameUniforms frameUniforms) { return frameUniforms.userWorldFromWorldMatrix; }

float getTime(FrameUniforms frameUniforms) { return frameUniforms.time; }

float4 getUserTime(FrameUniforms frameUniforms) { return frameUniforms.userTime; }

float getUserTimeMod(FrameUniforms frameUniforms, float m)
{
    return fmod(fmod(frameUniforms.userTime.x, m) + fmod(frameUniforms.userTime.y, m), m);
}

#define FILAMENT_OBJECT_SKINNING_ENABLED_BIT 0x100u
#define FILAMENT_OBJECT_MORPHING_ENABLED_BIT 0x200u
#define FILAMENT_OBJECT_CONTACT_SHADOWS_BIT 0x400u

float4 getResolution(FrameUniforms frameUniforms) { return frameUniforms.resolution; }

float3 getWorldCameraPosition(FrameUniforms frameUniforms) { return frameUniforms.worldFromViewMatrix[3].xyz; }

float getExposure(FrameUniforms frameUniforms) { return frameUniforms.exposure; }

float getEV100(FrameUniforms frameUniforms) { return frameUniforms.ev100; }
