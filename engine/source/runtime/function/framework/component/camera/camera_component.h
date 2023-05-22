#pragma once

#include "runtime/core/math/moyu_math.h"

#include "runtime/resource/res_type/components/camera.h"

#include "runtime/function/framework/component/component.h"

namespace MoYu
{
    class RenderCamera;

    enum class CameraMode : unsigned char
    {
        third_person,
        first_person,
        free,
        invalid
    };

    REFLECTION_TYPE(CameraComponent)
    CLASS(CameraComponent : public Component, WhiteListFields)
    {
        REFLECTION_BODY(CameraComponent)

    public:
        CameraComponent() = default;

        void reset();

        void postLoadResource(std::weak_ptr<GObject> parent_object) override;

        void tick(float delta_time) override;

    private:
        void tickFirstPersonCamera(float delta_time);
        void tickThirdPersonCamera(float delta_time);
        void tickFreeCamera();

        META(Enable)
        CameraComponentRes m_camera_res;

        CameraMode m_camera_mode {CameraMode::invalid};

        Vector3 m_foward {Vector3::Forward};
        Vector3 m_up {Vector3::Up};
        Vector3 m_left{Vector3::Left};
    };
} // namespace MoYu
