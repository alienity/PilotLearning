#pragma once

#include "runtime/core/math/moyu_math.h"

#include "runtime/resource/res_type/components/light.h"

#include "runtime/function/framework/component/component.h"

namespace Pilot
{
    class RenderCamera;

    enum class LightType : unsigned char
    {
        directional,
        point,
        spot,
        invalid
    };

    REFLECTION_TYPE(LightComponent)
    CLASS(LightComponent : public Component, WhiteListFields)
    {
        REFLECTION_BODY(LightComponent)

    public:
        LightComponent() = default;

        void reset();

        void postLoadResource(std::weak_ptr<GObject> parent_object) override;

    private:
        META(Enable)
        LightComponentRes m_light_res;

        LightType m_light_type {LightType::invalid};
    };
} // namespace Pilot
