#include "moyu_math2.h"

namespace MoYu
{
    //------------------------------------------------------------------------------------------

    /*
     * returns n! / d!
     */
    static constexpr float factorial(size_t n, size_t d) {
       d = std::max(size_t(1), d);
       n = std::max(size_t(1), n);
       float r = 1.0;
       if (n == d) {
           // intentionally left blank
       } else if (n > d) {
           for ( ; n>d ; n--) {
               r *= n;
           }
       } else {
           for ( ; d>n ; d--) {
               r *= d;
           }
           r = 1.0f / r;
       }
       return r;
    }

    //------------------------------------------------------------------------------------------
    


    //------------------------------------------------------------------------------------------

    glm::float2 MYFloat2::Zero(0.f, 0.f);
    glm::float2 MYFloat2::One(1.f, 1.f);
    glm::float2 MYFloat2::UnitX(1.f, 0.f);
    glm::float2 MYFloat2::UnitY(0.f, 1.f);

    glm::float3 MYFloat3::Zero(0.f, 0.f, 0.f);
    glm::float3 MYFloat3::One(1.f, 1.f, 1.f);
    glm::float3 MYFloat3::UnitX(1.f, 0.f, 0.f);
    glm::float3 MYFloat3::UnitY(0.f, 1.f, 0.f);
    glm::float3 MYFloat3::UnitZ(0.f, 0.f, 1.f);
    glm::float3 MYFloat3::Up(0.f, 1.f, 0.f);
    glm::float3 MYFloat3::Down(0.f, -1.f, 0.f);
    glm::float3 MYFloat3::Right(1.f, 0.f, 0.f);
    glm::float3 MYFloat3::Left(-1.f, 0.f, 0.f);
    glm::float3 MYFloat3::Forward(0.f, 0.f, -1.f);
    glm::float3 MYFloat3::Backward(0.f, 0.f, 1.f);

    glm::float4 MYFloat4::Zero(0.f, 0.f, 0.f, 0.f);
    glm::float4 MYFloat4::One(1.f, 1.f, 1.f, 1.f);
    glm::float4 MYFloat4::UnitX(1.f, 0.f, 0.f, 0.f);
    glm::float4 MYFloat4::UnitY(0.f, 1.f, 0.f, 0.f);
    glm::float4 MYFloat4::UnitZ(0.f, 0.f, 1.f, 0.f);
    glm::float4 MYFloat4::UnitW(0.f, 0.f, 0.f, 1.f);

    glm::float3x3 MYMatrix3x3::Zero(0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f);
    glm::float3x3 MYMatrix3x3::Identity(1.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 1.f);

    glm::float4x4 MYMatrix4x4::Zero(0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f);
    glm::float4x4 MYMatrix4x4::Identity(1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f);

    glm::quat MYQuaternion::Identity(1.f, 0.f, 0.f, 0.f);

    //------------------------------------------------------------------------------------------


    float MYFloat2::signedAngle(glm::float2 v1, glm::float2 v2)
    {
       glm::float2 n1 = glm::normalize(v1);
       glm::float2 n2 = glm::normalize(v2);

       float dot = glm::dot(n1, n2);
       if (dot > 1.0f)
           dot = 1.0f;
       if (dot < -1.0f)
           dot = -1.0f;

       float theta = glm::acos(dot);
       float sgn   = glm::dot(glm::float2(-n1.y, n1.x), n2);
       if (sgn >= 0.0f)
           return theta;
       else
           return -theta;
    }

    glm::float2 MYFloat2::rotate(glm::float2 v, float theta)
    {
       float cs = glm::cos(theta);
       float sn = glm::sin(theta);
       float x1 = v.x * cs - v.y * sn;
       float y1 = v.x * sn + v.y * cs;
       return glm::float2(x1, y1);
    }

