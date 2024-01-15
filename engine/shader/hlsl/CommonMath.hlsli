
#ifndef __COMMON_MATH_HLSLI__
#define __COMMON_MATH_HLSLI__

//------------------------------------------------------------------------------
// Common math
//------------------------------------------------------------------------------

static const float F_E        = 2.7182818284590;
static const float F_LOG2E = 1.4426950408889;
static const float F_LOG10E = 0.4342944819032;
static const float F_LN2      = 0.6931471805599;
static const float F_LN10     = 2.3025850929940;
static const float F_PI       = 3.1415926535897;
static const float F_PI_2     = 1.5707963267948;
static const float F_PI_4     = 0.7853981633974;
static const float F_1_PI     = 0.3183098861837;
static const float F_2_PI     = 0.6366197723675;
static const float F_2_SQRTPI = 1.1283791670955;
static const float F_SQRT2    = 1.4142135623730;
static const float F_SQRT1_2  = 0.7071067811865;
static const float F_TAU      = 2.0 * F_PI;

#define MILLIMETERS_PER_METER 1000
#define METERS_PER_MILLIMETER rcp(MILLIMETERS_PER_METER)
#define CENTIMETERS_PER_METER 100
#define METERS_PER_CENTIMETER rcp(CENTIMETERS_PER_METER)

#define PI          3.14159265358979323846
#define TWO_PI      6.28318530717958647693
#define FOUR_PI     12.5663706143591729538
#define INV_PI      0.31830988618379067154
#define INV_TWO_PI  0.15915494309189533577
#define INV_FOUR_PI 0.07957747154594766788
#define HALF_PI     1.57079632679489661923
#define INV_HALF_PI 0.63661977236758134308
#define LOG2_E      1.44269504088896340736
#define INV_SQRT2   0.70710678118654752440
#define PI_DIV_FOUR 0.78539816339744830961

#define FLT_INF  asfloat(0x7F800000)
#define FLT_EPS  5.960464478e-8  // 2^-24, machine epsilon: 1 + EPS = 1 (half of the ULP for 1.0f)
#define FLT_MIN  1.175494351e-38 // Minimum normalized positive floating-point number
#define FLT_MAX  3.402823466e+38 // Maximum representable floating-point number
#define HALF_EPS 4.8828125e-4    // 2^-11, machine epsilon: 1 + EPS = 1 (half of the ULP for 1.0f)
#define HALF_MIN 6.103515625e-5  // 2^-14, the same value for 10, 11 and 16-bit: https://www.khronos.org/opengl/wiki/Small_Float_Formats
#define HALF_MIN_SQRT 0.0078125  // 2^-7 == sqrt(HALF_MIN), useful for ensuring HALF_MIN after x^2
#define HALF_MAX 65504.0
#define UINT_MAX 0xFFFFFFFFu
#define INT_MAX  0x7FFFFFFF

//------------------------------------------------------------------------------
// Scalar operations
//------------------------------------------------------------------------------

// Computes x^5 using only multiply operations.
float pow5(float x)
{
    const float x2 = x * x;
    return x2 * x2 * x;
}

float pow6(float x)
{
    const float x2 = x * x;
    return x2 * x2 * x2;
}

// Computes x^2 as a single multiplication.
float sq(float x)
{
    return x * x;
}

//------------------------------------------------------------------------------
// Vector operations
//------------------------------------------------------------------------------

// Returns the maximum component of the specified vector.
float max3(const float3 v) { return max(v.x, max(v.y, v.z)); }

float fmax(const float2 v) { return max(v.x, v.y); }

float fmax(const float3 v) { return max(v.x, max(v.y, v.z)); }

float fmax(const float4 v) { return max(max(v.x, v.y), max(v.y, v.z)); }

// Returns the minimum component of the specified vector.
float min3(const float3 v) { return min(v.x, min(v.y, v.z)); }

