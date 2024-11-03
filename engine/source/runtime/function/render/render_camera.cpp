#include "runtime/function/render/render_camera.h"
#include "runtime/function/render/renderer/renderer_config.h"
#include "runtime/function/render/utility/HDUtils.h"
#include "runtime/function/render/render_swap_context.h"

namespace MoYu
{
    RenderCamera::RenderCamera(bool perspective)
    {
        taaFrameIndex = 0;
        rawCameraData.m_isPerspective = perspective;
    }

    RenderCamera::~RenderCamera()
    {

    }

    void RenderCamera::updatePerFrame(const CameraSwapData* pCamSwapData)
    {
        const int kMaxSampleCount = 8;
        if (++taaFrameIndex >= kMaxSampleCount)
            taaFrameIndex = 0;

        if (pCamSwapData == nullptr)
            return;

        updateCameraData(
            pCamSwapData->m_is_perspective, 
            pCamSwapData->m_view_matrix, 
            pCamSwapData->m_width, 
            pCamSwapData->m_height, 
            pCamSwapData->m_nearZ, 
            pCamSwapData->m_farZ, 
            pCamSwapData->m_fov_y);

    }

    void RenderCamera::updateCameraData(bool isPerspective, glm::float4x4 viewMatrix, int width, int height, float nearZ, float farZ, float fovY)
    {
        rawCameraData.m_isPerspective = isPerspective;
        setMainViewMatrix(viewMatrix, RenderCameraType::Editor);
        perspectiveProjection(width, height, nearZ, farZ, fovY);

        mainViewConstants.prevViewMatrix = mainViewConstants.viewMatrix;
        mainViewConstants.prevViewProjMatrix = mainViewConstants.viewProjMatrix;
        mainViewConstants.prevInvViewProjMatrix = mainViewConstants.invViewProjMatrix;
        mainViewConstants.prevViewProjMatrixNoCameraTrans = mainViewConstants.viewProjectionNoCameraTrans;

        mainViewConstants.prevWorldSpaceCameraPos = mainViewConstants.worldSpaceCameraPos;

        glm::float4x4 projMatrix = getProjMatrix();
        glm::float4x4 nonJitterProjMatrix = getProjMatrix(true);

        glm::float4x4 noTransViewMatrix = viewMatrix;
        noTransViewMatrix[3] = glm::float4(0, 0, 0, 1);

        glm::float4x4 gpuVPNoTrans = nonJitterProjMatrix * noTransViewMatrix;

        mainViewConstants.viewMatrix = viewMatrix;
        mainViewConstants.invViewMatrix = glm::inverse(mainViewConstants.viewMatrix);
        mainViewConstants.projMatrix = projMatrix;
        mainViewConstants.invProjMatrix = glm::inverse(mainViewConstants.projMatrix);
        mainViewConstants.viewProjMatrix = mainViewConstants.projMatrix * mainViewConstants.viewMatrix;;
        mainViewConstants.invViewProjMatrix = glm::inverse(mainViewConstants.viewProjMatrix);
        mainViewConstants.nonJitteredViewProjMatrix = nonJitterProjMatrix * mainViewConstants.viewMatrix;
        
        mainViewConstants.worldSpaceCameraPos = position();
        mainViewConstants.worldSpaceCameraPosViewOffset = glm::float3(0);
        mainViewConstants.viewProjectionNoCameraTrans = noTransViewMatrix;

        float gpuProjAspect = HDUtils::ProjectionMatrixAspect(mainViewConstants.projMatrix);

        glm::float4 screenSize = 
            glm::float4(rawCameraData.m_pixelWidth, rawCameraData.m_pixelHeight, 
                1.0f / rawCameraData.m_pixelWidth, 1.0f / rawCameraData.m_pixelHeight);

        mainViewConstants.pixelCoordToViewDirWS = 
            ComputePixelCoordToWorldSpaceViewDirectionMatrix(mainViewConstants, screenSize, gpuProjAspect);
    }

    RawCameraData& RenderCamera::GetRawCameraData()
    {
        return rawCameraData;
    }

    ViewConstants& RenderCamera::GetViewConstants()
    {
        return mainViewConstants;
    }

    void RenderCamera::setMainViewMatrix(const glm::float4x4& view_matrix, RenderCameraType type)
    {
        std::lock_guard<std::mutex> lock_guard(m_view_matrix_mutex);

        m_current_camera_type = type;
        rawCameraData.m_view_matrix = view_matrix;

        glm::float3 s  = glm::float3(view_matrix[0][0], view_matrix[1][0], view_matrix[2][0]);
        glm::float3 u  = glm::float3(view_matrix[0][1], view_matrix[1][1], view_matrix[2][1]);
        glm::float3 f  = glm::float3(-view_matrix[0][2], -view_matrix[1][2], -view_matrix[2][2]);
        //m_position = s * (-view_matrix[3][0]) + u * (-view_matrix[3][1]) + f * view_matrix[3][2];
        rawCameraData.m_position = -glm::column(view_matrix, 3);

        rawCameraData.m_rotation = glm::toQuat(view_matrix);
        rawCameraData.m_invRotation = glm::conjugate(rawCameraData.m_rotation);
    }

