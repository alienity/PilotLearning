#include "runtime/function/render/render_helper.h"
#include "runtime/function/render/render_camera.h"
#include "runtime/function/render/render_scene.h"

namespace MoYu
{
    ClusterFrustum CreateClusterFrustumFromMatrix(glm::mat4 mat,
                                                  float     x_left,
                                                  float     x_right,
                                                  float     y_top,
                                                  float     y_bottom,
                                                  float     z_near,
                                                  float     z_far)
    {
        ClusterFrustum f;

        // the following is in the vulkan space
        // note that the Y axis is flipped in Vulkan
        assert(y_top < y_bottom);

        assert(x_left < x_right);
        assert(z_near < z_far);

        // calculate the tiled frustum
        // [Fast Extraction of Viewing Frustum Planes from the WorldView - Projection
        // Matrix](http://gamedevs.org/uploads/fast-extraction-viewing-frustum-planes-from-world-view-projection-matrix.pdf)

        // glm uses column vector and column major
        glm::mat4 mat_column = glm::transpose(mat);

        // [vec.xyz 1][mat.col0] / [vec.xyz 1][mat.col3] > x_right
        // [vec.xyz 1][mat.col0 - mat.col3*x_right] > 0
        f.m_plane_right = mat_column[0] - (mat_column[3] * x_right);
        // normalize
        f.m_plane_right *= (1.0 / glm::length(glm::vec3(f.m_plane_right.x, f.m_plane_right.y, f.m_plane_right.z)));

        // for example, we try to calculate the "plane_left" of the tile frustum
        // note that we use the row vector to be consistent with the DirectXMath
        // [vec.xyz 1][mat.col0] / [vec.xyz 1][mat.col3] < x_left
        //
        // evidently, the value "[vec.xyz 1][mat.col3]" is expected to be greater than
        // 0 (w of clip space) and we multiply both sides by "[vec.xyz 1][mat.col3]",
        // the inequality symbol remains the same [vec.xyz 1][mat.col0] < [vec.xyz
        // 1][mat.col3]*x_left
        //
        // since "x_left" is a scalar, the "scalar multiplication" is applied
        // [vec.xyz 1][mat.col0] < [vec.xyz 1][mat.col3*x_left]
        // [vec.xyz 1][mat.col0 - mat.col3*x_left] < 0
        //
        // we follow the "DirectX::BoundingFrustum::Intersects", the normal of the
        // plane is pointing ourward [vec.xyz 1][mat.col3*x_left - mat.col0] > 0
        //
        // the plane can be defined as [x y z 1][A B C D] = 0 and the [A B C D] is
        // exactly [mat.col0 - mat.col3*x_left] and we need to normalize the normal[A
        // B C] of the plane let [A B C D] = [mat.col3*x_left - mat.col0] [A B C D] /=
        // length([A B C].xyz)
        f.m_plane_left = (mat_column[3] * x_left) - mat_column[0];
        // normalize
        f.m_plane_left *= (1.0 / glm::length(glm::vec3(f.m_plane_left.x, f.m_plane_left.y, f.m_plane_left.z)));

        // [vec.xyz 1][mat.col1] / [vec.xyz 1][mat.col3] < y_top
        // [vec.xyz 1][mat.col3*y_top - mat.col1] > 0
        f.m_plane_top = (mat_column[3] * y_top) - mat_column[1];
        // normalize
        f.m_plane_top *= (1.0 / glm::length(glm::vec3(f.m_plane_top.x, f.m_plane_top.y, f.m_plane_top.z)));

        // [vec.xyz 1][mat.col1] / [vec.xyz 1][mat.col3] > y_bottom
        // [vec.xyz 1][mat.col1 - mat.col3*y_bottom] > 0
        f.m_plane_bottom = mat_column[1] - (mat_column[3] * y_bottom);
        // normalize
        f.m_plane_bottom *= (1.0 / glm::length(glm::vec3(f.m_plane_bottom.x, f.m_plane_bottom.y, f.m_plane_bottom.z)));

        // [vec.xyz 1][mat.col2] / [vec.xyz 1][mat.col3] < z_near
        // [vec.xyz 1][mat.col3*z_near - mat.col2] > 0
        f.m_plane_near = (mat_column[3] * z_near) - mat_column[2];
        f.m_plane_near *= (1.0 / glm::length(glm::vec3(f.m_plane_near.x, f.m_plane_near.y, f.m_plane_near.z)));

        // [vec.xyz 1][mat.col2] / [vec.xyz 1][mat.col3] > z_far
        // [vec.xyz 1][mat.col2 - mat.col3*z_far] > 0
        f.m_plane_far = mat_column[2] - (mat_column[3] * z_far);
        f.m_plane_far *= (1.0 / glm::length(glm::vec3(f.m_plane_far.x, f.m_plane_far.y, f.m_plane_far.z)));

        return f;
    }

