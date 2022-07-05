#pragma once

#include "runtime/function/framework/object/object_id_allocator.h"

#include "runtime/function/render/light.h"
#include "runtime/function/render/render_common.h"
#include "runtime/function/render/render_entity.h"
#include "runtime/function/render/render_object.h"

#include <optional>
#include <vector>

namespace Pilot
{
    class RenderCamera;

    class RenderScene
    {
    public:
        static constexpr size_t MaterialLimit = 8192;
        static constexpr size_t LightLimit    = 32;
        static constexpr size_t MeshLimit     = 8192;

        
    };
} // namespace Piccolo