    glm::float4x4 MYMatrix4x4::createPerspectiveFieldOfView(float fovY, float aspectRatio, float zNearPlane, float zFarPlane)
    {
       glm::float4x4 m = Zero;

        m[0][0] = 1.0f / (aspectRatio * std::tan(fovY * 0.5f));
        m[1][1] = 1.0f / std::tan(fovY * 0.5f);
        m[2][2] = zNearPlane / (zFarPlane - zNearPlane);
        m[3][2] = zFarPlane * zNearPlane / (zFarPlane - zNearPlane);
        m[2][3] = -1;

        return m;
    }

    glm::float4x4 MYMatrix4x4::createPerspective(float width, float height, float zNearPlane, float zFarPlane)
    {
        return createPerspectiveOffCenter(-width * 0.5f, width * 0.5f, -height * 0.5f, height * 0.5f, zNearPlane, zFarPlane);
    }

    glm::float4x4 MYMatrix4x4::createPerspectiveOffCenter(float left, float right, float bottom, float top, float zNearPlane, float zFarPlane)
    {
        glm::float4x4 m = Zero;

        m[0][0] = 2 * zNearPlane / (right - left);
        m[2][0] = (right + left) / (right - left);
        m[1][1] = 2 * zNearPlane / (top - bottom);
        m[2][1] = (top + bottom) / (top - bottom);
        m[2][2] = zNearPlane / (zFarPlane - zNearPlane);
        m[3][2] = zFarPlane * zNearPlane / (zFarPlane - zNearPlane);
        m[2][3] = -1;

        return m;
    }

    glm::float4x4 MYMatrix4x4::createOrthographic(float width, float height, float zNearPlane, float zFarPlane)
    {
        return createOrthographicOffCenter(-width * 0.5f, width * 0.5f, -height * 0.5f, height * 0.5f, zNearPlane, zFarPlane);
    }

    glm::float4x4 MYMatrix4x4::createOrthographicOffCenter(float left, float right, float bottom, float top, float zNearPlane, float zFarPlane)
    {
        glm::float4x4 m = Zero;

        m[0][0] = 2 / (right - left);
        m[3][0] = -(right + left) / (right - left);
        m[1][1] = 2 / (top - bottom);
        m[3][1] = -(top + bottom) / (top - bottom);
        m[2][2] = 1 / (zFarPlane - zNearPlane);
        m[3][2] = zFarPlane / (zFarPlane - zNearPlane);
        m[3][3] = 1;

        return m;
    }

    glm::float4x4 MYMatrix4x4::createLookAtMatrix(const glm::float3& eye, const glm::float3& center, const glm::float3& up)
    {
        glm::mat lookAtMat = glm::lookAtRH(eye, center, up);
        return lookAtMat;
    }

    glm::float4x4 MYMatrix4x4::createViewMatrix(const glm::float3& position, const glm::quat& orientation)
    {
        glm::mat r_inverse  = glm::mat4(glm::inverse(orientation));
        glm::mat t_inverse  = glm::translate(glm::mat4(1.0), -position);
        glm::mat tr_inverse = r_inverse * t_inverse;
        return tr_inverse;
    }

    glm::float4x4 MYMatrix4x4::createWorldMatrix(const glm::float3& position, const glm::quat& orientation, const glm::float3& scale)
    {
        glm::mat t = glm::translate(glm::mat4(1.0), position);
        glm::mat r = glm::toMat4(orientation);
        glm::mat s = glm::scale(glm::mat4(1.0), scale);
        glm::mat trs = t * r * s;
        return trs;
    }

    glm::float4x4 MYMatrix4x4::makeTransform(const glm::float3& position, const glm::quat& orientation, const glm::float3& scale)
    {
        glm::mat t = glm::translate(glm::mat4(1.0), position);
        glm::mat r = glm::toMat4(orientation);
        glm::mat s = glm::scale(glm::mat4(1.0), scale);
        glm::mat trs = t * r * s;
        return trs;
    }

