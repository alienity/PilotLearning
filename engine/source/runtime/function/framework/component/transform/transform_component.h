#pragma once

#include "runtime/core/math/moyu_math2.h"
#include "runtime/function/framework/component/component.h"
#include "runtime/function/framework/object/object.h"

namespace MoYu
{
    class TransformComponent : public Component
    {
    public:
        TransformComponent() { m_component_name = "TransformComponent"; };

        void postLoadResource(std::weak_ptr<GObject> object, const std::string json_data) override;

        void save(ComponentDefinitionRes& out_component_res) override;

        void markToErase() override {};

        glm::float3    getPosition() const { return m_transform_buffer[m_current_index].m_position; }
        glm::float3    getScale() const { return m_transform_buffer[m_current_index].m_scale; }
        glm::quat getRotation() const { return m_transform_buffer[m_current_index].m_rotation; }

        void setPosition(const glm::float3& new_translation);
        void setScale(const glm::float3& new_scale);
        void setRotation(const glm::quat& new_rotation);

        const Transform& getTransformConst() const { return m_transform_buffer[m_current_index]; }

        // for editor
        Transform& getTransform() { return m_transform_buffer[m_next_index]; }

        const glm::float4x4 getMatrix() const { return m_transform_buffer[m_current_index].getMatrix(); }

        const glm::float4x4 getMatrixWorld();

        const bool isMatrixDirty() const;

        void preTick(float delta_time) override;
        void tick(float delta_time) override;
        void lateTick(float delta_time) override;

        static glm::float4x4 getMatrixWorldRecursively(const TransformComponent* trans);
        static bool isDirtyRecursively(const TransformComponent* trans);
        static void UpdateWorldMatrixRecursively(TransformComponent* trans);

    private:
        glm::float4x4 m_matrix_world_prev {MYMatrix4x4::Zero};
        glm::float4x4 m_matrix_world {MYMatrix4x4::Identity};

        //Transform m_transform;

        Transform m_transform_buffer[2];
        uint32_t  m_current_index {0};
        uint32_t  m_next_index {1};
    };
} // namespace MoYu
