#pragma once

//------------------------------------------------------------------------------
// Common math
//------------------------------------------------------------------------------

static const float g_PI		 = 3.141592654f;
static const float g_2PI	 = 6.283185307f;
static const float g_1DIVPI	 = 0.318309886f;
static const float g_1DIV2PI = 0.159154943f;
static const float g_1DIV4PI = 0.079577471f;
static const float g_PIDIV2	 = 1.570796327f;
static const float g_PIDIV4	 = 0.785398163f;

static const float FLT_EPS = 1e-5;
static const float FLT_MAX = asfloat(0x7F7FFFFF);

#define PI g_PI
#define HALF_PI g_PIDIV2

//------------------------------------------------------------------------------
// Scalar operations
//------------------------------------------------------------------------------

// Computes x^5 using only multiply operations.
float pow5(float x)
{
    float x2 = x * x;
    return x2 * x2 * x;
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
    return x >= 0.0 ? p : PI - p;
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
    return v.x * m[0] + (v.y * m[1] + (v.z * m[2] + m[3]));
}

/**
 * Multiplies the specified 3-component vector by the 3x3 matrix (m * v) in
 * high precision.
 *
 * @public-api
 */
float3 mulMat3x3Float3(const float4x4 m, const float3 v)
{
    return v.x * m[0].xyz + (v.y * m[1].xyz + (v.z * m[2].xyz));
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
	return (dot(n, v) < 0.0f) ? -n : n;
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
	return (p < 0) ? (p + g_2PI) : p;
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
	return float2((1.0f - abs(v.y)) * (v.x >= 0.0f ? 1.0f : -1.0f), (1.0f - abs(v.x)) * (v.y >= 0.0f ? 1.0f : -1.0f));
}

struct OctahedralVector
{
	float x, y;

	float3 Decode()
	{
		float3 v   = float3(x, y, 1.0f - abs(x) - abs(y));
		float2 tmp = (v.z < 0.0f) ? octWrap(float2(v.x, v.y)) : float2(v.x, v.y);
		v.x		   = tmp.x;
		v.y		   = tmp.y;
		return normalize(v);
	}
};

OctahedralVector InitOctahedralVector(float3 v)
{
	OctahedralVector ov;

	float2 p = float2(v.x, v.y) * (1.0f / (abs(v.x) + abs(v.y) + abs(v.z)));
	p		 = (v.z < 0.0f) ? octWrap(p) : p;

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
		asfloat(asint(p.x) + ((p.x < 0) ? -of_i.x : of_i.x)),
		asfloat(asint(p.y) + ((p.y < 0) ? -of_i.y : of_i.y)),
		asfloat(asint(p.z) + ((p.z < 0) ? -of_i.z : of_i.z)));

	return float3(
		abs(p.x) < origin ? p.x + float_scale * ng.x : p_i.x,
		abs(p.y) < origin ? p.y + float_scale * ng.y : p_i.y,
		abs(p.z) < origin ? p.z + float_scale * ng.z : p_i.z);
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