float fmin(const float2 v) { return min(v.x, v.y); }

float fmin(const float3 v) { return min(v.x, min(v.y, v.z)); }

float fmin(const float4 v) { return min(min(v.x, v.y), min(v.y, v.z)); }

//------------------------------------------------------------------------------
// Trigonometry
//------------------------------------------------------------------------------

/**
 * Approximates acos(x) with a max absolute error of 9.0x10^-3.
 * Valid in the range -1..1.
 */
float acosFast(float x)
{
    // Lagarde 2014, "Inverse trigonometric functions GPU optimization for AMD GCN architecture"
    // This is the approximation of degree 1, with a max absolute error of 9.0x10^-3
    float y = abs(x);
    float p = -0.1565827 * y + 1.570796;
    p = p * sqrt(1.0 - y);
    return select(x >= 0.0, p, F_PI - p);
}

/**
 * Approximates acos(x) with a max absolute error of 9.0x10^-3.
 * Valid only in the range 0..1.
 */
float acosFastPositive(float x)
{
    float p = -0.1565827 * x + 1.570796;
    return p * sqrt(1.0 - x);
}

//------------------------------------------------------------------------------
// Matrix and quaternion operations
//------------------------------------------------------------------------------

/**
 * Multiplies the specified 3-component vector by the 4x4 matrix (m * v) in
 * high precision.
 *
 * @public-api
 */
float4 mulMat4x4Float3(const float4x4 m, const float3 v)
{
	const float4x4 _m_trans = transpose(m);
    return v.x * _m_trans[0] + (v.y * _m_trans[1] + (v.z * _m_trans[2] + _m_trans[3]));
}

/**
 * Multiplies the specified 3-component vector by the 3x3 matrix (m * v) in
 * high precision.
 *
 * @public-api
 */
float3 mulMat3x3Float3(const float4x4 m, const float3 v)
{
	const float4x4 _m_trans = transpose(m);
    return v.x * _m_trans[0].xyz + (v.y * _m_trans[1].xyz + (v.z * _m_trans[2].xyz));
}

/**
 * Extracts the normal vector of the tangent frame encoded in the specified quaternion.
 */
void toTangentFrame(const float4 q, out float3 n)
{
    n = float3(0.0, 0.0, 1.0) + float3(2.0, -2.0, -2.0) * q.x * q.zwx + float3(2.0, 2.0, -2.0) * q.y * q.wzy;
}

/**
 * Extracts the normal and tangent vectors of the tangent frame encoded in the
 * specified quaternion.
 */
void toTangentFrame(const float4 q, out float3 n, out float3 t)
{
    toTangentFrame(q, n);
    t = float3(1.0, 0.0, 0.0) + float3(-2.0, 2.0, -2.0) * q.y * q.yxw + float3(-2.0, 2.0, 2.0) * q.z * q.zwx;
}

float3x3 cofactor(const float3x3 m)
{
    float a = m[0][0];
    float b = m[1][0];
    float c = m[2][0];
    float d = m[0][1];
    float e = m[1][1];
    float f = m[2][1];
    float g = m[0][2];
    float h = m[1][2];
    float i = m[2][2];

    float3x3 cof;
    cof[0][0] = e * i - f * h;
    cof[0][1] = c * h - b * i;
    cof[0][2] = b * f - c * e;
    cof[1][0] = f * g - d * i;
    cof[1][1] = a * i - c * g;
    cof[1][2] = c * d - a * f;
    cof[2][0] = d * h - e * g;
    cof[2][1] = b * g - a * h;
    cof[2][2] = a * e - b * d;
    return cof;
}

//------------------------------------------------------------------------------
// Random
//------------------------------------------------------------------------------

/*
 * Random number between 0 and 1, using interleaved gradient noise.
 * w must not be normalized (e.g. window coordinates)
 */
float interleavedGradientNoise(float2 w)
{
    const float3 m = float3(0.06711056, 0.00583715, 52.9829189);
    return frac(m.z * frac(dot(w, m.xy)));
}

