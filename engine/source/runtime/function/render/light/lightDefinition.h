#pragma once

#ifndef GLM_FORCE_RADIANS
#define GLM_FORCE_RADIANS 1
#endif

#ifndef GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_DEPTH_ZERO_TO_ONE 1
#endif

#include <glm/glm.hpp>

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
