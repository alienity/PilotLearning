#pragma once

#include "runtime/core/math/moyu_math2.h"
#include "runtime/function/render/jitter_helper.h"
#include <mutex>

namespace MoYu
{
    enum class RenderCameraType : int
    {
        Editor,
        Motor
    };

    class RenderCamera
    {
    public:
        RenderCameraType m_current_camera_type {RenderCameraType::Editor};

        glm::float3 m_up_axis {Y};

        // 可以从已知数据中计算出来
        glm::float3   m_position {0.0f, 0.0f, 0.0f};
        glm::quat     m_rotation {MYQuaternion::Identity};
        glm::quat     m_invRotation {MYQuaternion::Identity};
        float         m_aspect {1.78f};
        glm::float4x4 m_project_matrix {MYMatrix4x4::Identity};

        // 需要传入，以计算projectionMatrix
        int       m_pixelWidth {1366};
        int       m_pixelHeight {768};
        float     m_nearClipPlane {0.1f};
        float     m_farClipPlane {1000.0f};
        float     m_fieldOfViewY {90.f};
        bool      m_isPerspective {true};
        glm::float4x4 m_view_matrix {MYMatrix4x4::Identity};

        static const glm::float3 X, Y, Z;

        static constexpr float MIN_FOVY {10.0f};
        static constexpr float MAX_FOVY {120.0f};
        static constexpr int   MAIN_VIEW_MATRIX_INDEX {0};

        void updatePerFrame();

        void setMainViewMatrix(const glm::float4x4& view_matrix, RenderCameraType type = RenderCameraType::Editor);
         
        void move(glm::float3 delta);
        void rotate(glm::float2 delta); // delta.x -- yaw, delta.y -- pitch
        void zoom(float offset);
        void lookAt(const glm::float3& position, const glm::float3& target, const glm::float3& up);
        void perspectiveProjection(int width, int height, float znear, float zfar, float fovy);

        glm::float3 position() const { return m_position; }
        glm::quat   rotation() const { return m_rotation; }

        glm::float3 forward() const { return (m_invRotation * (-Z)); }
        glm::float3 up() const { return (m_invRotation * Y); }
        glm::float3 right() const { return (m_invRotation * X); }

        glm::float4x4 getViewMatrix();
        glm::float4x4 getPersProjMatrix() const;
        glm::float4x4 getUnJitterPersProjMatrix() const;
        glm::float4x4 getLookAtMatrix() const;

        glm::float4x4 getWorldToCameraMatrix();
        glm::float4x4 getCameraToWorldMatrix();

        glm::float4   getProjectionExtents(float texelOffsetX = 0, float texelOffsetY = 0) const;
        glm::float4x4 getProjectionMatrix(float texelOffsetX, float texelOffsetY) const;

        FrustumJitter* getFrustumJitter();

    protected:
        std::mutex m_view_matrix_mutex;
        FrustumJitter m_frustumJitter;
    };

    inline const glm::float3 RenderCamera::X = {1.0f, 0.0f, 0.0f};
    inline const glm::float3 RenderCamera::Y = {0.0f, 1.0f, 0.0f};
    inline const glm::float3 RenderCamera::Z = {0.0f, 0.0f, 1.0f};

} // namespace MoYu