//------------------------------------------------------------------------------
// Culling
//------------------------------------------------------------------------------

float3 Faceforward(float3 n, float3 v)
{
	return select((dot(n, v) < 0.0f), -n, n);
}

float Sqr(float x)
{
	return x * x;
}

float SphericalTheta(float3 v)
{
	return acos(clamp(v.z, -1.0f, 1.0f));
}

float SphericalPhi(float3 v)
{
	float p = atan2(v.y, v.x);
    return select((p < 0), (p + F_TAU), p);
}

void CoordinateSystem(float3 v1, out float3 v2, out float3 v3)
{
	float sign = (v1.z >= 0.0f) * 2.0f - 1.0f; // copysign(1.0f, v1.z); // No HLSL support yet
	float a	   = -1.0f / (sign + v1.z);
	float b	   = v1.x * v1.y * a;
	v2		   = float3(1.0f + sign * v1.x * v1.x * a, sign * b, -sign * v1.x);
	v3		   = float3(b, sign + v1.y * v1.y * a, -v1.y);
}

struct Frame
{
	float3 x;
	float3 y;
	float3 z;

	float3 ToWorld(float3 v) { return x * v.x + y * v.y + z * v.z; }

	float3 ToLocal(float3 v) { return float3(dot(v, x), dot(v, y), dot(v, z)); }
};

Frame InitFrameFromZ(float3 z)
{
	Frame frame;
	frame.z = z;
	CoordinateSystem(frame.z, frame.x, frame.y);
	return frame;
}

Frame InitFrameFromXY(float3 x, float3 y)
{
	Frame frame;
	frame.x = x;
	frame.y = y;
	frame.z = cross(x, y);
	return frame;
}

float2 octWrap(float2 v)
{
	return float2((1.0f - abs(v.y)) * select(v.x >= 0.0f, 1.0f, -1.0f), (1.0f - abs(v.x)) * select(v.y >= 0.0f, 1.0f, -1.0f));
}

struct OctahedralVector
{
	float x, y;

	float3 Decode()
	{
		float3 v   = float3(x, y, 1.0f - abs(x) - abs(y));
		float2 tmp = select((v.z < 0.0f), octWrap(float2(v.x, v.y)), float2(v.x, v.y));
		v.x		   = tmp.x;
		v.y		   = tmp.y;
		return normalize(v);
	}
};

OctahedralVector InitOctahedralVector(float3 v)
{
	OctahedralVector ov;

	float2 p = float2(v.x, v.y) * (1.0f / (abs(v.x) + abs(v.y) + abs(v.z)));
	p		 = select((v.z < 0.0f), octWrap(p), p);

	ov.x = p.x;
	ov.y = p.y;

	return ov;
}

// Ray tracing gems chapter 06: A fast and robust method for avoiding self-intersection
// Offsets the ray origin from current position p, along normal n (which must be geometric normal)
// so that no self-intersection can occur.
float3 OffsetRay(float3 p, float3 ng)
{
	static const float origin	   = 1.0f / 32.0f;
	static const float float_scale = 1.0f / 65536.0f;
	static const float int_scale   = 256.0f;

	int3 of_i = int3(int_scale * ng.x, int_scale * ng.y, int_scale * ng.z);

	float3 p_i = float3(
		asfloat(asint(p.x) + select((p.x < 0), -of_i.x, of_i.x)),
		asfloat(asint(p.y) + select((p.y < 0), -of_i.y, of_i.y)),
		asfloat(asint(p.z) + select((p.z < 0), -of_i.z, of_i.z)));

	return float3(
		select(abs(p.x) < origin, p.x + float_scale * ng.x, p_i.x),
		select(abs(p.y) < origin, p.y + float_scale * ng.y, p_i.y),
		select(abs(p.z) < origin, p.z + float_scale * ng.z, p_i.z));
}

