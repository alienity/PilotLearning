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

// datas map to hlsl
namespace HLSL
{
#include "../shader/pipeline/Runtime/Lighting/LightDefinition.hlsl"

    enum GPULightType
    {
        Directional,
        Point,
        Spot,
        ProjectorPyramid,
        ProjectorBox,

        // AreaLight
        Tube, // Keep Line lights before Rectangle. This is needed because of a compiler bug (see LightLoop.hlsl)
        Rectangle,
        // Currently not supported in real time (just use for reference)
        Disc,
        // Sphere,
    };





} // namespace MoYu
