#include "runtime/function/render/render_camera.h"

namespace MoYu
{

    void RenderCamera::setMainViewMatrix(const glm::float4x4& view_matrix, RenderCameraType type)
    {
        std::lock_guard<std::mutex> lock_guard(m_view_matrix_mutex);

        m_current_camera_type = type;
        m_view_matrix = view_matrix;

        glm::float3 s  = glm::float3(view_matrix[0][0], view_matrix[1][0], view_matrix[2][0]);
        glm::float3 u  = glm::float3(view_matrix[0][1], view_matrix[1][1], view_matrix[2][1]);
        glm::float3 f  = glm::float3(-view_matrix[0][2], -view_matrix[1][2], -view_matrix[2][2]);
        m_position = s * (-view_matrix[3][0]) + u * (-view_matrix[3][1]) + f * view_matrix[3][2];
        
        m_rotation = glm::toQuat(view_matrix);
        m_invRotation = glm::conjugate(m_rotation);
    }

    void RenderCamera::move(glm::float3 delta) { m_position += delta; }

     // delta.x -- yaw, delta.y -- pitch
    void RenderCamera::rotate(glm::float2 delta)
    {
        // rotation around x, y axis
        delta = glm::float2(MoYu::degreesToRadians(delta.x), MoYu::degreesToRadians(delta.y));

        // ZXY从右往左乘得旋转矩阵，就是intrinsic的，对应的分别是从右往左 Roll - Pitch - Yaw

        float _alpha, _beta, _gamma;
        glm::extractEulerAngleZXY(glm::toMat4(m_rotation), _gamma, _beta, _alpha);

        _alpha += delta.x;
        _beta += delta.y;
        _gamma = 0;

        // limit pitch
        float _pitchLimit = MoYu::degreesToRadians(89.0f);
        if (_beta > _pitchLimit)
            _beta = _pitchLimit;
        if (_beta < -_pitchLimit)
            _beta = -_pitchLimit;
        
        m_rotation    = glm::toQuat(glm::eulerAngleZXY(_gamma, _beta, _alpha));
        m_invRotation = glm::conjugate(m_rotation);

        //glm::quat pitch = glm::rotate(glm::quat(), delta.x, X);
        //glm::quat yaw   = glm::rotate(glm::quat(), delta.y, Y);

        //glm::quat _rot = glm::toQuat(glm::eulerAngleYXZ(delta.y, delta.x, 0.f));

        //m_rotation = pitch * m_rotation * yaw;
        //m_invRotation = glm::conjugate(m_rotation);
    }

    void RenderCamera::zoom(float offset)
    {
        // > 0 = zoom in (decrease FOV by <offset> angles)
        m_fovy = glm::clamp(m_fovy - offset, MIN_FOVY, MAX_FOVY);
    }

    void RenderCamera::lookAt(const glm::float3& position, const glm::float3& target, const glm::float3& up)
    {
        m_position = position;

        glm::float4x4 viewMat = MoYu::MYMatrix4x4::createLookAtMatrix(position, target, up);

        m_rotation = glm::toQuat(viewMat);
        m_invRotation = glm::inverse(m_rotation);

        /*
        // model rotation
        // maps vectors to camera space (x, y, z)
        glm::float3 forward = glm::float3::normalize(target - position);
        m_rotation      = glm::quat::fromToRotation(forward, glm::float3::Forward);

        // correct the up vector
        // the cross product of non-orthogonal vectors is not normalized
        glm::float3 right  = glm::float3::normalize(glm::float3::cross(glm::float3::normalize(up), -forward));
        glm::float3 orthUp = glm::float3::cross(-forward, right);

        glm::quat upRotation = glm::quat::fromToRotation(m_rotation * orthUp, Y);

        m_rotation = upRotation * m_rotation;

        // inverse of the model rotation
        // maps camera space vectors to model vectors
        m_invRotation = glm::quat::conjugate(m_rotation);
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

    glm::float4x4 RenderCamera::getViewMatrix()
    {
        std::lock_guard<std::mutex> lock_guard(m_view_matrix_mutex);
        auto view_matrix = MYMatrix4x4::Identity;
        switch (m_current_camera_type)
        {
            case RenderCameraType::Editor:
                {
                    glm::float3 _position = position();
                    glm::float3 _forward = forward();
                    glm::float3 _up  = up();

                    view_matrix = MoYu::MYMatrix4x4::createLookAtMatrix(_position, _position + _forward, _up);
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

    glm::float4x4 RenderCamera::getPersProjMatrix() const
    {
        glm::float4x4 proj_mat =
            MoYu::MYMatrix4x4::createPerspectiveFieldOfView(MoYu::f::DEG_TO_RAD * m_fovy, m_aspect, m_znear, m_zfar);

        return proj_mat;
    }

    glm::float4x4 RenderCamera::getLookAtMatrix() const
    {
        return MoYu::MYMatrix4x4::createLookAtMatrix(position(), position() + forward(), up());
    }

} // namespace MoYu