    void RenderCamera::move(glm::float3 delta)
    {
        rawCameraData.m_position += delta;
    }

     // delta.x -- yaw, delta.y -- pitch
    void RenderCamera::rotate(glm::float2 delta)
    {
        // rotation around x, y axis
        delta = glm::float2(MoYu::degreesToRadians(delta.x), MoYu::degreesToRadians(delta.y));

        // ZXY从右往左乘得旋转矩阵，就是intrinsic的，对应的分别是从右往左 Roll - Pitch - Yaw

        float _alpha, _beta, _gamma;
        glm::extractEulerAngleZXY(glm::toMat4(rawCameraData.m_rotation), _gamma, _beta, _alpha);

        _alpha += delta.x;
        _beta += delta.y;
        _gamma = 0;

        // limit pitch
        float _pitchLimit = MoYu::degreesToRadians(89.0f);
        if (_beta > _pitchLimit)
            _beta = _pitchLimit;
        if (_beta < -_pitchLimit)
            _beta = -_pitchLimit;
        
        rawCameraData.m_rotation    = glm::toQuat(glm::eulerAngleZXY(_gamma, _beta, _alpha));
        rawCameraData.m_invRotation = glm::conjugate(rawCameraData.m_rotation);

        //glm::quat pitch = glm::rotate(glm::quat(), delta.x, X);
        //glm::quat yaw   = glm::rotate(glm::quat(), delta.y, Y);

        //glm::quat _rot = glm::toQuat(glm::eulerAngleYXZ(delta.y, delta.x, 0.f));

        //m_rotation = pitch * m_rotation * yaw;
        //m_invRotation = glm::conjugate(m_rotation);
    }

    void RenderCamera::zoom(float offset)
    {
        // > 0 = zoom in (decrease FOV by <offset> angles)
        rawCameraData.m_fieldOfViewY = glm::clamp(rawCameraData.m_fieldOfViewY - offset, MIN_FOVY, MAX_FOVY);
    }

    void RenderCamera::lookAt(const glm::float3& position, const glm::float3& target, const glm::float3& up)
    {
        rawCameraData.m_position = position;

        glm::float4x4 viewMat = MoYu::MYMatrix4x4::createLookAtMatrix(position, target, up);

        rawCameraData.m_rotation = glm::toQuat(viewMat);
        rawCameraData.m_invRotation = glm::inverse(rawCameraData.m_rotation);

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
        rawCameraData.m_pixelWidth    = width;
        rawCameraData.m_pixelHeight   = height;
        rawCameraData.m_nearClipPlane = znear;
        rawCameraData.m_farClipPlane  = zfar;
        rawCameraData.m_fieldOfViewY  = fovy;

        float tAspect = width / (float)height;

        rawCameraData.m_project_matrix = MoYu::MYMatrix4x4::createPerspectiveFieldOfView(
            MoYu::f::DEG_TO_RAD * rawCameraData.m_fieldOfViewY, tAspect, rawCameraData.m_nearClipPlane, rawCameraData.m_farClipPlane);

        // Analyze the projection matrix.
            // p[2][3] = (reverseZ ? 1 : -1) * (depth_0_1 ? 1 : 2) * (f * n) / (f - n)
        float n = znear;
        float f = zfar;
        float scale = rawCameraData.m_project_matrix[3][2] / (f * n) * (f - n);
        bool depth_0_1 = glm::abs(scale) < 1.5f;
        bool reverseZ = scale > 0;

        // http://www.humus.name/temp/Linearize%20depth.txt
        if (reverseZ)
        {
            rawCameraData.zBufferParams = glm::float4(-1 + f / n, 1, -1 / f + 1 / n, 1 / f);
        }
        else
        {
            rawCameraData.zBufferParams = glm::float4(1 - f / n, f / n, 1 / f - 1 / n, 1 / n);
        }
    }

    bool RenderCamera::isPerspective() const
    {
        return rawCameraData.m_isPerspective;
    }

    void RenderCamera::setViewport(int width, int height)
    {
        rawCameraData.m_pixelWidth = width;
        rawCameraData.m_pixelHeight = height;
    }

    glm::float3 RenderCamera::position() const
    {
        return rawCameraData.m_position;
    }

    glm::quat RenderCamera::rotation() const
    {
        return rawCameraData.m_rotation;
    }