#define PLANE_INTERSECTION_POSITIVE_HALFSPACE 0
#define PLANE_INTERSECTION_NEGATIVE_HALFSPACE 1
#define PLANE_INTERSECTION_INTERSECTING		  2

#define CONTAINMENT_DISJOINT				  0
#define CONTAINMENT_INTERSECTS				  1
#define CONTAINMENT_CONTAINS				  2

// ax + by + cz = d where d = dot(n, P)
struct Plane
{
	float3 Normal; // Plane normal. Points x on the plane satisfy dot(n, x) = d
	float  Offset; // d = dot(n, p) for a given point p on the plane
};

struct BoundingSphere
{
	float3 Center;
	float  Radius;

	bool Intersects(BoundingSphere Other)
	{
		// The distance between the sphere centers is computed and compared
		// against the sum of the sphere radii. To avoid an square root operation, the
		// squared distances are compared with squared sum radii instead.
		float3 d		 = Center - Other.Center;
		float  dist2	 = dot(d, d);
		float  radiusSum = Radius + Other.Radius;
		float  r2		 = radiusSum * radiusSum;
		return dist2 <= r2;
	}
};

struct BoundingBox
{
	float3 Center;
    float  _Padding_Center;
	float3 Extents;
    float  _Padding_Extents;

	bool Intersects(BoundingBox Other)
	{
		float3 minA = Center - Extents;
		float3 maxA = Center + Extents;

		float3 minB = Other.Center - Other.Extents;
		float3 maxB = Other.Center + Other.Extents;

		// All axis needs to overlap for a intersection
		return maxA.x >= minB.x && minA.x <= maxB.x && // Overlap on x-axis?
			   maxA.y >= minB.y && minA.y <= maxB.y && // Overlap on y-axis?
			   maxA.z >= minB.z && minA.z <= maxB.z;   // Overlap on z-axis?
	}

	// https://stackoverflow.com/questions/6053522/how-to-recalculate-axis-aligned-bounding-box-after-translate-rotate
	void Transform(float4x4 m, inout BoundingBox b)
	{
		//float3 t = m[3].xyz;
        float3 t = float3(m[0][3], m[1][3], m[2][3]);

		b.Center  = t;
		b.Extents = float3(0.0f, 0.0f, 0.0f);
		for (int i = 0; i < 3; ++i)
		{
			for (int j = 0; j < 3; ++j)
			{
				b.Center[i] += m[i][j] * Center[j];
				b.Extents[i] += abs(m[i][j]) * Extents[j];
			}
		}
	}
};

Plane ComputePlane(float3 a, float3 b, float3 c)
{
	Plane plane;
	plane.Normal = normalize(cross(b - a, c - a));
	plane.Offset = dot(plane.Normal, a);
	return plane;
}

#define FRUSTUM_PLANE_LEFT	 0
#define FRUSTUM_PLANE_RIGHT	 1
#define FRUSTUM_PLANE_BOTTOM 2
#define FRUSTUM_PLANE_TOP	 3
#define FRUSTUM_PLANE_NEAR	 4
#define FRUSTUM_PLANE_FAR	 5

struct Frustum
{
	Plane Left;	  // -x
	Plane Right;  // +x
	Plane Bottom; // -y
	Plane Top;	  // +y
	Plane Near;	  // -z
	Plane Far;	  // +z
};

int BoundingSphereToPlane(BoundingSphere s, Plane p)
{
	// Compute signed distance from plane to sphere center
	float sd = dot(s.Center, p.Normal) - p.Offset;
	if (sd > s.Radius)
	{
		return PLANE_INTERSECTION_POSITIVE_HALFSPACE;
	}
	if (sd < -s.Radius)
	{
		return PLANE_INTERSECTION_NEGATIVE_HALFSPACE;
	}
	return PLANE_INTERSECTION_INTERSECTING;
}

