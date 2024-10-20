#pragma once

#include "runtime/core/math/moyu_math2.h"
#include "runtime/function/render/HaltonSequence.h"
#include <mutex>

namespace MoYu
{
    struct CameraSwapData;

    enum class RenderCameraType : int
    {
        Editor,
        Motor
    };

    enum class MSAASamples : int
    {
        /// <summary>No MSAA.</summary>
        None = 1,
        /// <summary>MSAA 2X.</summary>
        MSAA2x = 2,
        /// <summary>MSAA 4X.</summary>
        MSAA4x = 4,
        /// <summary>MSAA 8X.</summary>
        MSAA8x = 8
    };

    struct ViewConstants
    {
        /// <summary>View matrix.</summary>
        glm::float4x4 viewMatrix;
        /// <summary>Inverse View matrix.</summary>
        glm::float4x4 invViewMatrix;
        /// <summary>Projection matrix.</summary>
        glm::float4x4 projMatrix;
        /// <summary>Inverse Projection matrix.</summary>
        glm::float4x4 invProjMatrix;
        /// <summary>View Projection matrix.</summary>
        glm::float4x4 viewProjMatrix;
        /// <summary>Inverse View Projection matrix.</summary>
        glm::float4x4 invViewProjMatrix;
        /// <summary>Non-jittered View Projection matrix.</summary>
        glm::float4x4 nonJitteredViewProjMatrix;
        /// <summary>Previous view matrix from previous frame.</summary>
        glm::float4x4 prevViewMatrix;
        /// <summary>Non-jittered View Projection matrix from previous frame.</summary>
        glm::float4x4 prevViewProjMatrix;
        /// <summary>Non-jittered Inverse View Projection matrix from previous frame.</summary>
        glm::float4x4 prevInvViewProjMatrix;
        /// <summary>Non-jittered View Projection matrix from previous frame without translation.</summary>
        glm::float4x4 prevViewProjMatrixNoCameraTrans;

        /// <summary>Utility matrix (used by sky) to map screen position to WS view direction.</summary>
        glm::float4x4 pixelCoordToViewDirWS;

        // We need this to track the previous VP matrix with camera translation excluded. Internal since it is used only in its "previous" form
        glm::float4x4 viewProjectionNoCameraTrans;

        /// <summary>World Space camera position.</summary>
        glm::float3 worldSpaceCameraPos;
        float pad0;
        /// <summary>Offset from the main view position for stereo view constants.</summary>
        glm::float3 worldSpaceCameraPosViewOffset;
        float pad1;
        /// <summary>World Space camera position from previous frame.</summary>
        glm::float3 prevWorldSpaceCameraPos;
        float pad2;
    };

    struct RawCameraData
    {
        glm::float3 m_position{ 0.0f, 0.0f, 0.0f };
        glm::quat   m_rotation{ MYQuaternion::Identity };
        glm::quat   m_invRotation{ MYQuaternion::Identity };

        int       m_pixelWidth{ 1366 };
        int       m_pixelHeight{ 768 };
        float     m_nearClipPlane{ 0.1f };
        float     m_farClipPlane{ 1000.0f };
        float     m_fieldOfViewY{ 90.f };
        bool      m_isPerspective{ true };

        glm::float4 zBufferParams;

        glm::float4x4 m_view_matrix{ MYMatrix4x4::Identity };
        glm::float4x4 m_project_matrix{ MYMatrix4x4::Identity };
    };

    class RenderCamera
    {
    public:
        RenderCamera(bool perspective = true);
        ~RenderCamera();

        RenderCameraType m_current_camera_type {RenderCameraType::Editor};

        long long taaFrameIndex{ 0 };
        glm::float4 taaJitter;

        /// <summary>Camera frustum.</summary>
        Frustum frustum;

        /// <summary>Number of MSAA samples used for this frame.</summary>
        MSAASamples msaaSamples = MSAASamples::None;
        /// <summary>Number of MSAA samples used for this frame.</summary>
        bool msaaEnabled = false;