    float RenderCamera::nearZ()
    {
        return rawCameraData.m_nearClipPlane;
    }
    
    float RenderCamera::farZ()
    {
        return rawCameraData.m_farClipPlane;
    }

    float RenderCamera::fovy() const
    {
        return rawCameraData.m_fieldOfViewY;
    }

    float RenderCamera::asepct() const
    {
        return rawCameraData.m_pixelWidth / (float)rawCameraData.m_pixelHeight;
    }

    glm::float1 RenderCamera::getWidth()
    {
        return rawCameraData.m_pixelWidth;
    }

    glm::float1 RenderCamera::getHeight()
    {
        return rawCameraData.m_pixelHeight;
    }

    glm::float3 RenderCamera::forward() const
    { 
        return (rawCameraData.m_invRotation * (-Z));
    }

    glm::float3 RenderCamera::up() const
    {
        return (rawCameraData.m_invRotation * Y);
    }

    glm::float3 RenderCamera::right() const
    {
        return (rawCameraData.m_invRotation * X);
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
                    view_matrix = rawCameraData.m_view_matrix;
                break;
            default:
                break;
        }
        return view_matrix;
    }

    glm::float4x4 RenderCamera::getProjMatrix(bool unjitter)
    {
        if (EngineConfig::g_AntialiasingMode != EngineConfig::AntialiasingMode::TAAMode || unjitter)
        {
            float actualWidth = rawCameraData.m_pixelWidth;
            float actualHeight = rawCameraData.m_pixelHeight;

            if (!rawCameraData.m_isPerspective)
            {
                rawCameraData.m_project_matrix = 
                    MoYu::MYMatrix4x4::createOrthographic(actualWidth, actualHeight, rawCameraData.m_nearClipPlane, rawCameraData.m_farClipPlane);
            }
            else
            {
                rawCameraData.m_project_matrix = getProjectionMatrix(0.0f, 0.0f);
            }
        }
        else
        {
            // The variance between 0 and the actual halton sequence values reveals noticeable
            // instability in Unity's shadow maps, so we avoid index 0.
            float jitterX = HaltonSequence::Get((taaFrameIndex & 1023) + 1, 2) - 0.5f;
            float jitterY = HaltonSequence::Get((taaFrameIndex & 1023) + 1, 3) - 0.5f;

            if (EngineConfig::g_AntialiasingMode == EngineConfig::AntialiasingMode::TAAMode)
            {
                float taaJitterScale = EngineConfig::g_TAAConfig.taaJitterScale;
                jitterX *= taaJitterScale;
                jitterY *= taaJitterScale;
            }

            float actualWidth = rawCameraData.m_pixelWidth;
            float actualHeight = rawCameraData.m_pixelHeight;

            taaJitter = glm::float4(jitterX, jitterY, jitterX / actualWidth, jitterY / actualHeight);

            if (!rawCameraData.m_isPerspective)
            {
                float vertical = actualHeight * 0.5f;
                float horizontal = actualWidth * 0.5f;

                glm::float4 offset = taaJitter;
                offset.x *= horizontal / (0.5f * actualWidth);
                offset.y *= vertical / (0.5f * actualHeight);

                float left = offset.x - horizontal;
                float right = offset.x + horizontal;
                float top = offset.y + vertical;
                float bottom = offset.y - vertical;

                rawCameraData.m_project_matrix = 
                    MoYu::MYMatrix4x4::createOrthographicOffCenter(left, right, bottom, top, rawCameraData.m_nearClipPlane, rawCameraData.m_farClipPlane);
            }
            else
            {
                rawCameraData.m_project_matrix = getProjectionMatrix(jitterX, jitterY);
            }
        }

        {
            // Analyze the projection matrix.
            // p[2][3] = (reverseZ ? 1 : -1) * (depth_0_1 ? 1 : 2) * (f * n) / (f - n)
            float n = rawCameraData.m_nearClipPlane;
            float f = rawCameraData.m_farClipPlane;
            float scale = rawCameraData.m_project_matrix[3][2] / (f * n) * (f - n);
            bool depth_0_1 = glm::abs(scale) < 1.5f;
            bool reverseZ = scale > 0;

            // http://www.humus.name/temp/Linearize%20depth.txt
            if (reverseZ)
            {
                rawCameraData.zBufferParams = glm::float4(-1 + f / n, 1, -1 / f + 1 / n, 1 / f);
            }
            else
            {
                rawCameraData.zBufferParams = glm::float4(1 - f / n, f / n, 1 / f - 1 / n, 1 / n);
            }
        }

        return rawCameraData.m_project_matrix;
    }

    glm::float4x4 RenderCamera::getLookAtMatrix()
    {
        return MoYu::MYMatrix4x4::createLookAtMatrix(position(), position() + forward(), up());
    }

    glm::float4x4 RenderCamera::getWorldToCameraMatrix()
    {
        return getViewMatrix();
    }

    glm::float4x4 RenderCamera::getCameraToWorldMatrix()
    {
        glm::float4x4 worldToCameraMat = getWorldToCameraMatrix();
        return glm::inverse(worldToCameraMat);
    }

    glm::float4 RenderCamera::getProjectionExtents(float jitterX, float jitterY)
    {
        float tAspect = rawCameraData.m_pixelWidth / (float)rawCameraData.m_pixelHeight;
        float oneExtentY = glm::tan(0.5f * MoYu::f::DEG_TO_RAD * rawCameraData.m_fieldOfViewY);
        float oneExtentX = oneExtentY * tAspect;
        float texelSizeX = oneExtentX / (0.5f * rawCameraData.m_pixelWidth);
        float texelSizeY = oneExtentY / (0.5f * rawCameraData.m_pixelHeight);
        float oneJitterX = texelSizeX * jitterX;
        float oneJitterY = texelSizeY * jitterY;

        return glm::float4(oneExtentX,
                           oneExtentY,
                           oneJitterX,
                           oneJitterY); // xy = frustum extents at distance 1, zw = jitter at distance 1
    }

    glm::float4x4 RenderCamera::getProjectionMatrix(float jitterX, float jitterY)
    {
        glm::float4 extents = this->getProjectionExtents(jitterX, jitterY);

        float cf = rawCameraData.m_farClipPlane;
        float cn = rawCameraData.m_nearClipPlane;
        float xm = extents.z - extents.x;
        float xp = extents.z + extents.x;
        float ym = extents.w - extents.y;
        float yp = extents.w + extents.y;

        return MoYu::MYMatrix4x4::createPerspectiveOffCenter(xm * cn, xp * cn, ym * cn, yp * cn, cn, cf);
    }

    static glm::float4x4 Scale(glm::float3 vector)
    {
        glm::float4x4 result;
        result[0][0] = vector.x;
        result[1][0] = 0.0f;
        result[2][0] = 0.0f;
        result[3][0] = 0.0f;
        result[0][1] = 0.0f;
        result[1][1] = vector.y;
        result[2][1] = 0.0f;
        result[3][1] = 0.0f;
        result[0][2] = 0.0f;
        result[1][2] = 0.0f;
        result[2][2] = vector.z;
        result[3][2] = 0.0f;
        result[0][3] = 0.0f;
        result[1][3] = 0.0f;
        result[2][3] = 0.0f;
        result[3][3] = 1.0f;
        return result;
    }

    glm::float4x4 RenderCamera::ComputePixelCoordToWorldSpaceViewDirectionMatrix(ViewConstants viewConstants, glm::float4 resolution, float aspect)
    {
        // Asymmetry is also possible from a user-provided projection, so we must check for it too.
        // Note however, that in case of physical camera, the lens shift term is the only source of
        // asymmetry, and this is accounted for in the optimized path below. Additionally, Unity C++ will
        // automatically disable physical camera when the projection is overridden by user.
        bool useGenericMatrix = HDUtils::IsProjectionMatrixAsymmetric(viewConstants.projMatrix);

        if (useGenericMatrix)
        {
            glm::float4x4 viewSpaceRasterTransform = glm::float4x4(
                glm::float4(2.0f * resolution.z, 0.0f, 0.0f, 0.0f),
                glm::float4(0.0f, -2.0f * resolution.w, 0.0f, 0.0f),
                glm::float4(0.0f, 0.0f, 1.0f, 0.0f),
                glm::float4(-1.0f, 1.0f, 0.0f, 1.0f));

            //glm::float4x4 transformT = viewConstants.invViewProjMatrix.transpose * Matrix4x4.Scale(new Vector3(-1.0f, -1.0f, -1.0f));

            glm::float4x4 transformT = Scale(glm::float3(-1.0f, -1.0f, -1.0f)) * viewConstants.invViewProjMatrix;

            return transformT * viewSpaceRasterTransform;
        }

        float verticalFoV = glm::atan(-1.0f / viewConstants.projMatrix[1][1]) * 2.0f;
        glm::float2 lensShift = glm::float2(0);

        return HDUtils::ComputePixelCoordToWorldSpaceViewDirectionMatrix(
            verticalFoV, lensShift, resolution, viewConstants.viewMatrix, false, aspect, rawCameraData.m_isPerspective);
    }

    void RenderCamera::GetPixelCoordToViewDirWS(glm::float4 resolution, float aspect, glm::float4x4& transforms)
    {
        transforms = ComputePixelCoordToWorldSpaceViewDirectionMatrix(mainViewConstants, resolution, aspect);
    }

} // namespace MoYu