int BoundingBoxToPlane(BoundingBox b, Plane p)
{
	// Compute signed distance from plane to box center
	float sd = dot(b.Center, p.Normal) - p.Offset;

	// Compute the projection interval radius of b onto L(t) = b.Center + t * p.Normal
	// Projection radii r_i of the 8 bounding box vertices
	// r_i = dot((V_i - C), n)
	// r_i = dot((C +- e0*u0 +- e1*u1 +- e2*u2 - C), n)
	// Cancel C and distribute dot product
	// r_i = +-(dot(e0*u0, n)) +-(dot(e1*u1, n)) +-(dot(e2*u2, n))
	// We take the maximum position radius by taking the absolute value of the terms, we assume Extents to be positive
	// r = e0*|dot(u0, n)| + e1*|dot(u1, n)| + e2*|dot(u2, n)|
	// When the separating axis vector Normal is not a unit vector, we need to divide the radii by the length(Normal)
	// u0,u1,u2 are the local axes of the box, which is = [(1,0,0), (0,1,0), (0,0,1)] respectively for axis aligned bb
	float r = dot(b.Extents, abs(p.Normal));

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

int FrustumContainsBoundingSphere(Frustum f, BoundingSphere s)
{
	int	 p0			= BoundingSphereToPlane(s, f.Left);
	int	 p1			= BoundingSphereToPlane(s, f.Right);
	int	 p2			= BoundingSphereToPlane(s, f.Bottom);
	int	 p3			= BoundingSphereToPlane(s, f.Top);
	int	 p4			= BoundingSphereToPlane(s, f.Near);
	int	 p5			= BoundingSphereToPlane(s, f.Far);
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

int FrustumContainsBoundingBox(Frustum f, BoundingBox b)
{
	int	 p0			= BoundingBoxToPlane(b, f.Left);
	int	 p1			= BoundingBoxToPlane(b, f.Right);
	int	 p2			= BoundingBoxToPlane(b, f.Bottom);
	int	 p3			= BoundingBoxToPlane(b, f.Top);
	int	 p4			= BoundingBoxToPlane(b, f.Near);
	int	 p5			= BoundingBoxToPlane(b, f.Far);
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

Frustum ExtractPlanesDX(const float4x4 mvp)
{
    Frustum frustum;

	// Left clipping plane
    frustum.Left.Normal.x = mvp[3][0] + mvp[0][0];
    frustum.Left.Normal.y = mvp[3][1] + mvp[0][1];
    frustum.Left.Normal.z = mvp[3][2] + mvp[0][2];
    frustum.Left.Offset   = -(mvp[3][3] + mvp[0][3]);
    // Right clipping plane
    frustum.Right.Normal.x = mvp[3][0] - mvp[0][0];
    frustum.Right.Normal.y = mvp[3][1] - mvp[0][1];
    frustum.Right.Normal.z = mvp[3][2] - mvp[0][2];
    frustum.Right.Offset   = -(mvp[3][3] - mvp[0][3]);
    // Bottom clipping plane
    frustum.Bottom.Normal.x = mvp[3][0] + mvp[1][0];
    frustum.Bottom.Normal.y = mvp[3][1] + mvp[1][1];
    frustum.Bottom.Normal.z = mvp[3][2] + mvp[1][2];
    frustum.Bottom.Offset   = -(mvp[3][3] + mvp[1][3]);
    // Top clipping plane
    frustum.Top.Normal.x = mvp[3][0] - mvp[1][0];
    frustum.Top.Normal.y = mvp[3][1] - mvp[1][1];
    frustum.Top.Normal.z = mvp[3][2] - mvp[1][2];
    frustum.Top.Offset   = -(mvp[3][3] - mvp[1][3]);
    // Far clipping plane
    frustum.Far.Normal.x = mvp[2][0];
    frustum.Far.Normal.y = mvp[2][1];
    frustum.Far.Normal.z = mvp[2][2];
    frustum.Far.Offset   = -(mvp[2][3]);
    // Near clipping plane
    frustum.Near.Normal.x = mvp[3][0] - mvp[2][0];
    frustum.Near.Normal.y = mvp[3][1] - mvp[2][1];
    frustum.Near.Normal.z = mvp[3][2] - mvp[2][2];
    frustum.Near.Offset   = -(mvp[3][3] - mvp[2][3]);

	return frustum;
}

//------------------------------------------------------------------------------
// CubeMap Helper
//------------------------------------------------------------------------------

#define PX  0     // left            +----+
#define NX  1     // right           | PY |
#define PY  2     // bottom     +----+----+----+----+
#define NY  3     // top        | NX | PZ | PX | NZ |
#define PZ  4     // back       +----+----+----+----+
#define NZ  5     // front           | NY |
                  //                 +----+

#define Face uint

struct CubemapAddress
{
    Face face;
    float2 st;
};

float3 getDirectionForCubemap(Face face, float2 xy)
{
    // map [0, dim] to [-1,1]
    float cx = xy.x;
    float cy = -xy.y;

    float3 dir;
    const float l = sqrt(cx * cx + cy * cy + 1);
    switch (face)
    {
        case PX:  dir = float3(   1, cy, -cx); break;
        case NX:  dir = float3(  -1, cy,  cx); break;
        case PY:  dir = float3(  cx,  1, -cy); break;
        case NY:  dir = float3(  cx, -1,  cy); break;
        case PZ:  dir = float3(  cx, cy,   1); break;
        case NZ:  dir = float3( -cx, cy,  -1); break;
    }
    return dir * (1.0f / l);
}

CubemapAddress getAddressForCubemap(float3 r)
{
    CubemapAddress addr;
    float sc, tc, ma;
    const float rx = abs(r.x);
    const float ry = abs(r.y);
    const float rz = abs(r.z);
    if (rx >= ry && rx >= rz)
    {
        ma = 1.0f / rx;
        if (r.x >= 0)
        {
            addr.face = PX;
            sc = -r.z;
            tc = -r.y;
        }
        else
        {
            addr.face = NX;
            sc = r.z;
            tc = -r.y;
        }
    }
    else if (ry >= rx && ry >= rz)
    {
        ma = 1.0f / ry;
        if (r.y >= 0)
        {
            addr.face = PY;
            sc = r.x;
            tc = r.z;
        }
        else
        {
            addr.face = NY;
            sc = r.x;
            tc = -r.z;
        }
    }
    else
    {
        ma = 1.0f / rz;
        if (r.z >= 0)
        {
            addr.face = PZ;
            sc = r.x;
            tc = -r.y;
        }
        else
        {
            addr.face = NZ;
            sc = -r.x;
            tc = -r.y;
        }
    }
    // ma is guaranteed to be >= sc and tc
    addr.st = float2((sc * ma + 1.0f) * 0.5f, (tc * ma + 1.0f) * 0.5f);
    return addr;
}

/*
 * Area of a cube face's quadrant projected onto a sphere
 *
 *  1 +---+----------+
 *    |   |          |
 *    |---+----------|
 *    |   |(x,y)     |
 *    |   |          |
 *    |   |          |
 * -1 +---+----------+
 *   -1              1
 *
 *
 * The quadrant (-1,1)-(x,y) is projected onto the unit sphere
 *
 */
float sphereQuadrantArea(float x, float y)
{
    return atan2(x*y, sqrt(x*x + y*y + 1));
}

//! computes the solid angle of a pixel of a face of a cubemap
float solidAngle(uint dim, uint u, uint v)
{
    const float iDim = 1.0f / dim;
    float s = ((u + 0.5f) * 2 * iDim) - 1;
    float t = ((v + 0.5f) * 2 * iDim) - 1;
    const float x0 = s - iDim;
    const float y0 = t - iDim;
    const float x1 = s + iDim;
    const float y1 = t + iDim;
    float solidAngle = sphereQuadrantArea(x0, y0) -
                        sphereQuadrantArea(x0, y1) -
                        sphereQuadrantArea(x1, y0) +
                        sphereQuadrantArea(x1, y1);
    return solidAngle;
}

//------------------------------------------------------------------------------
// Material Transform
//------------------------------------------------------------------------------

#define MIN_PERCEPTUAL_ROUGHNESS 0.045
#define MIN_ROUGHNESS 0.002025

#define MIN_N_DOT_V 1e-4


float clampNoV(float NoV)
{
    // Neubelt and Pettineo 2013, "Crafting a Next-gen Material Pipeline for The Order: 1886"
    return max(NoV, MIN_N_DOT_V);
}

float3 computeDiffuseColor(const float4 baseColor, float metallic)
{
    return baseColor.rgb * (1.0 - metallic);
}

float3 computeF0(const float4 baseColor, float metallic, float reflectance)
{
    return baseColor.rgb * metallic + (reflectance * (1.0 - metallic));
}

float computeDielectricF0(float reflectance)
{
    return 0.16 * reflectance * reflectance;
}

float computeMetallicFromSpecularColor(const float3 specularColor)
{
    return max3(specularColor);
}

float computeRoughnessFromGlossiness(float glossiness)
{
    return 1.0 - glossiness;
}

float perceptualRoughnessToRoughness(float perceptualRoughness)
{
    return perceptualRoughness * perceptualRoughness;
}

float roughnessToPerceptualRoughness(float roughness)
{
    return sqrt(roughness);
}

float iorToF0(float transmittedIor, float incidentIor)
{
    return sq((transmittedIor - incidentIor) / (transmittedIor + incidentIor));
}

float f0ToIor(float f0)
{
    float r = sqrt(f0);
    return (1.0 + r) / (1.0 - r);
}

float3 f0ClearCoatToSurface(const float3 f0)
{
    // Approximation of iorTof0(f0ToIor(f0), 1.5)
    // This assumes that the clear coat layer has an IOR of 1.5
//#if FILAMENT_QUALITY == FILAMENT_QUALITY_LOW
    return saturate(f0 * (f0 * 0.526868 + 0.529324) - 0.0482256);
//#else
//    return saturate(f0 * (f0 * (0.941892 - 0.263008 * f0) + 0.346479) - 0.0285998);
//#endif
}

bool isPerspectiveProjection(const float4x4 clipFromViewMatrix)
{
    return transpose(clipFromViewMatrix)[2].w != 0.0;
}

float3x3 ToTBNMatrix(float3 N)
{
    float3 UpVector = abs(N.y) < 0.999 ? float3(0.0, 1.0, 0.0) : float3(0.0, 0.0, 1.0);
    float3 T = normalize(cross(N, UpVector));
    float3 B = cross(N, T);
	return transpose(float3x3(T,B,N));
}

float4 TangentToWorld(float3 N, float4 H)
{
	float3x3 tbn = ToTBNMatrix(N);
	float3 newN = mul(tbn, H.xyz);
	return float4(newN.xyz, H.w);
}

// [start, end] -> [0, 1] : (x - start) / (end - start) = x * rcpLength - (start * rcpLength)
float Remap01(float x, float rcpLength, float startTimesRcpLength)
{
	return saturate(x * rcpLength - startTimesRcpLength);
}

// [start, end] -> [1, 0] : (end - x) / (end - start) = (end * rcpLength) - x * rcpLength
float Remap10(float x, float rcpLength, float endTimesRcpLength)
{
	return saturate(endTimesRcpLength - x * rcpLength);
}

float3 PositivePow(float3 base, float3 power)
{
	return pow(max(abs(base), float3(FLT_EPS, FLT_EPS, FLT_EPS)), power);
}

#endif // __COMMON_MATH_HLSLI__