    glm::float4x4 MYMatrix4x4::makeInverseTransform(const glm::float3& position, const glm::quat& orientation, const glm::float3& scale)
    {
        glm::mat t_inverse = glm::translate(glm::mat4(1.0), -position);
        glm::mat r_inverse = glm::toMat4(glm::inverse(orientation));
        glm::mat s_inverse = glm::scale(glm::mat4(1.0), 1.0f / scale);
        glm::mat trs_inverse = s_inverse * r_inverse * t_inverse;
        return trs_inverse;
    }

    //------------------------------------------------------------------------------------------
    AABB::AABB(const glm::float3& minpos, const glm::float3& maxpos)
    {
        m_min = minpos;
        m_max = maxpos;
    }

    AABB::AABB(const AABB& other)
    {
        m_min = other.m_min;
        m_max = other.m_max;
    }

    void AABB::merge(const AABB& axis_aligned_box)
    { 
        merge(axis_aligned_box.getMinCorner());
        merge(axis_aligned_box.getMaxCorner());
    }

    void AABB::merge(const glm::float3& new_point)
    {
        m_min = glm::min(m_min, new_point);
        m_max = glm::max(m_max, new_point);
    }

    void AABB::update(const glm::float3& center, const glm::float3& half_extent)
    {
        m_min = center - half_extent;
        m_max = center + half_extent;
    }

    float AABB::minDistanceFromPointSq(const glm::float3& point)
    {
        float dist = 0.0f;

        if (point.x < m_min.x)
        {
           float d = point.x - m_min.x;
           dist += d * d;
        }
        else if (point.x > m_max.x)
        {
           float d = point.x - m_max.x;
           dist += d * d;
        }

        if (point.y < m_min.y)
        {
           float d = point.y - m_min.y;
           dist += d * d;
        }
        else if (point.y > m_max.y)
        {
           float d = point.y - m_max.y;
           dist += d * d;
        }

        if (point.z < m_min.z)
        {
           float d = point.z - m_min.z;
           dist += d * d;
        }
        else if (point.z > m_max.z)
        {
           float d = point.z - m_max.z;
           dist += d * d;
        }

        return dist;
    }

    float AABB::maxDistanceFromPointSq(const glm::float3& point)
    {
        float dist = 0.0f;
        float k;

        k = glm::max(fabsf(point.x - m_min.x), fabsf(point.x - m_max.x));
        dist += k * k;

        k = glm::max(fabsf(point.y - m_min.y), fabsf(point.y - m_max.y));
        dist += k * k;

        k = glm::max(fabsf(point.z - m_min.z), fabsf(point.z - m_max.z));
        dist += k * k;

        return dist;
    }

    bool AABB::isInsideSphereSq(const BSphere& other)
    {
        float radiusSq = other.radius * other.radius;
        return maxDistanceFromPointSq(other.center) <= radiusSq;
    }

    bool AABB::intersects(const BSphere& other)
    {
        float radiusSq = other.radius * other.radius;
        return minDistanceFromPointSq(other.center) <= radiusSq;
    }

    bool AABB::intersects(const AABB& other)
    {
        return !((other.m_max.x < this->m_min.x) || (other.m_min.x > this->m_max.x) || (other.m_max.y < this->m_min.y) ||
                 (other.m_min.y > this->m_max.y) || (other.m_max.z < this->m_min.z) || (other.m_min.z > this->m_max.z));
    }