    bool TiledFrustumIntersectBox(ClusterFrustum const& f, BoundingBox const& b)
    {
        // we follow the "DirectX::BoundingFrustum::Intersects"

        // Center of the box.
        glm::vec4 box_center((b.max_bound.x + b.min_bound.x) * 0.5,
                             (b.max_bound.y + b.min_bound.y) * 0.5,
                             (b.max_bound.z + b.min_bound.z) * 0.5,
                             1.0);

        // Distance from the center to each side.
        // half extent //more exactly
        glm::vec3 box_extents((b.max_bound.x - b.min_bound.x) * 0.5,
                              (b.max_bound.y - b.min_bound.y) * 0.5,
                              (b.max_bound.z - b.min_bound.z) * 0.5);

        // plane_right
        {
            float signed_distance_from_plane_right = glm::dot(f.m_plane_right, box_center);
            float radius_project_plane_right =
                glm::dot(glm::abs(glm::vec3(f.m_plane_right.x, f.m_plane_right.y, f.m_plane_right.z)), box_extents);

            bool intersecting_or_inside_right = signed_distance_from_plane_right < radius_project_plane_right;
            if (!intersecting_or_inside_right)
            {
                return false;
            }
        }

        // plane_left
        {
            float signed_distance_from_plane_left = glm::dot(f.m_plane_left, box_center);
            float radius_project_plane_left =
                glm::dot(glm::abs(glm::vec3(f.m_plane_left.x, f.m_plane_left.y, f.m_plane_left.z)), box_extents);

            bool intersecting_or_inside_left = signed_distance_from_plane_left < radius_project_plane_left;
            if (!intersecting_or_inside_left)
            {
                return false;
            }
        }

        // plane_top
        {
            float signed_distance_from_plane_top = glm::dot(f.m_plane_top, box_center);
            float radius_project_plane_top =
                glm::dot(glm::abs(glm::vec3(f.m_plane_top.x, f.m_plane_top.y, f.m_plane_top.z)), box_extents);

            bool intersecting_or_inside_top = signed_distance_from_plane_top < radius_project_plane_top;
            if (!intersecting_or_inside_top)
            {
                return false;
            }
        }

        // plane_bottom
        {
            float signed_distance_from_plane_bottom = glm::dot(f.m_plane_bottom, box_center);
            float radius_project_plane_bottom =
                glm::dot(glm::abs(glm::vec3(f.m_plane_bottom.x, f.m_plane_bottom.y, f.m_plane_bottom.z)), box_extents);

            bool intersecting_or_inside_bottom = signed_distance_from_plane_bottom < radius_project_plane_bottom;
            if (!intersecting_or_inside_bottom)
            {
                return false;
            }
        }

        // plane_near
        {
            float signed_distance_from_plane_near = glm::dot(f.m_plane_near, box_center);
            float radius_project_plane_near =
                glm::dot(glm::abs(glm::vec3(f.m_plane_near.x, f.m_plane_near.y, f.m_plane_near.z)), box_extents);

            bool intersecting_or_inside_near = signed_distance_from_plane_near < radius_project_plane_near;
            if (!intersecting_or_inside_near)
            {
                return false;
            }
        }

        // plane_far
        {
            float signed_distance_from_plane_far = glm::dot(f.m_plane_far, box_center);
            float radius_project_plane_far =
                glm::dot(glm::abs(glm::vec3(f.m_plane_far.x, f.m_plane_far.y, f.m_plane_far.z)), box_extents);

            bool intersecting_or_inside_far = signed_distance_from_plane_far < radius_project_plane_far;
            if (!intersecting_or_inside_far)
            {
                return false;
            }
        }

        return true;
    }

