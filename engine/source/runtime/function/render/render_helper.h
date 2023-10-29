#pragma once

#include "runtime/core/math/moyu_math2.h"

namespace MoYu
{
    class RenderScene;
    class RenderCamera;

    // TODO: support cluster lighting
    struct ClusterFrustum
    {
        // we don't consider the near and far plane currently
        glm::vec4 m_plane_right;
        glm::vec4 m_plane_left;
        glm::vec4 m_plane_top;
        glm::vec4 m_plane_bottom;
        glm::vec4 m_plane_near;
        glm::vec4 m_plane_far;
    };

    struct BoundingBox
    {
        glm::float3 min_bound {std::numeric_limits<float>::max(),
                           std::numeric_limits<float>::max(),
                           std::numeric_limits<float>::max()};
        glm::float3 max_bound {std::numeric_limits<float>::min(),
                           std::numeric_limits<float>::min(),
                           std::numeric_limits<float>::min()};

        BoundingBox() {}

        BoundingBox(const glm::float3& minv, const glm::float3& maxv)
        {
            min_bound = minv;
            max_bound = maxv;
        }

        void merge(const BoundingBox& rhs)
        {
            min_bound = glm::min(min_bound, rhs.min_bound);
            max_bound = glm::max(max_bound, rhs.max_bound);
        }

        void merge(const glm::float3& point)
        {
            min_bound = glm::min(min_bound, point);
            max_bound = glm::max(max_bound, point);
        }
    };

    struct BoundingSphere
    {
        glm::vec3 m_center;
        float     m_radius;
    };

    struct FrustumPoints
    {
        glm::vec3 m_frustum_points;
    };

    ClusterFrustum CreateClusterFrustumFromMatrix(glm::mat4 mat,
                                                  float     x_left,
                                                  float     x_right,
                                                  float     y_top,
                                                  float     y_bottom,
                                                  float     z_near,
                                                  float     z_far);

    bool TiledFrustumIntersectBox(ClusterFrustum const& f, BoundingBox const& b);

    BoundingBox BoundingBoxTransform(BoundingBox const& b, glm::mat4 const& m);

    bool BoxIntersectsWithSphere(BoundingBox const& b, BoundingSphere const& s);

    //glm::mat4 CalculateDirectionalLightCamera(RenderScene& scene, RenderCamera& camera);

    namespace ibl
    {
        /*
        class float5 {
            float v[5];
        public:
            float5() = default;
            constexpr float5(float a, float b, float c, float d, float e) : v{ a, b, c, d, e } {}
            constexpr float operator[](size_t i) const { return v[i]; }
            float& operator[](size_t i) { return v[i]; }
        };

        static inline const float5 multiply(const float5 M[5], float5 x) noexcept {
            return float5{
                    M[0][0] * x[0] + M[1][0] * x[1] + M[2][0] * x[2] + M[3][0] * x[3] + M[4][0] * x[4],
                    M[0][1] * x[0] + M[1][1] * x[1] + M[2][1] * x[2] + M[3][1] * x[3] + M[4][1] * x[4],
                    M[0][2] * x[0] + M[1][2] * x[1] + M[2][2] * x[2] + M[3][2] * x[3] + M[4][2] * x[4],
                    M[0][3] * x[0] + M[1][3] * x[1] + M[2][3] * x[2] + M[3][3] * x[3] + M[4][3] * x[4],
                    M[0][4] * x[0] + M[1][4] * x[1] + M[2][4] * x[2] + M[3][4] * x[3] + M[4][4] * x[4]
            };
        };

        inline constexpr size_t SHindex(size_t m, size_t l) { return l * (l + 1) + m; }

        void ComputeShBasis(float* SHb, size_t numBands, const MoYu::Vector3& s);
        float Kml(size_t m, size_t l);
        std::vector<float> Ki(size_t numBands);
        float ComputeTruncatedCosSh(size_t l);
        MoYu::Vector3 rotateShericalHarmonicBand1(MoYu::Vector3 band1, MoYu::Matrix3x3 const& M);
        float5 rotateShericalHarmonicBand2(float5 const& band2, MoYu::Matrix3x3 const& M);
        */
    }

} // namespace MoYu
