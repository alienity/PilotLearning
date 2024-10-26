#pragma once

#include "runtime/core/math/moyu_math2.h"

namespace MoYu
{

	class HDUtils
	{
    public:

		// Get the aspect ratio of a projection matrix
		inline static float ProjectionMatrixAspect(glm::float4x4 matrix)
		{
			return -matrix[1][1] / matrix[0][0];
		}

		// Determine if a projection matrix is off-center
		inline static bool IsProjectionMatrixAsymmetric(glm::float4x4 matrix)
		{
			return matrix[2][0] != 0 || matrix[2][1] != 0;
		}

        inline static glm::float4x4 ComputePixelCoordToWorldSpaceViewDirectionMatrix(
            float verticalFoV, glm::float2 lensShift, glm::float4 screenSize, glm::float4x4 worldToViewMatrix, 
            bool renderToCubemap, float aspectRatio = -1, bool isOrthographic = false)
        {
            glm::float4x4 viewSpaceRasterTransform;

            if (isOrthographic)
            {
                // For ortho cameras, project the skybox with no perspective
                // the same way as builtin does (case 1264647)
                viewSpaceRasterTransform = glm::float4x4(
                    glm::float4(-2.0f * screenSize.z, 0.0f, 1.0f, 0.0f),
                    glm::float4(0.0f, -2.0f * screenSize.w, 1.0f, 0.0f),
                    glm::float4(0.0f, 0.0f, -1.0f, 0.0f),
                    glm::float4(0.0f, 0.0f, 0.0f, 0.0f));
            }
            else
            {
                // Compose the view space version first.
                // V = -(X, Y, Z), s.t. Z = 1,
                // X = (2x / resX - 1) * tan(vFoV / 2) * ar = x * [(2 / resX) * tan(vFoV / 2) * ar] + [-tan(vFoV / 2) * ar] = x * [-m00] + [-m20]
                // Y = (2y / resY - 1) * tan(vFoV / 2)      = y * [(2 / resY) * tan(vFoV / 2)]      + [-tan(vFoV / 2)]      = y * [-m11] + [-m21]

                aspectRatio = aspectRatio < 0 ? screenSize.x * screenSize.w : aspectRatio;
                float tanHalfVertFoV = glm::tan(0.5f * verticalFoV);

                // Compose the matrix.
                float m12 = (1.0f - 2.0f * lensShift.y) * tanHalfVertFoV;
                float m11 = -2.0f * screenSize.w * tanHalfVertFoV;

                float m02 = (1.0f - 2.0f * lensShift.x) * tanHalfVertFoV * aspectRatio;
                float m00 = -2.0f * screenSize.z * tanHalfVertFoV * aspectRatio;

                if (renderToCubemap)
                {
                    // Flip Y.
                    m11 = -m11;
                    m12 = -m12;
                }

                viewSpaceRasterTransform = glm::float4x4(
                    glm::float4(m00, 0.0f, m02, 0.0f),
                    glm::float4(0.0f, m11, m12, 0.0f),
                    glm::float4(0.0f, 0.0f, -1.0f, 0.0f),
                    glm::float4(0.0f, 0.0f, 0.0f, 1.0f));
            }

            // Remove the translation component.
            glm::float4 homogeneousZero = glm::float4(0, 0, 0, 1);
            worldToViewMatrix[3] = homogeneousZero;

            // Flip the Z to make the coordinate system left-handed.
            //worldToViewMatrix.SetRow(2, -worldToViewMatrix.GetRow(2));

            //// Transpose for HLSL.
            //return Matrix4x4.Transpose(worldToViewMatrix.transpose * viewSpaceRasterTransform);

            return glm::transpose(worldToViewMatrix) * viewSpaceRasterTransform;
        }

        inline static float ComputZPlaneTexelSpacing(float planeDepth, float verticalFoV, float resolutionY)
        {
            float tanHalfVertFoV = glm::tan(0.5f * verticalFoV);
            return tanHalfVertFoV * (2.0f / resolutionY) * planeDepth;
        }

        inline static int DivRoundUp(int x, int y)
        {
            return (x + y - 1) / y;
        }

        inline static glm::ivec2 DivRoundUp(glm::ivec2 n, int d)
        {
            return glm::ivec2(HDUtils::DivRoundUp(n.x, d), HDUtils::DivRoundUp(n.y, d));
        }

        inline static glm::ivec2 DivRoundUp(glm::ivec2 n, glm::ivec2 d)
        {
            return glm::ivec2(HDUtils::DivRoundUp(n.x, d.x), HDUtils::DivRoundUp(n.y, d.y));
        }

        inline static glm::ivec3 DivRoundUp(glm::ivec3 n, int d)
        {
            return glm::ivec3(HDUtils::DivRoundUp(n.x, d), HDUtils::DivRoundUp(n.y, d), HDUtils::DivRoundUp(n.z, d));
        }

        inline static bool IsQuaternionValid(glm::quat q)
        {
            return (q[0] * q[0] + q[1] * q[1] + q[2] * q[2] + q[3] * q[3]) > 1.401298E-45F;
        }

        inline static float ComputeViewportScale(int viewportSize, int bufferSize)
        {
            float rcpBufferSize = 1.0f / bufferSize;

            // Scale by (vp_dim / buf_dim).
            return viewportSize * rcpBufferSize;
        }

        inline static float ComputeViewportLimit(int viewportSize, int bufferSize)
        {
            float rcpBufferSize = 1.0f / bufferSize;

            // Clamp to (vp_dim - 0.5) / buf_dim.
            return (viewportSize - 0.5f) * rcpBufferSize;
        }

        inline static glm::vec4 ComputeViewportScaleAndLimit(glm::ivec2 viewportSize, glm::ivec2 bufferSize)
        {
            return glm::vec4(
                ComputeViewportScale(viewportSize.x, bufferSize.x),  // Scale(x)
                ComputeViewportScale(viewportSize.y, bufferSize.y),  // Scale(y)
                ComputeViewportLimit(viewportSize.x, bufferSize.x),  // Limit(x)
                ComputeViewportLimit(viewportSize.y, bufferSize.y)); // Limit(y)
        }




	};


} // namespace MoYu