    bool AABB::intersectRay(const glm::float3 rayOrigin, const glm::float3 rayDirection, float& distance)
    {
        float tmin = -FLT_MAX; // set to -FLT_MAX to get first hit on line
        float tmax = FLT_MAX;  // set to max distance ray can travel

        float _rayOrigin[]    = {rayOrigin.x, rayOrigin.y, rayOrigin.z};
        float _rayDirection[] = {rayDirection.x, rayDirection.y, rayDirection.z};
        float _min[]          = {m_min.x, m_min.y, m_min.z};
        float _max[]          = {m_max.x, m_max.y, m_max.z};

        const float EPSILON = 1e-5f;

        for (int i = 0; i < 3; i++)
        {
           if (fabsf(_rayDirection[i]) < EPSILON)
           {
               // Parallel to the plane
               if (_rayOrigin[i] < _min[i] || _rayOrigin[i] > _max[i])
               {
                   // assert( !k );
                   // hits1_false++;
                   return false;
               }
           }
           else
           {
               float ood = 1.0f / _rayDirection[i];
               float t1  = (_min[i] - _rayOrigin[i]) * ood;
               float t2  = (_max[i] - _rayOrigin[i]) * ood;

               if (t1 > t2)
                   std::swap(t1, t2);

               if (t1 > tmin)
                   tmin = t1;
               if (t2 < tmax)
                   tmax = t2;

               if (tmin > tmax)
               {
                   // assert( !k );
                   // hits1_false++;
                   return false;
               }
           }
        }

        distance = tmin;

        return true;
    }

    PlaneIntersectionFlag TestAABBToPlane(const AABB aabb, const Plane p)
    {
        glm::float3 center  = aabb.getCenter();
        glm::float3 extents = aabb.getHalfExtent();

        // Compute signed distance from plane to box center
        float sd = glm::dot(center, p.normal) - p.offset;

        // Compute the projection interval radius of b onto L(t) = b.Center + t * p.Normal
        // Projection radii r_i of the 8 bounding box vertices
        // r_i = dot((V_i - C), n)
        // r_i = dot((C +- e0*u0 +- e1*u1 +- e2*u2 - C), n)
        // Cancel C and distribute dot product
        // r_i = +-(dot(e0*u0, n)) +-(dot(e1*u1, n)) +-(dot(e2*u2, n))
        // We take the maximum position radius by taking the absolute value of the terms, we assume Extents to be
        // positive r = e0*|dot(u0, n)| + e1*|dot(u1, n)| + e2*|dot(u2, n)| When the separating axis vector Normal is
        // not a unit vector, we need to divide the radii by the length(Normal) u0,u1,u2 are the local axes of the box,
        // which is = [(1,0,0), (0,1,0), (0,0,1)] respectively for axis aligned bb
        float r = glm::dot(extents, abs(p.normal));

        if (sd > r)
        {
           return PLANE_INTERSECTION_POSITIVE_HALFSPACE;
        }
        if (sd < -r)
        {
           return PLANE_INTERSECTION_NEGATIVE_HALFSPACE;
        }
        return PLANE_INTERSECTION_INTERSECTING;
    }

    PlaneIntersectionFlag TestBSphereToPlane(const BSphere bsphere, const Plane p)
    {
        // Compute signed distance from plane to sphere center
        float sd = glm::dot(bsphere.center, p.normal) - p.offset;
        if (sd > bsphere.radius)
        {
           return PLANE_INTERSECTION_POSITIVE_HALFSPACE;
        }
        if (sd < -bsphere.radius)
        {
           return PLANE_INTERSECTION_NEGATIVE_HALFSPACE;
        }
        return PLANE_INTERSECTION_INTERSECTING;
    }

