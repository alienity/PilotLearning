#pragma once
#include "runtime/resource/res_type/common_serializer.h"
#include "material.h"
#include <vector>

namespace MoYu
{
    struct LocalVolumeFogComponentRes
    {
        glm::float3 SingleScatteringAlbedo {1,1,1};
        float FogDistance {150};
        
        glm::float3 Size {8,7.53,8};
        bool PerAxisControl {true};
        glm::float3 BlendDistanceNear {0.5245552,3.104702,1.36497};
        glm::float3 BlendDistanceFar {0.4401054,1.907349e-06,0.2636642};
        int FalloffMode {0};
        bool InvertBlend {false};
        float DistanceFadeStart {50};
        float DistanceFadeEnd {60};

        MaterialImage NoiseImage { DefaultMaterialFogNoiseImage };
        glm::float3 ScrollSpeed {0,0.05,0};
        glm::float3 Tilling {1,1,1};
    };
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(LocalVolumeFogComponentRes,
        SingleScatteringAlbedo, FogDistance, Size, PerAxisControl, BlendDistanceNear, BlendDistanceFar,
        FalloffMode, InvertBlend, DistanceFadeStart, DistanceFadeEnd, NoiseImage, ScrollSpeed, Tilling)
    
} // namespace MoYu