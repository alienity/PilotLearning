#pragma once

#include "runtime/core/math/moyu_math.h"

#include "runtime/function/framework2/component/component.h"
#include "runtime/function/framework2/object/object.h"

namespace MoYu
{
    class TransformComponent : public Component
    {
    public:
        TransformComponent() = default;

        std::weak_ptr<GObject> getParent() { return m_parent_object; }
        void setParent(std::weak_ptr<GObject> parentObj) { m_parent_object = parentObj; }

        Pilot::Vector3    getPosition() const { return m_transform_buffer[m_current_index].m_position; }
        Pilot::Vector3    getScale() const { return m_transform_buffer[m_current_index].m_scale; }
        Pilot::Quaternion getRotation() const { return m_transform_buffer[m_current_index].m_rotation; }

        void setPosition(const Pilot::Vector3& new_translation);
        void setScale(const Pilot::Vector3& new_scale);
        void setRotation(const Pilot::Quaternion& new_rotation);

        const Pilot::Transform& getTransformConst() const { return m_transform_buffer[m_current_index]; }
        Pilot::Transform&       getTransform() { return m_transform_buffer[m_next_index]; }

        Pilot::Matrix4x4 getMatrix() const { return m_transform_buffer[m_current_index].getMatrix(); }

        Pilot::Matrix4x4 getMatrixWorld();

        void tick(float delta_time) override;
        bool isDirty() const override;

    protected:
        static Pilot::Matrix4x4 getMatrixWorldRecursively(const TransformComponent* trans);
        static bool isDirtyRecursively(const TransformComponent* trans);

        Pilot::Matrix4x4 m_matrix_world;
        Pilot::Transform m_transform;
        Pilot::Transform m_transform_buffer[2];
        size_t           m_current_index {0};
        size_t           m_next_index {1};


    };
} // namespace Pilot
