#include "runtime/function/render/render_camera.h"

namespace MoYu
{

    void RenderCamera::setMainViewMatrix(const MMatrix4x4& view_matrix, RenderCameraType type)
    {
        std::lock_guard<std::mutex> lock_guard(m_view_matrix_mutex);

        m_current_camera_type = type;
        m_view_matrix = view_matrix;

        MFloat3 s  = MFloat3(view_matrix[0][0], view_matrix[1][0], view_matrix[2][0]);
        MFloat3 u  = MFloat3(view_matrix[0][1], view_matrix[1][1], view_matrix[2][1]);
        MFloat3 f  = MFloat3(-view_matrix[0][2], -view_matrix[1][2], -view_matrix[2][2]);
        m_position = s * (-view_matrix[3][0]) + u * (-view_matrix[3][1]) + f * view_matrix[3][2];
        
        m_rotation = glm::toQuat(view_matrix);
        m_invRotation = glm::conjugate(m_rotation);
    }

    void RenderCamera::move(MFloat3 delta) { m_position += delta; }

    void RenderCamera::rotate(MFloat2 delta)
    {
        // rotation around x, y axis
        delta = MFloat2(MoYu::degreesToRadians(delta.x), MoYu::degreesToRadians(delta.y));

        // limit pitch
        float dot = glm::dot(m_up_axis, forward());
        if ((dot < -0.99f && delta.x > 0.0f) || // angle nearing 180 degrees
            (dot > 0.99f && delta.x < 0.0f))    // angle nearing 0 degrees
            delta.x = 0.0f;

        float _y, _x, _z;
        glm::extractEulerAngleZXY(glm::toMat4(m_rotation), _z, _x, _y);

        _y += delta.y;
        _x += delta.x;

        glm::mat4 _rot = glm::eulerAngleZXY(_z, _x, _y);

        m_rotation    = glm::toQuat(_rot);
        m_invRotation = glm::conjugate(m_rotation);

        //MQuaternion pitch = glm::rotate(glm::quat(), delta.x, X);
        //MQuaternion yaw   = glm::rotate(glm::quat(), delta.y, Y);

        //MQuaternion _rot = glm::toQuat(glm::eulerAngleYXZ(delta.y, delta.x, 0.f));

        //m_rotation = pitch * m_rotation * yaw;
        //m_invRotation = glm::conjugate(m_rotation);
    }

    void RenderCamera::zoom(float offset)
    {
        // > 0 = zoom in (decrease FOV by <offset> angles)
        m_fovy = glm::clamp(m_fovy - offset, MIN_FOVY, MAX_FOVY);
    }

    void RenderCamera::lookAt(const MFloat3& position, const MFloat3& target, const MFloat3& up)
    {
        m_position = position;

        MMatrix4x4 viewMat = MoYu::MYMatrix4x4::createLookAtMatrix(position, target, up);

        m_rotation = glm::toQuat(viewMat);
        m_invRotation = glm::inverse(m_rotation);

        /*
        // model rotation
        // maps vectors to camera space (x, y, z)
        MFloat3 forward = MFloat3::normalize(target - position);
        m_rotation      = MQuaternion::fromToRotation(forward, MFloat3::Forward);

        // correct the up vector
        // the cross product of non-orthogonal vectors is not normalized
        MFloat3 right  = MFloat3::normalize(MFloat3::cross(MFloat3::normalize(up), -forward));
        MFloat3 orthUp = MFloat3::cross(-forward, right);

        MQuaternion upRotation = MQuaternion::fromToRotation(m_rotation * orthUp, Y);

        m_rotation = upRotation * m_rotation;

        // inverse of the model rotation
        // maps camera space vectors to model vectors
        m_invRotation = MQuaternion::conjugate(m_rotation);
        */
    }

    void RenderCamera::perspectiveProjection(int width, int height, float znear, float zfar, float fovy)
    {
        m_width  = width;
        m_height = height;
        m_aspect = width / (float)height; 
        m_znear  = znear;
        m_zfar   = zfar;
        m_fovy   = fovy;

        m_project_matrix =
            MoYu::MYMatrix4x4::createPerspectiveFieldOfView(MoYu::f::DEG_TO_RAD * m_fovy, m_aspect, m_znear, m_zfar);
    }

    MMatrix4x4 RenderCamera::getViewMatrix()
    {
        std::lock_guard<std::mutex> lock_guard(m_view_matrix_mutex);
        auto view_matrix = MYMatrix4x4::Identity;
        switch (m_current_camera_type)
        {
            case RenderCameraType::Editor:
                {
                    view_matrix = MoYu::MYMatrix4x4::createLookAtMatrix(position(), position() + forward(), up());
                }
                break;
            case RenderCameraType::Motor:
                    view_matrix = m_view_matrix;
                break;
            default:
                break;
        }
        return view_matrix;
    }

    MMatrix4x4 RenderCamera::getPersProjMatrix() const
    {
        MMatrix4x4 proj_mat =
            MoYu::MYMatrix4x4::createPerspectiveFieldOfView(MoYu::f::DEG_TO_RAD * m_fovy, m_aspect, m_znear, m_zfar);

        return proj_mat;
    }

    MMatrix4x4 RenderCamera::getLookAtMatrix() const
    {
        return MoYu::MYMatrix4x4::createLookAtMatrix(position(), position() + forward(), up());
    }

} // namespace MoYu
