#pragma once

#include "runtime/core/math/moyu_math.h"
#include "runtime/resource/res_type/components/camera.h"
#include "runtime/function/framework/component/component.h"

namespace MoYu
{
    enum class CameraMode : unsigned char
    {
        third_person,
        first_person,
        free,
        invalid
    };

    class CameraComponent : public Component
    {
    public:
        CameraComponent() { m_component_name = "CameraComponent"; };

        void reset();

        void postLoadResource(std::weak_ptr<GObject> object, const std::string json_data) override;

        void save(ComponentDefinitionRes& out_component_res) override;

        void tick(float delta_time) override;

        // for editor
        CameraComponentRes& getCameraComponent() { return m_camera_res; }

    private:
        void tickFirstPersonCamera(float delta_time);
        void tickThirdPersonCamera(float delta_time);
        void tickFreeCamera(float delta_time);

        CameraComponentRes m_camera_res;

        CameraMode m_camera_mode {CameraMode::invalid};

        Vector3 m_foward {Vector3::Forward};
        Vector3 m_up {Vector3::Up};
        Vector3 m_left{Vector3::Left};
    };
} // namespace MoYu
