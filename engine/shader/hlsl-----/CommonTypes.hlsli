#pragma once

// Adreno drivers seem to ignore precision qualifiers in structs, unless they're used in
// UBOs, which is is the case here.
struct ShadowData
{
    float4x4 lightFromWorldMatrix;
    float4   lightFromWorldZ;
    float4   scissorNormalized;
    float    texelSizeAtOneMeter;
    float    bulbRadiusLs;
    float    nearOverFarMinusNear;
    float    normalBias;
    bool     elvsm;
    uint     layer;
    uint     reserved1;
    uint     reserved2;
};

struct BoneData
{
    float3x4 transform; // bone transform is mat4x3 stored in row-major (last row [0,0,0,1])
    float4   cof;       // 8 first cofactor matrix of transform's upper left
};

struct PerRenderableData
{
    float4x4 worldFromModelMatrix;
    float3x3 worldFromModelNormalMatrix;
    uint     morphTargetCount;
    uint     flagsChannels; // see packFlags() below (0x00000fll)
    uint     objectId;      // used for picking
    float  userData; // TODO: We need a better solution, this currently holds the average local scale for the renderable
    float4 reserved[8];
};
