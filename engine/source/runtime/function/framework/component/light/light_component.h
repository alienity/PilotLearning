#pragma once

#include "runtime/core/math/moyu_math.h"

#include "runtime/resource/res_type/components/light.h"

#include "runtime/function/framework/component/component.h"

#include "runtime/function/render/render_object.h"

namespace Pilot
{
    class RenderCamera;

    REFLECTION_TYPE(LightComponent)
    CLASS(LightComponent : public Component, WhiteListFields)
    {
        REFLECTION_BODY(LightComponent)

    public:
        LightComponent() = default;

        void reset();

        void postLoadResource(std::weak_ptr<GObject> parent_object) override;

        void tick(float delta_time) override;

    private:
        META(Enable)
        LightComponentRes m_light_res;

        GameObjectComponentDesc m_light_part_desc;
    };
} // namespace Pilot