    ContainmentFlag IsFrustumContainAABB(const Frustum f, const AABB& aabb)
    {
        int  p0         = TestAABBToPlane(aabb, f.Left);
        int  p1         = TestAABBToPlane(aabb, f.Right);
        int  p2         = TestAABBToPlane(aabb, f.Bottom);
        int  p3         = TestAABBToPlane(aabb, f.Top);
        int  p4         = TestAABBToPlane(aabb, f.Near);
        int  p5         = TestAABBToPlane(aabb, f.Far);
        bool anyOutside = p0 == PLANE_INTERSECTION_NEGATIVE_HALFSPACE;
        anyOutside |= p1 == PLANE_INTERSECTION_NEGATIVE_HALFSPACE;
        anyOutside |= p2 == PLANE_INTERSECTION_NEGATIVE_HALFSPACE;
        anyOutside |= p3 == PLANE_INTERSECTION_NEGATIVE_HALFSPACE;
        anyOutside |= p4 == PLANE_INTERSECTION_NEGATIVE_HALFSPACE;
        anyOutside |= p5 == PLANE_INTERSECTION_NEGATIVE_HALFSPACE;
        bool allInside = p0 == PLANE_INTERSECTION_POSITIVE_HALFSPACE;
        allInside &= p1 == PLANE_INTERSECTION_POSITIVE_HALFSPACE;
        allInside &= p2 == PLANE_INTERSECTION_POSITIVE_HALFSPACE;
        allInside &= p3 == PLANE_INTERSECTION_POSITIVE_HALFSPACE;
        allInside &= p4 == PLANE_INTERSECTION_POSITIVE_HALFSPACE;
        allInside &= p5 == PLANE_INTERSECTION_POSITIVE_HALFSPACE;

        if (anyOutside)
        {
           return CONTAINMENT_DISJOINT;
        }

        if (allInside)
        {
           return CONTAINMENT_CONTAINS;
        }

        return CONTAINMENT_INTERSECTS;
    }

    ContainmentFlag IsFrustumContainBSphere(const Frustum f, const BSphere& bsphere)
    {
        int  p0         = TestBSphereToPlane(bsphere, f.Left);
        int  p1         = TestBSphereToPlane(bsphere, f.Right);
        int  p2         = TestBSphereToPlane(bsphere, f.Bottom);
        int  p3         = TestBSphereToPlane(bsphere, f.Top);
        int  p4         = TestBSphereToPlane(bsphere, f.Near);
        int  p5         = TestBSphereToPlane(bsphere, f.Far);
        bool anyOutside = p0 == PLANE_INTERSECTION_NEGATIVE_HALFSPACE;
        anyOutside |= p1 == PLANE_INTERSECTION_NEGATIVE_HALFSPACE;
        anyOutside |= p2 == PLANE_INTERSECTION_NEGATIVE_HALFSPACE;
        anyOutside |= p3 == PLANE_INTERSECTION_NEGATIVE_HALFSPACE;
        anyOutside |= p4 == PLANE_INTERSECTION_NEGATIVE_HALFSPACE;
        anyOutside |= p5 == PLANE_INTERSECTION_NEGATIVE_HALFSPACE;
        bool allInside = p0 == PLANE_INTERSECTION_POSITIVE_HALFSPACE;
        allInside &= p1 == PLANE_INTERSECTION_POSITIVE_HALFSPACE;
        allInside &= p2 == PLANE_INTERSECTION_POSITIVE_HALFSPACE;
        allInside &= p3 == PLANE_INTERSECTION_POSITIVE_HALFSPACE;
        allInside &= p4 == PLANE_INTERSECTION_POSITIVE_HALFSPACE;
        allInside &= p5 == PLANE_INTERSECTION_POSITIVE_HALFSPACE;

        if (anyOutside)
        {
           return CONTAINMENT_DISJOINT;
        }

        if (allInside)
        {
           return CONTAINMENT_CONTAINS;
        }

        return CONTAINMENT_INTERSECTS;
    }

