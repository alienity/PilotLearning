#include "runtime/function/framework2/component/transform/transform_component.h"

#include "runtime/engine.h"

namespace MoYu
{
    void TransformComponent::setPosition(const Pilot::Vector3& new_translation)
    {
        m_transform_buffer[m_next_index].m_position = new_translation;
        m_transform.m_position                      = new_translation;
        m_is_dirty                                  = true;
    }

    void TransformComponent::setScale(const Pilot::Vector3& new_scale)
    {
        m_transform_buffer[m_next_index].m_scale = new_scale;
        m_transform.m_scale                      = new_scale;
        m_is_dirty                               = true;
    }

    void TransformComponent::setRotation(const Pilot::Quaternion& new_rotation)
    {
        m_transform_buffer[m_next_index].m_rotation = new_rotation;
        m_transform.m_rotation                      = new_rotation;
        m_is_dirty                                  = true;
    }

    Pilot::Matrix4x4 TransformComponent::getMatrixWorld()
    {
        if (TransformComponent::isDirtyRecursively(this))
        {
            m_matrix_world = getMatrixWorldRecursively(this);
        }
        return m_matrix_world;
    }

    void TransformComponent::tick(float delta_time)
    {
        std::swap(m_current_index, m_next_index);

        if (m_is_dirty)
        {
            // update transform component, dirty flag will be reset in mesh component
            //tryUpdateRigidBodyComponent();
        }
    }

    // check all parent object to see if they are some object dirty
    bool TransformComponent::isDirty() const
    {
        return TransformComponent::isDirtyRecursively(this);
    }

    Pilot::Matrix4x4 TransformComponent::getMatrixWorldRecursively(const TransformComponent* trans)
    {
        if (trans == nullptr)
            return Pilot::Matrix4x4::Identity;

        Pilot::Matrix4x4 matrix_world = trans->getMatrix();
        if (!trans->m_parent_object.expired())
        {
            auto m_object_ptr    = trans->m_parent_object.lock();
            auto m_object_parent = m_object_ptr->getParent();
            if (!m_object_parent.expired())
            {
                TransformComponent* m_parent_trans = m_object_parent.lock()->getTransformComponent();
                matrix_world = getMatrixWorldRecursively(m_parent_trans) * matrix_world;
            }
        }
        return matrix_world;
    }

    bool TransformComponent::isDirtyRecursively(const TransformComponent* trans)
    {
        if (trans == nullptr)
            return false;

        bool is_dirty = trans->m_is_dirty;
        if (!trans->m_parent_object.expired())
        {
            auto m_object_ptr = trans->m_parent_object.lock();
            auto m_object_parent = m_object_ptr->getParent();
            if (!m_object_parent.expired())
            {
                TransformComponent* m_parent_trans = m_object_parent.lock()->getTransformComponent();
                is_dirty |= isDirtyRecursively(m_parent_trans);
            }
        }
        return is_dirty;
    }


} // namespace MoYu