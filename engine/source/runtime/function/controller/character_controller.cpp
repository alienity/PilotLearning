#include "runtime/function/controller/character_controller.h"

#include "runtime/function/framework/component/motor/motor_component.h"
#include "runtime/function/global/global_context.h"

namespace Pilot
{
    Vector3 CharacterController::move(const Vector3& current_position, const Vector3& displacement)
    {
        const float gap = 0.1f;

        Capsule test_capsule;
        test_capsule.m_half_height = m_capsule.m_half_height - gap;
        test_capsule.m_radius      = m_capsule.m_radius - gap;

        Vector3 desired_position = current_position + displacement;

        return desired_position;
    }
} // namespace Pilot
