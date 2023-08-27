#pragma once

#include "runtime/core/math/moyu_math.h"

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

        Vector3 m_up_axis {Y};

        // 可以从已知数据中计算出来
        Vector3    m_position {0.0f, 0.0f, 0.0f};
        Quaternion m_rotation {Quaternion::Identity};
        Quaternion m_invRotation {Quaternion::Identity};
        float      m_aspect {1.78f};
        Matrix4x4  m_project_matrix {Matrix4x4::Identity};

        // 需要传入，以计算projectionMatrix
        int       m_width {1366};
        int       m_height {768};
        float     m_znear {0.1f};
        float     m_zfar {1000.0f};
        float     m_fovy {90.f};
        bool      m_isPerspective {true};
        Matrix4x4 m_view_matrix {Matrix4x4::Identity};

        static const Vector3 X, Y, Z;

        static constexpr float MIN_FOVY {10.0f};
        static constexpr float MAX_FOVY {120.0f};
        static constexpr int   MAIN_VIEW_MATRIX_INDEX {0};

        void setMainViewMatrix(const Matrix4x4& view_matrix, RenderCameraType type = RenderCameraType::Editor);
         
        void move(Vector3 delta);
        void rotate(Vector2 delta);
        void zoom(float offset);
        void lookAt(const Vector3& position, const Vector3& target, const Vector3& up);
        void perspectiveProjection(int width, int height, float znear, float zfar, float fovy);

        Vector3    position() const { return m_position; }
        Quaternion rotation() const { return m_rotation; }

        Vector3   forward() const { return (m_invRotation * (-Z)); }
        Vector3   up() const { return (m_invRotation * Y); }
        Vector3   right() const { return (m_invRotation * X); }

        Matrix4x4 getViewMatrix();
        Matrix4x4 getPersProjMatrix() const;
        Matrix4x4 getLookAtMatrix() const { return Math::makeLookAtMatrix(position(), position() + forward(), up()); }

    protected:
        std::mutex m_view_matrix_mutex;
    };

    inline const Vector3 RenderCamera::X = {1.0f, 0.0f, 0.0f};
    inline const Vector3 RenderCamera::Y = {0.0f, 1.0f, 0.0f};
    inline const Vector3 RenderCamera::Z = {0.0f, 0.0f, 1.0f};

} // namespace MoYu