    Frustum ExtractPlanesDX(const glm::float4x4 mvp)
    {
        Frustum frustum;

        // Left clipping plane
        frustum.Left.normal.x = mvp[3][0] + mvp[0][0];
        frustum.Left.normal.y = mvp[3][1] + mvp[0][1];
        frustum.Left.normal.z = mvp[3][2] + mvp[0][2];
        frustum.Left.offset   = -(mvp[3][3] + mvp[0][3]);
        // Right clipping plane
        frustum.Right.normal.x = mvp[3][0] - mvp[0][0];
        frustum.Right.normal.y = mvp[3][1] - mvp[0][1];
        frustum.Right.normal.z = mvp[3][2] - mvp[0][2];
        frustum.Right.offset   = -(mvp[3][3] - mvp[0][3]);
        // Bottom clipping plane
        frustum.Bottom.normal.x = mvp[3][0] + mvp[1][0];
        frustum.Bottom.normal.y = mvp[3][1] + mvp[1][1];
        frustum.Bottom.normal.z = mvp[3][2] + mvp[1][2];
        frustum.Bottom.offset   = -(mvp[3][3] + mvp[1][3]);
        // Top clipping plane
        frustum.Top.normal.x = mvp[3][0] - mvp[1][0];
        frustum.Top.normal.y = mvp[3][1] - mvp[1][1];
        frustum.Top.normal.z = mvp[3][2] - mvp[1][2];
        frustum.Top.offset   = -(mvp[3][3] - mvp[1][3]);
        // Far clipping plane
        frustum.Far.normal.x = mvp[2][0];
        frustum.Far.normal.y = mvp[2][1];
        frustum.Far.normal.z = mvp[2][2];
        frustum.Far.offset   = -(mvp[2][3]);
        // Near clipping plane
        frustum.Near.normal.x = mvp[3][0] - mvp[2][0];
        frustum.Near.normal.y = mvp[3][1] - mvp[2][1];
        frustum.Near.normal.z = mvp[3][2] - mvp[2][2];
        frustum.Near.offset   = -(mvp[3][3] - mvp[2][3]);

        return frustum;
    }

    Plane ComputePlane(glm::float3 a, glm::float3 b, glm::float3 c)
    {
        Plane plane;
        plane.normal = normalize(cross(b - a, c - a));
        plane.offset = dot(plane.normal, a);
        return plane;
    }

    bool BSphere::Intersects(BSphere other)
    {
        // The distance between the sphere centers is computed and compared
        // against the sum of the sphere radii. To avoid an square root operation, the
        // squared distances are compared with squared sum radii instead.
        glm::float3 d         = center - other.center;
        float       dist2     = dot(d, d);
        float       radiusSum = radius + other.radius;
        float       r2        = radiusSum * radiusSum;
        return dist2 <= r2;
    }

    //------------------------------------------------------------------------------------------

    const Color Color::White(1.f, 1.f, 1.f, 1.f);
    const Color Color::Black(0.f, 0.f, 0.f, 0.f);
    const Color Color::Red(1.f, 0.f, 0.f, 1.f);
    const Color Color::Green(0.f, 1.f, 0.f, 1.f);
    const Color Color::Blue(0.f, 1.f, 0.f, 1.f);

    Color Color::ToSRGB() const
    {
        float c[3] = {r, g, b};
        float o[3] = {r, g, b};
        for (size_t i = 0; i < 3; i++)
        {
            float _o = powf(c[i], 1.0f / 2.4f) * 1.055f - 0.055f;
            if (c[i] < 0.0031308f)
                _o = c[i] * 12.92f;
            o[i] = _o;
        }
        return Color(o[0], o[1], o[2], a);
    }

    Color Color::FromSRGB() const
    {
        float c[3] = {r, g, b};
        float o[3] = {r, g, b};
        for (size_t i = 0; i < 3; i++)
        {
            float _o = powf((c[i] + 0.055f) / 1.055f, 2.4f);
            if (c[i] < 0.0031308f)
                _o = c[i] / 12.92f;
            o[i] = _o;
        }
        return Color(o[0], o[1], o[2], a);
    }

    Color Color::ToREC709() const
    {
        float c[3] = {r, g, b};
        float o[3] = {r, g, b};
        for (size_t i = 0; i < 3; i++)
        {
            float _o = powf(c[i], 0.45f) * 1.099f - 0.099f;
            if (c[i] < 0.0018f)
                _o = c[i] * 4.5f;
            o[i] = _o;
        }
        return Color(o[0], o[1], o[2], a);
    }

