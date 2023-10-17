#pragma once

#include "runtime/core/math/moyu_math2.h"

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

        MFloat3 m_up_axis {Y};

        // 可以从已知数据中计算出来
        MFloat3     m_position {0.0f, 0.0f, 0.0f};
        MQuaternion m_rotation {MYQuaternion::Identity};
        MQuaternion m_invRotation {MYQuaternion::Identity};
        float       m_aspect {1.78f};
        MMatrix4x4  m_project_matrix {MYMatrix4x4::Identity};

        // 需要传入，以计算projectionMatrix
        int       m_width {1366};
        int       m_height {768};
        float     m_znear {0.1f};
        float     m_zfar {1000.0f};
        float     m_fovy {90.f};
        bool      m_isPerspective {true};
        MMatrix4x4 m_view_matrix {MYMatrix4x4::Identity};

        static const MFloat3 X, Y, Z;

        static constexpr float MIN_FOVY {10.0f};
        static constexpr float MAX_FOVY {120.0f};
        static constexpr int   MAIN_VIEW_MATRIX_INDEX {0};

        void setMainViewMatrix(const MMatrix4x4& view_matrix, RenderCameraType type = RenderCameraType::Editor);
         
        void move(MFloat3 delta);
        void rotate(MFloat2 delta);
        void zoom(float offset);
        void lookAt(const MFloat3& position, const MFloat3& target, const MFloat3& up);
        void perspectiveProjection(int width, int height, float znear, float zfar, float fovy);

        MFloat3    position() const { return m_position; }
        MQuaternion rotation() const { return m_rotation; }

        MFloat3   forward() const { return (m_invRotation * (-Z)); }
        MFloat3   up() const { return (m_invRotation * Y); }
        MFloat3   right() const { return (m_invRotation * X); }

        MMatrix4x4 getViewMatrix();
        MMatrix4x4 getPersProjMatrix() const;
        MMatrix4x4 getLookAtMatrix() const;

    protected:
        std::mutex m_view_matrix_mutex;
    };

    inline const MFloat3 RenderCamera::X = {1.0f, 0.0f, 0.0f};
    inline const MFloat3 RenderCamera::Y = {0.0f, 1.0f, 0.0f};
    inline const MFloat3 RenderCamera::Z = {0.0f, 0.0f, 1.0f};

} // namespace MoYu