    BoundingBox BoundingBoxTransform(BoundingBox const& b, glm::mat4 const& m)
    {
        // we follow the "BoundingBox::Transform"

        glm::vec3 const g_BoxOffset[8] = {glm::vec3(-1.0f, -1.0f, 1.0f),
                                          glm::vec3(1.0f, -1.0f, 1.0f),
                                          glm::vec3(1.0f, 1.0f, 1.0f),
                                          glm::vec3(-1.0f, 1.0f, 1.0f),
                                          glm::vec3(-1.0f, -1.0f, -1.0f),
                                          glm::vec3(1.0f, -1.0f, -1.0f),
                                          glm::vec3(1.0f, 1.0f, -1.0f),
                                          glm::vec3(-1.0f, 1.0f, -1.0f)};

        size_t const CORNER_COUNT = 8;

        // Load center and extents.
        // Center of the box.
        glm::vec3 center((b.max_bound.x + b.min_bound.x) * 0.5,
                         (b.max_bound.y + b.min_bound.y) * 0.5,
                         (b.max_bound.z + b.min_bound.z) * 0.5);

        // Distance from the center to each side.
        // half extent //more exactly
        glm::vec3 extents((b.max_bound.x - b.min_bound.x) * 0.5,
                          (b.max_bound.y - b.min_bound.y) * 0.5,
                          (b.max_bound.z - b.min_bound.z) * 0.5);

        glm::vec3 min;
        glm::vec3 max;

        // Compute and transform the corners and find new min/max bounds.
        for (size_t i = 0; i < CORNER_COUNT; ++i)
        {
            glm::vec3 corner_before = extents * g_BoxOffset[i] + center;
            glm::vec4 corner_with_w = m * glm::vec4(corner_before.x, corner_before.y, corner_before.z, 1.0);
            glm::vec3 corner        = glm::vec3(corner_with_w.x / corner_with_w.w,
                                         corner_with_w.y / corner_with_w.w,
                                         corner_with_w.z / corner_with_w.w);

            if (0 == i)
            {
                min = corner;
                max = corner;
            }
            else
            {
                min = glm::min(min, corner);
                max = glm::max(max, corner);
            }
        }

        BoundingBox b_out;
        b_out.max_bound = max;
        b_out.min_bound = min;
        return b_out;
    }

    bool BoxIntersectsWithSphere(BoundingBox const& b, BSphere const& s)
    {
        for (size_t i = 0; i < 3; ++i)
        {
            if (s.center[i] < b.min_bound[i])
            {
                if ((b.min_bound[i] - s.center[i]) > s.radius)
                {
                    return false;
                }
            }
            else if (s.center[i] > b.max_bound[i])
            {
                if ((s.center[i] - b.max_bound[i]) > s.radius)
                {
                    return false;
                }
            }
        }

        return true;
    }
    
    // https://forum.unity.com/threads/decodedepthnormal-linear01depth-lineareyedepth-explanations.608452/
    //
    //// Z buffer to linear depth
    // inline float LinearEyeDepth( float z )
    //{
    //     return 1.0 / (_ZBufferParams.z * z + _ZBufferParams.w);
    // }
    //
    //  Values used to linearize the Z buffer (http://www.humus.name/temp/Linearize%20depth.txt)
    //  x = 1-far/near
    //  y = far/near
    //  z = x/far
    //  w = y/far
    //  or in case of a reversed depth buffer (UNITY_REVERSED_Z is 1)
    //  x = -1+far/near
    //  y = 1
    //  z = x/far
    //  w = 1/far
    glm::float4 CalculateZBufferParams(float nearClipPlane, float farClipPlane)
    {
        float fpn = farClipPlane / nearClipPlane;
        return glm::float4(fpn - 1.0f, 1.0f, (fpn - 1.0f) / farClipPlane, 1.0f / farClipPlane);
    }


