#include "runtime/function/framework/component/transform/transform_component.h"

#include "runtime/engine.h"

namespace Pilot
{
    void TransformComponent::postLoadResource(std::weak_ptr<GObject> parent_gobject)
    {
        m_parent_object       = parent_gobject;
        m_transform_buffer[0] = m_transform;
        m_transform_buffer[1] = m_transform;
        m_is_dirty            = true;
        
        m_parent_object.lock()->setDirty();
    }

    void TransformComponent::setPosition(const Vector3& new_translation)
    {
        m_transform_buffer[m_next_index].m_position = new_translation;
        m_transform.m_position                      = new_translation;
        m_is_dirty                                  = true;

        m_parent_object.lock()->setDirty();
    }

    void TransformComponent::setScale(const Vector3& new_scale)
    {
        m_transform_buffer[m_next_index].m_scale = new_scale;
        m_transform.m_scale                      = new_scale;
        m_is_dirty                               = true;

        m_parent_object.lock()->setDirty();
    }

    void TransformComponent::setRotation(const Quaternion& new_rotation)
    {
        m_transform_buffer[m_next_index].m_rotation = new_rotation;
        m_transform.m_rotation                      = new_rotation;
        m_is_dirty                                  = true;

        m_parent_object.lock()->setDirty();
    }

    void TransformComponent::tick(float delta_time)
    {
        std::swap(m_current_index, m_next_index);

        if (m_is_dirty)
        {
            // update transform component, dirty flag will be reset in mesh component
            //tryUpdateRigidBodyComponent();
        }

        if (g_is_editor_mode)
        {
            if (m_transform_buffer[m_next_index] != m_transform_buffer[m_current_index])
            {
                m_parent_object.lock()->setDirty();
            }
            m_transform_buffer[m_next_index] = m_transform;
        }
    }

} // namespace Pilot