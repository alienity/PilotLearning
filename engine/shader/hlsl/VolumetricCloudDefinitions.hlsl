#ifndef VOLUME_CLOUD_DEFINITION_INCLUDE
#define VOLUME_CLOUD_DEFINITION_INCLUDE

struct CloudsConstants
{
    float4 AmbientColor;
    float4 WindDir;
    float WindSpeed;
    float Time;
    float Crispiness;
    float Curliness;
    float Coverage;
    float Absorption;
    float BottomHeight;
    float TopHeight;
    float DensityFactor;
    float DistanceToFadeFrom; // when to start fading
    float DistanceOfFade; // for how long to fade
    float PlanetRadius;
    float2 UpsampleRatio;
    float2 padding0;
};

struct CloudsConstantsCB
{
    CloudsConstants cloudConstants;
};

#endif