    int m_SampleIndex = 0;
    const int k_SampleCount = 8;

    float GetHaltonValue(int index, int radix)
    {
        float result   = 0.0f;
        float fraction = 1.0f / (float)radix;

        while (index > 0)
        {
            result += (float)(index % radix) * fraction;

            index /= radix;
            fraction /= (float)radix;
        }

        return result;
    }

    glm::float2 GenerateRandomOffset()
    {
        glm::float2 offset =
            glm::float2(GetHaltonValue(m_SampleIndex & 1023, 2), GetHaltonValue(m_SampleIndex & 1023, 3));

        if (++m_SampleIndex >= k_SampleCount)
            m_SampleIndex = 0;

        return offset;
    }


    /*
    glm::mat4 CalculateDirectionalLightCamera(RenderScene& scene, RenderCamera& camera)
    {
        Matrix4x4 proj_view_matrix;
        {
            Matrix4x4 view_matrix = camera.getViewMatrix();
            Matrix4x4 proj_matrix = camera.getPersProjMatrix();
            proj_view_matrix      = proj_matrix * view_matrix;
        }

        BoundingBox frustum_bounding_box;
        // CascadedShadowMaps11 / CreateFrustumPointsFromCascadeInterval
        {
            glm::vec3 const g_frustum_points_ndc_space[8] = {glm::vec3(-1.0f, -1.0f, 1.0f),
                                                             glm::vec3(1.0f, -1.0f, 1.0f),
                                                             glm::vec3(1.0f, 1.0f, 1.0f),
                                                             glm::vec3(-1.0f, 1.0f, 1.0f),
                                                             glm::vec3(-1.0f, -1.0f, 0.0f),
                                                             glm::vec3(1.0f, -1.0f, 0.0f),
                                                             glm::vec3(1.0f, 1.0f, 0.0f),
                                                             glm::vec3(-1.0f, 1.0f, 0.0f)};

            glm::mat4 inverse_proj_view_matrix = glm::inverse(GLMUtil::FromMat4x4(proj_view_matrix));

            frustum_bounding_box.min_bound = Vector3(FLT_MAX, FLT_MAX, FLT_MAX);
            frustum_bounding_box.max_bound = Vector3(FLT_MIN, FLT_MIN, FLT_MIN);

            size_t const CORNER_COUNT = 8;
            for (size_t i = 0; i < CORNER_COUNT; ++i)
            {
                glm::vec4 frustum_point_with_w = inverse_proj_view_matrix * glm::vec4(g_frustum_points_ndc_space[i].x,
                                                                                      g_frustum_points_ndc_space[i].y,
                                                                                      g_frustum_points_ndc_space[i].z,
                                                                                      1.0);
                Vector3   frustum_point        = Vector3(frustum_point_with_w.x / frustum_point_with_w.w,
                                                frustum_point_with_w.y / frustum_point_with_w.w,
                                                frustum_point_with_w.z / frustum_point_with_w.w);

                frustum_bounding_box.merge(frustum_point);
            }
        }

        BoundingBox scene_bounding_box;
        {
            scene_bounding_box.min_bound = Vector3(FLT_MAX, FLT_MAX, FLT_MAX);
            scene_bounding_box.max_bound = Vector3(FLT_MIN, FLT_MIN, FLT_MIN);

            for (const InternalMeshRenderer& entity : scene.m_mesh_renderers)
            {
                BoundingBox mesh_asset_bounding_box {entity.m_bounding_box.getMinCorner(), entity.m_bounding_box.getMaxCorner()};
                BoundingBox mesh_bounding_box_world = BoundingBoxTransform(mesh_asset_bounding_box, GLMUtil::FromMat4x4(entity.model_matrix));
                scene_bounding_box.merge(mesh_bounding_box_world);
            }
        }

        // CascadedShadowMaps11 / ComputeNearAndFar
        glm::mat4 light_view;
        glm::mat4 light_proj;
        {
            glm::vec3 box_center((frustum_bounding_box.max_bound.x + frustum_bounding_box.min_bound.x) * 0.5,
                                 (frustum_bounding_box.max_bound.y + frustum_bounding_box.min_bound.y) * 0.5,
                                 (frustum_bounding_box.max_bound.z + frustum_bounding_box.min_bound.z));
            glm::vec3 box_extents((frustum_bounding_box.max_bound.x - frustum_bounding_box.min_bound.x) * 0.5,
                                  (frustum_bounding_box.max_bound.y - frustum_bounding_box.min_bound.y) * 0.5,
                                  (frustum_bounding_box.max_bound.z - frustum_bounding_box.min_bound.z) * 0.5);

            glm::vec3 eye =
                box_center + GLMUtil::FromVec3(scene.m_directional_light.m_direction) * glm::length(box_extents);
            glm::vec3 center = box_center;
            light_view       = glm::lookAtRH(eye, center, glm::vec3(0.0, 1.0, 0.0));

            BoundingBox frustum_bounding_box_light_view = BoundingBoxTransform(frustum_bounding_box, light_view);
            BoundingBox scene_bounding_box_light_view   = BoundingBoxTransform(scene_bounding_box, light_view);

            light_proj = glm::orthoRH(
                std::max(frustum_bounding_box_light_view.min_bound.x, scene_bounding_box_light_view.min_bound.x),
                std::min(frustum_bounding_box_light_view.max_bound.x, scene_bounding_box_light_view.max_bound.x),
                std::max(frustum_bounding_box_light_view.min_bound.y, scene_bounding_box_light_view.min_bound.y),
                std::min(frustum_bounding_box_light_view.max_bound.y, scene_bounding_box_light_view.max_bound.y),
                -scene_bounding_box_light_view.max_bound.z, // the objects which are nearer than the frustum bounding box may caster shadow as well
                -std::max(frustum_bounding_box_light_view.min_bound.z, scene_bounding_box_light_view.min_bound.z));
        }

        glm::mat4 light_proj_view = (light_proj * light_view);
        return light_proj_view;
    }
    */

