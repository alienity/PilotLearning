#pragma once

#include "runtime/core/math/moyu_math.h"

#include <mutex>

namespace Pilot
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

        static const Vector3 X, Y, Z;

        Vector3    m_position {0.0f, 0.0f, 0.0f};
        Quaternion m_rotation {Quaternion::Identity};
        Quaternion m_invRotation {Quaternion::Identity};
        float      m_znear {0.1f};
        float      m_zfar {1000.0f};
        Vector3    m_up_axis {Y};

        static constexpr float MIN_FOV {10.0f};
        static constexpr float MAX_FOV {89.0f};
        static constexpr int   MAIN_VIEW_MATRIX_INDEX {0};

        std::vector<Matrix4x4> m_view_matrices {Matrix4x4::Identity};

        void setCurrentCameraType(RenderCameraType type);
        void setMainViewMatrix(const Matrix4x4& view_matrix, RenderCameraType type = RenderCameraType::Editor);

        void move(Vector3 delta);
        void rotate(Vector2 delta);
        void zoom(float offset);
        void lookAt(const Vector3& position, const Vector3& target, const Vector3& up);

        void setAspect(float aspect);
        void setFOVy(float fovy) { m_fovy = fovy; }

        float getAspect() const { return m_aspect; }
        float getFOVy() const { return m_fovy; }

        Vector3    position() const { return m_position; }
        Quaternion rotation() const { return m_rotation; }

        Vector3   forward() const { return (m_invRotation * (-Z)); }
        Vector3   up() const { return (m_invRotation * Y); }
        Vector3   right() const { return (m_invRotation * X); }

        Matrix4x4 getViewMatrix();
        Matrix4x4 getPersProjMatrix() const;
        Matrix4x4 getLookAtMatrix() const { return Math::makeLookAtMatrix(position(), position() + forward(), up()); }

    protected:
        float m_aspect {0.f};
        float m_fovy {0.f};

        std::mutex m_view_matrix_mutex;
    };

    inline const Vector3 RenderCamera::X = {1.0f, 0.0f, 0.0f};
    inline const Vector3 RenderCamera::Y = {0.0f, 1.0f, 0.0f};
    inline const Vector3 RenderCamera::Z = {0.0f, 0.0f, 1.0f};

} // namespace Pilot