#pragma once

#include "runtime/core/math/moyu_math.h"
#include "runtime/function/framework/component/component.h"
#include "runtime/function/framework/object/object.h"

namespace MoYu
{
    class TransformComponent : public Component
    {
    public:
        TransformComponent() = default;

        void postLoadResource(std::weak_ptr<GObject> object, void* data) override;

        Vector3    getPosition() const { return m_transform_buffer[m_current_index].m_position; }
        Vector3    getScale() const { return m_transform_buffer[m_current_index].m_scale; }
        Quaternion getRotation() const { return m_transform_buffer[m_current_index].m_rotation; }

        void setPosition(const Vector3& new_translation);
        void setScale(const Vector3& new_scale);
        void setRotation(const Quaternion& new_rotation);

        const Transform& getTransformConst() const { return m_transform_buffer[m_current_index]; }
        Transform&       getTransform() { return m_transform_buffer[m_next_index]; }

        Matrix4x4 getMatrix() const { return m_transform_buffer[m_current_index].getMatrix(); }

        Matrix4x4 getMatrixWorld();

        void tick(float delta_time) override;

        bool isDirty() const override;

    protected:
        static Matrix4x4 getMatrixWorldRecursively(const TransformComponent* trans);
        static bool isDirtyRecursively(const TransformComponent* trans);

        Matrix4x4 m_matrix_world;

    protected:
        Transform m_transform;

        Transform m_transform_buffer[2];
        uint32_t  m_current_index {0};
        uint32_t  m_next_index {1};
    };
} // namespace MoYu