    namespace ibl
    {
        ///*
        ///*
        // * SH scaling factors:
        // *  returns sqrt((2*l + 1) / 4*pi) * sqrt( (l-|m|)! / (l+|m|)! )
        // */
        //float Kml(size_t m, size_t l)
        //{
        //    m = m < 0 ? -m : m;  // abs() is not constexpr
        //    const float K = (2 * l + 1) * factorial(size_t(l - m), size_t(l + m));
        //    return std::sqrt(K) * (F_2_SQRTPI * 0.25);
        //}

        //std::vector<float> Ki(size_t numBands)
        //{
        //    const size_t numCoefs = numBands * numBands;
        //    std::vector<float> K(numCoefs);
        //    for (size_t l = 0; l < numBands; l++) {
        //        K[SHindex(0, l)] = Kml(0, l);
        //        for (size_t m = 1; m <= l; m++) {
        //            K[SHindex(m, l)] =
        //            K[SHindex(-m, l)] = F_SQRT2 * Kml(m, l);
        //        }
        //    }
        //    return K;
        //}

        //// < cos(theta) > SH coefficients pre-multiplied by 1 / K(0,l)
        //float ComputeTruncatedCosSh(size_t l)
        //{
        //    if (l == 0) {
        //        return F_PI;
        //    } else if (l == 1) {
        //        return 2 * F_PI / 3;
        //    } else if (l & 1u) {
        //        return 0;
        //    }
        //    const size_t l_2 = l / 2;
        //    float A0 = ((l_2 & 1u) ? 1.0f : -1.0f) / ((l + 2) * (l - 1));
        //    float A1 = factorial(l, l_2) / (factorial(l_2) * (1 << l));
        //    return 2 * F_PI * A0 * A1;
        //}

        ///*
        // * Calculates non-normalized SH bases, i.e.:
        // *  m > 0, cos(m*phi)   * P(m,l)
        // *  m < 0, sin(|m|*phi) * P(|m|,l)
        // *  m = 0, P(0,l)
        // */
        //void ComputeShBasis(float* SHb, size_t numBands, const MoYu::Vector3& s)
        //{
        //    /*
        //     * TODO: all the Legendre computation below is identical for all faces, so it
        //     * might make sense to pre-compute it once. Also note that there is
        //     * a fair amount of symmetry within a face (which we could take advantage of
        //     * to reduce the pre-compute table).
        //     */