    Color Color::FromREC709() const
    {
        float c[3] = {r, g, b};
        float o[3] = {r, g, b};
        for (size_t i = 0; i < 3; i++)
        {
            float _o = powf((c[i] + 0.099f) / 1.099f, 1.0f / 0.45f);
            if (c[i] < 0.0081f)
                _o = c[i] / 4.5f;
            o[i] = _o;
        }
        return Color(o[0], o[1], o[2], a);
    }

    uint32_t Color::R10G10B10A2() const
    {
        return 0;
    }

    uint32_t Color::R8G8B8A8() const
    {
        return 0;
    }

    uint32_t Color::R11G11B10F(bool RoundToEven) const
    {
        static const float kMaxVal   = float(1 << 16);
        static const float kF32toF16 = (1.0 / (1ull << 56)) * (1.0 / (1ull << 56));

        union
        {
            float    f;
            uint32_t u;
        } R, G, B;

        R.f = glm::clamp(r, 0.0f, kMaxVal) * kF32toF16;
        G.f = glm::clamp(g, 0.0f, kMaxVal) * kF32toF16;
        B.f = glm::clamp(b, 0.0f, kMaxVal) * kF32toF16;

        if (RoundToEven)
        {
            // Bankers rounding:  2.5 -> 2.0  ;  3.5 -> 4.0
            R.u += 0x0FFFF + ((R.u >> 16) & 1);
            G.u += 0x0FFFF + ((G.u >> 16) & 1);
            B.u += 0x1FFFF + ((B.u >> 17) & 1);
        }
        else
        {
            // Default rounding:  2.5 -> 3.0  ;  3.5 -> 4.0
            R.u += 0x00010000;
            G.u += 0x00010000;
            B.u += 0x00020000;
        }

        R.u &= 0x0FFE0000;
        G.u &= 0x0FFE0000;
        B.u &= 0x0FFC0000;

        return R.u >> 17 | G.u >> 6 | B.u << 4;
    }

    uint32_t Color::R9G9B9E5() const
    {
        static const float kMaxVal = float(0x1FF << 7);
        static const float kMinVal = float(1.f / (1 << 16));

        // Clamp RGB to [0, 1.FF*2^16]
        float _r = glm::clamp(r, 0.0f, kMaxVal);
        float _g = glm::clamp(g, 0.0f, kMaxVal);
        float _b = glm::clamp(b, 0.0f, kMaxVal);

        // Compute the maximum channel, no less than 1.0*2^-15
        float MaxChannel = glm::max(glm::max(_r, _g), glm::max(_b, kMinVal));

        // Take the exponent of the maximum channel (rounding up the 9th bit) and
        // add 15 to it.  When added to the channels, it causes the implicit '1.0'
        // bit and the first 8 mantissa bits to be shifted down to the low 9 bits
        // of the mantissa, rounding the truncated bits.
        union
        {
            float   f;
            int32_t i;
        } R, G, B, E;
        E.f = MaxChannel;
        E.i += 0x07804000; // Add 15 to the exponent and 0x4000 to the mantissa
        E.i &= 0x7F800000; // Zero the mantissa

        // This shifts the 9-bit values we need into the lowest bits, rounding as
        // needed.  Note that if the channel has a smaller exponent than the max
        // channel, it will shift even more.  This is intentional.
        R.f = _r + E.f;
        G.f = _g + E.f;
        B.f = _b + E.f;

        // Convert the Bias to the correct exponent in the upper 5 bits.
        E.i <<= 4;
        E.i += 0x10000000;

        // Combine the fields.  RGB floats have unwanted data in the upper 9
        // bits.  Only red needs to mask them off because green and blue shift
        // it out to the left.
        return E.i | B.i << 18 | G.i << 9 | R.i & 511;
    }

} // namespace MoYuMath