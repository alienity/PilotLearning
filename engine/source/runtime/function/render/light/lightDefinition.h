#pragma once

#include "runtime/core/math/moyu_math2.h"

#ifndef _CPP_MACRO_
#define _CPP_MACRO_
#endif

#include <vector>

// datas map to hlsl
namespace HLSL
{
#include "../shader/pipeline/Runtime/Lighting/LightDefinition.hlsl"
#include "../shader/pipeline/Runtime/Lighting/Shadow/HDShadowManager.hlsl"


    //bool PrepareLightData(std::vector<LightData>& InOutLightDatas, std::vector<HDShadowData>& InOutLightShadows);

    //bool PrepareDirectionLightData(std::vector<DirectionalLightData>& InOutDirLightDatas, std::vector<HDDirectionalShadowData>& InOutDirShadows);



} // namespace HLSL