        /// <summary>Flag that tracks if one of the objects that is included into the RTAS had its transform changed.</summary>
        bool transformsDirty = false;
        /// <summary>Flag that tracks if one of the objects that is included into the RTAS had its material changed.</summary>
        bool materialsDirty = false;

        /// <summary>Color pyramid history buffer state.</summary>
        bool colorPyramidHistoryIsValid = false;
        /// <summary>Volumetric history buffer state.</summary>
        bool volumetricHistoryIsValid = false;

        // Pass all the systems that may want to update per-camera data here.
        // That way you will never update an HDCamera and forget to update the dependent system.
        // NOTE: This function must be called only once per rendering (not frame, as a single camera can be rendered multiple times with different parameters during the same frame)
        // Otherwise, previous frame view constants will be wrong.
        void updatePerFrame(const CameraSwapData* pCamSwapData);

        const RawCameraData& GetRawCameraData() const;
        const ViewConstants& GetViewConstants() const;

        static const glm::float3 X, Y, Z;

        static constexpr float MIN_FOVY {10.0f};
        static constexpr float MAX_FOVY {120.0f};
        static constexpr int   MAIN_VIEW_MATRIX_INDEX {0};

        void setMainViewMatrix(const glm::float4x4& view_matrix, RenderCameraType type = RenderCameraType::Editor);
         
        void move(glm::float3 delta);
        void rotate(glm::float2 delta); // delta.x -- yaw, delta.y -- pitch
        void zoom(float offset);
        void lookAt(const glm::float3& position, const glm::float3& target, const glm::float3& up);
        void perspectiveProjection(int width, int height, float znear, float zfar, float fovy);

        bool isPerspective() const;

        void setViewport(int width, int height);

        glm::float3 position() const;
        glm::quat   rotation() const;

        float nearZ();
        float farZ();

        float fovy() const;
        float asepct() const;

        float getWidth();
        float getHeight();

        glm::float3 forward() const;
        glm::float3 up() const;
        glm::float3 right() const;

        glm::float4x4 getViewMatrix();
        glm::float4x4 getProjMatrix(bool unjitter = false);
        glm::float4x4 getLookAtMatrix();

        glm::float4x4 getWorldToCameraMatrix();
        glm::float4x4 getCameraToWorldMatrix();

        /// <summary>
        /// Compute the matrix from screen space (pixel) to world space direction (RHS).
        ///
        /// You can use this matrix on the GPU to compute the direction to look in a cubemap for a specific
        /// screen pixel.
        /// </summary>
        /// <param name="viewConstants"></param>
        /// <param name="resolution">The target texture resolution.</param>
        /// <param name="aspect">
        /// The aspect ratio to use.
        ///
        /// if negative, then the aspect ratio of <paramref name="resolution"/> will be used.
        ///
        /// It is different from the aspect ratio of <paramref name="resolution"/> for anamorphic projections.
        /// </param>
        /// <returns></returns>
        glm::float4x4 ComputePixelCoordToWorldSpaceViewDirectionMatrix(ViewConstants viewConstants, glm::float4 resolution, float aspect = -1);

        /// <param name="aspect">
        /// The aspect ratio to use.
        /// if negative, then the aspect ratio of <paramref name="resolution"/> will be used.
        /// It is different from the aspect ratio of <paramref name="resolution"/> for anamorphic projections.
        /// </param>
        void GetPixelCoordToViewDirWS(glm::float4 resolution, float aspect, glm::float4x4& transforms);

    protected:
        glm::float4   getProjectionExtents(float jitterX = 0, float jitterY = 0);
        glm::float4x4 getProjectionMatrix(float jitterX, float jitterY);

        // <summary>View constants.</summary>
        ViewConstants mainViewConstants;

        // 直接从swapdata传入的相机需要的数据
        RawCameraData rawCameraData;

        std::mutex m_view_matrix_mutex;
    };

    inline const glm::float3 RenderCamera::X = {1.0f, 0.0f, 0.0f};
    inline const glm::float3 RenderCamera::Y = {0.0f, 1.0f, 0.0f};
    inline const glm::float3 RenderCamera::Z = {0.0f, 0.0f, 1.0f};

} // namespace MoYu