        //    /*
        //     * Below, we compute the associated Legendre polynomials using recursion.
        //     * see: http://mathworld.wolfram.com/AssociatedLegendrePolynomial.html
        //     *
        //     * Note [0]: s.z == cos(theta) ==> we only need to compute P(s.z)
        //     *
        //     * Note [1]: We in fact compute P(s.z) / sin(theta)^|m|, by removing
        //     * the "sqrt(1 - s.z*s.z)" [i.e.: sin(theta)] factor from the recursion.
        //     * This is later corrected in the ( cos(m*phi), sin(m*phi) ) recursion.
        //     */

        //    // s = (x, y, z) = (sin(theta)*cos(phi), sin(theta)*sin(phi), cos(theta))

        //    // handle m=0 separately, since it produces only one coefficient
        //    float Pml_2 = 0;
        //    float Pml_1 = 1;
        //    SHb[0] =  Pml_1;
        //    for (size_t l=1; l<numBands; l++) {
        //        float Pml = ((2*l-1.0f)*Pml_1*s.z - (l-1.0f)*Pml_2) / l;
        //        Pml_2 = Pml_1;
        //        Pml_1 = Pml;
        //        SHb[SHindex(0, l)] = Pml;
        //    }
        //    float Pmm = 1;
        //    for (size_t m=1 ; m<numBands ; m++) {
        //        Pmm = (1.0f - 2*m) * Pmm;      // See [1], divide by sqrt(1 - s.z*s.z);
        //        Pml_2 = Pmm;
        //        Pml_1 = (2*m + 1.0f)*Pmm*s.z;
        //        // l == m
        //        SHb[SHindex(-m, m)] = Pml_2;
        //        SHb[SHindex( m, m)] = Pml_2;
        //        if (m+1 < numBands) {
        //            // l == m+1
        //            SHb[SHindex(-m, m+1)] = Pml_1;
        //            SHb[SHindex( m, m+1)] = Pml_1;
        //            for (size_t l=m+2 ; l<numBands ; l++) {
        //                float Pml = ((2*l - 1.0f)*Pml_1*s.z - (l + m - 1.0f)*Pml_2) / (l-m);
        //                Pml_2 = Pml_1;
        //                Pml_1 = Pml;
        //                SHb[SHindex(-m, l)] = Pml;
        //                SHb[SHindex( m, l)] = Pml;
        //            }
        //        }
        //    }

        //    // At this point, SHb contains the associated Legendre polynomials divided
        //    // by sin(theta)^|m|. Below we compute the SH basis.
        //    //
        //    // ( cos(m*phi), sin(m*phi) ) recursion:
        //    // cos(m*phi + phi) == cos(m*phi)*cos(phi) - sin(m*phi)*sin(phi)
        //    // sin(m*phi + phi) == sin(m*phi)*cos(phi) + cos(m*phi)*sin(phi)
        //    // cos[m+1] == cos[m]*s.x - sin[m]*s.y
        //    // sin[m+1] == sin[m]*s.x + cos[m]*s.y
        //    //
        //    // Note that (d.x, d.y) == (cos(phi), sin(phi)) * sin(theta), so the
        //    // code below actually evaluates:
        //    //      (cos((m*phi), sin(m*phi)) * sin(theta)^|m|
        //    float Cm = s.x;
        //    float Sm = s.y;
        //    for (size_t m = 1; m <= numBands; m++) {
        //        for (size_t l = m; l < numBands; l++) {
        //            SHb[SHindex(-m, l)] *= Sm;
        //            SHb[SHindex( m, l)] *= Cm;
        //        }
        //        float Cm1 = Cm * s.x - Sm * s.y;
        //        float Sm1 = Sm * s.x + Cm * s.y;
        //        Cm = Cm1;
        //        Sm = Sm1;
        //    }
        //}



    }

    

} // namespace MoYu
