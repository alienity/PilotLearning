#include "runtime/function/framework/component/transform/transform_component.h"
#include "runtime/resource/res_type/components/transform.h"
#include "runtime/engine.h"

namespace MoYu
{
    void TransformComponent::postLoadResource(std::weak_ptr<GObject> object, void* data)
    {
        m_object = object;

        TransformRes* transform_res = (TransformRes*)data;

        Transform m_transform = {};

        m_transform.m_position = transform_res->m_position;
        m_transform.m_scale    = transform_res->m_scale;
        m_transform.m_rotation = transform_res->m_rotation;

        m_transform_buffer[m_current_index] = m_transform;
        m_transform_buffer[m_next_index]    = m_transform;

        markInit();
    }

    void TransformComponent::setPosition(const Vector3& new_translation)
    {
        //m_transform.m_position = new_translation;

        m_transform_buffer[m_next_index].m_position = new_translation;

        markDirty();
    }

    void TransformComponent::setScale(const Vector3& new_scale)
    {
        //m_transform.m_scale = new_scale;

        m_transform_buffer[m_next_index].m_scale = new_scale;
        
        markDirty();
    }

    void TransformComponent::setRotation(const Quaternion& new_rotation)
    {
        //m_transform.m_rotation = new_rotation;

        m_transform_buffer[m_next_index].m_rotation = new_rotation;

        markDirty();
    }

    const Matrix4x4 TransformComponent::getMatrixWorld()
    {
        if (TransformComponent::isDirtyRecursively(this))
        {
            m_matrix_world = getMatrixWorldRecursively(this);
        }
        return m_matrix_world;
    }

    const bool TransformComponent::isMatrixDirty() const
    {
        return m_matrix_world_prev != m_matrix_world;
    }

    void TransformComponent::preTick(float delta_time)
    {
        if (m_object.expired() || this->isNone())
            return;

        m_matrix_world_prev = m_matrix_world;

        if (TransformComponent::isDirtyRecursively(this))
        {
            // update transform component, dirty flag will be reset in mesh component
            UpdateWorldMatrixRecursively(this);
        }

        m_transform_buffer[m_current_index] = m_transform_buffer[m_next_index];

        std::swap(m_current_index, m_next_index);
    }

    void TransformComponent::tick(float delta_time)
    {
        if (m_object.expired() || this->isNone())
            return;

        //m_matrix_world_prev = m_matrix_world;

        //if (TransformComponent::isDirtyRecursively(this))
        //{
        //    // update transform component, dirty flag will be reset in mesh component
        //    UpdateWorldMatrixRecursively(this);
        //}

        //m_transform_buffer[m_current_index] = m_transform_buffer[m_next_index];

        //std::swap(m_current_index, m_next_index);

        //if (TransformComponent::isDirtyRecursively(this))
        //{
        //    // update transform component, dirty flag will be reset in mesh component
        //    //tryUpdateRigidBodyComponent();
        //}

        //if (g_is_editor_mode)
        //{
        //    m_transform_buffer[m_next_index] = m_transform;
        //}

        //markIdle();
    }
    
    void TransformComponent::lateTick(float delta_time)
    {
        if (m_object.expired() || this->isNone())
            return;

        markIdle();
    }

    Matrix4x4 TransformComponent::getMatrixWorldRecursively(const TransformComponent* trans)
    {
        if (trans == nullptr)
            return Matrix4x4::Identity;

        Matrix4x4 matrix_world = trans->getMatrix();
        if (!trans->m_object.expired())
        {
            auto m_object_ptr    = trans->m_object.lock();
            auto m_object_parent = m_object_ptr->getParent();
            if (m_object_parent)
            {
                TransformComponent* m_parent_trans = m_object_parent->getTransformComponent().lock().get();
                matrix_world = getMatrixWorldRecursively(m_parent_trans) * matrix_world;
            }
        }
        return matrix_world;
    }

    // check all parent object to see if they are some object dirty
    bool TransformComponent::isDirtyRecursively(const TransformComponent* trans)
    {
        if (trans == nullptr)
            return false;

        bool is_dirty = trans->isDirty();
        if (!trans->m_object.expired())
        {
            auto m_object_ptr    = trans->m_object.lock();
            auto m_object_parent = m_object_ptr->getParent();
            if (m_object_parent)
            {
                TransformComponent* m_parent_trans = m_object_parent->getTransformComponent().lock().get();
                is_dirty |= isDirtyRecursively(m_parent_trans);
            }
        }
        return is_dirty;
    }

    void TransformComponent::UpdateWorldMatrixRecursively(TransformComponent* trans)
    {
        if (TransformComponent::isDirtyRecursively(trans))
        {
            trans->m_matrix_world = getMatrixWorldRecursively(trans);
            trans->markIdle();
        }
    }

} // namespace MoYu