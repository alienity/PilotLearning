#include "moyu_math.h"

namespace MoYu
{
    //------------------------------------------------------------------------------------------
    Math::AngleUnit Math::k_AngleUnit;

    Math::Math() { k_AngleUnit = AngleUnit::AU_DEGREE; }

    bool Math::realEqual(float a, float b, float tolerance /* = std::numeric_limits<float>::epsilon() */)
    {
        return std::fabs(b - a) <= tolerance;
    }

    float Math::degreesToRadians(float degrees) { return degrees * Math_fDeg2Rad; }

    float Math::radiansToDegrees(float radians) { return radians * Math_fRad2Deg; }

    float Math::angleUnitsToRadians(float angleunits)
    {
        if (k_AngleUnit == AngleUnit::AU_DEGREE)
            return angleunits * Math_fDeg2Rad;

        return angleunits;
    }

    float Math::radiansToAngleUnits(float radians)
    {
        if (k_AngleUnit == AngleUnit::AU_DEGREE)
            return radians * Math_fRad2Deg;

        return radians;
    }

    float Math::angleUnitsToDegrees(float angleunits)
    {
        if (k_AngleUnit == AngleUnit::AU_RADIAN)
            return angleunits * Math_fRad2Deg;

        return angleunits;
    }

    float Math::degreesToAngleUnits(float degrees)
    {
        if (k_AngleUnit == AngleUnit::AU_RADIAN)
            return degrees * Math_fDeg2Rad;

        return degrees;
    }

    float Math::acos(float value)
    {
        if (-1.0 < value)
        {
            if (value < 1.0)
                return std::acos(value);

            return 0.0;
        }

        return Math_PI;
    }

    float Math::asin(float value)
    {
        if (-1.0 < value)
        {
            if (value < 1.0)
                return std::asin(value);

            return Math_HALF_PI;
        }

        return -Math_HALF_PI;
    }

    Matrix4x4 Math::makeViewMatrix(const Vector3& position, const Quaternion& orientation)
    {
        return Matrix4x4::createView(position, orientation);
    }

    Matrix4x4 Math::makeLookAtMatrix(const Vector3& eye_position, const Vector3& target_position, const Vector3& up_dir)
    {
        return Matrix4x4::lookAt(eye_position, target_position, up_dir);
    }

    Matrix4x4 Math::makePerspectiveMatrix(Radian fovy, float aspect, float znear, float zfar)
    {
        return Matrix4x4::createPerspectiveFieldOfView(fovy.valueRadians(), aspect, znear, zfar);
    }

    Matrix4x4 Math::makeOrthographicProjectionMatrix(float left, float right, float bottom, float top, float znear, float zfar)
    {
        return Matrix4x4::createOrthographicOffCenter(left, right, bottom, top, znear, zfar);
    }


    //------------------------------------------------------------------------------------------

    const Vector2 Vector2::Zero(0.f, 0.f);
    const Vector2 Vector2::One(1.f, 1.f);
    const Vector2 Vector2::UnitX(1.f, 0.f);
    const Vector2 Vector2::UnitY(0.f, 1.f);

    const Vector3 Vector3::Zero(0.f, 0.f, 0.f);
    const Vector3 Vector3::One(1.f, 1.f, 1.f);
    const Vector3 Vector3::UnitX(1.f, 0.f, 0.f);
    const Vector3 Vector3::UnitY(0.f, 1.f, 0.f);
    const Vector3 Vector3::UnitZ(0.f, 0.f, 1.f);
    const Vector3 Vector3::Up(0.f, 1.f, 0.f);
    const Vector3 Vector3::Down(0.f, -1.f, 0.f);
    const Vector3 Vector3::Right(1.f, 0.f, 0.f);
    const Vector3 Vector3::Left(-1.f, 0.f, 0.f);
    const Vector3 Vector3::Forward(0.f, 0.f, -1.f);
    const Vector3 Vector3::Backward(0.f, 0.f, 1.f);

    const Vector4 Vector4::Zero(0.f, 0.f, 0.f, 0.f);
    const Vector4 Vector4::One(1.f, 1.f, 1.f, 1.f);
    const Vector4 Vector4::UnitX(1.f, 0.f, 0.f, 0.f);
    const Vector4 Vector4::UnitY(0.f, 1.f, 0.f, 0.f);
    const Vector4 Vector4::UnitZ(0.f, 0.f, 1.f, 0.f);
    const Vector4 Vector4::UnitW(0.f, 0.f, 0.f, 1.f);

    const Matrix3x3 Matrix3x3::Zero(0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f);
    const Matrix3x3 Matrix3x3::Identity(1.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 1.f);

    const Matrix4x4 Matrix4x4::Zero(0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f);
    const Matrix4x4 Matrix4x4::Identity(1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f);

    const Quaternion Quaternion::Identity(1.f, 0.f, 0.f, 0.f);

    //------------------------------------------------------------------------------------------

    Matrix3x3::Matrix3x3(float arr[3][3])
    {
        memcpy(m[0], arr[0], 3 * sizeof(float));
        memcpy(m[1], arr[1], 3 * sizeof(float));
        memcpy(m[2], arr[2], 3 * sizeof(float));
    }

    Matrix3x3::Matrix3x3(float (&float_array)[9])
    {
        m[0][0] = float_array[0];
        m[0][1] = float_array[1];
        m[0][2] = float_array[2];
        m[1][0] = float_array[3];
        m[1][1] = float_array[4];
        m[1][2] = float_array[5];
        m[2][0] = float_array[6];
        m[2][1] = float_array[7];
        m[2][2] = float_array[8];
    }

    Matrix3x3::Matrix3x3(float m00,
                         float m01,
                         float m02,
                         float m10,
                         float m11,
                         float m12,
                         float m20,
                         float m21,
                         float m22)
    {
        m[0][0] = m00;
        m[0][1] = m01;
        m[0][2] = m02;
        m[1][0] = m10;
        m[1][1] = m11;
        m[1][2] = m12;
        m[2][0] = m20;
        m[2][1] = m21;
        m[2][2] = m22;
    }

    Matrix3x3::Matrix3x3(const Vector3& row0, const Vector3& row1, const Vector3& row2)
    {
        m[0][0] = row0.x;
        m[0][1] = row0.y;
        m[0][2] = row0.z;
        m[1][0] = row1.x;
        m[1][1] = row1.y;
        m[1][2] = row1.z;
        m[2][0] = row2.x;
        m[2][1] = row2.y;
        m[2][2] = row2.z;
    }

    // https://www.euclideanspace.com/maths/geometry/rotations/conversions/quaternionToMatrix/index.htm
    Matrix3x3::Matrix3x3(const Quaternion& q)
    {
        float yy = q.y * q.y;
        float zz = q.z * q.z;
        float xy = q.x * q.y;
        float zw = q.z * q.w;
        float xz = q.x * q.z;
        float yw = q.y * q.w;
        float xx = q.x * q.x;
        float yz = q.y * q.z;
        float xw = q.x * q.w;

        m[0][0] = 1 - 2 * yy - 2 * zz;
        m[0][1] = 2 * xy - 2 * zw;
        m[0][2] = 2 * xz + 2 * yw;

        m[1][0] = 2 * xy + 2 * zw;
        m[1][1] = 1 - 2 * xx - 2 * zz;
        m[1][2] = 2 * yz - 2 * xw;

        m[2][0] = 2 * xz - 2 * yw;
        m[2][1] = 2 * yz + 2 * xw;
        m[2][2] = 1 - 2 * xx - 2 * yy;
    }

    void Matrix3x3::fromData(float (&float_array)[9])
    {
        m[0][0] = float_array[0];
        m[0][1] = float_array[1];
        m[0][2] = float_array[2];
        m[1][0] = float_array[3];
        m[1][1] = float_array[4];
        m[1][2] = float_array[5];
        m[2][0] = float_array[6];
        m[2][1] = float_array[7];
        m[2][2] = float_array[8];
    }

    void Matrix3x3::toData(float (&float_array)[9]) const
    {
        float_array[0] = m[0][0];
        float_array[1] = m[0][1];
        float_array[2] = m[0][2];
        float_array[3] = m[1][0];
        float_array[4] = m[1][1];
        float_array[5] = m[1][2];
        float_array[6] = m[2][0];
        float_array[7] = m[2][1];
        float_array[8] = m[2][2];
    }

    float* Matrix3x3::operator[](size_t row_index) const { return (float*)m[row_index]; }

    Vector3 Matrix3x3::getColumn(size_t col_index) const
    {
        assert(0 <= col_index && col_index < 3);
        return Vector3(m[0][col_index], m[1][col_index], m[2][col_index]);
    }

    void Matrix3x3::setColumn(size_t col_index, const Vector3& vec)
    {
        m[0][col_index] = vec.x;
        m[1][col_index] = vec.y;
        m[2][col_index] = vec.z;
    }

    void Matrix3x3::fromAxes(const Vector3& x_axis, const Vector3& y_axis, const Vector3& z_axis)
    {
        setColumn(0, x_axis);
        setColumn(1, y_axis);
        setColumn(2, z_axis);
    }

    // assignment and comparison
    bool Matrix3x3::operator==(const Matrix3x3& rhs) const
    {
        for (size_t row_index = 0; row_index < 3; row_index++)
        {
            for (size_t col_index = 0; col_index < 3; col_index++)
            {
                if (m[row_index][col_index] != rhs.m[row_index][col_index])
                    return false;
            }
        }

        return true;
    }

    bool Matrix3x3::operator!=(const Matrix3x3& rhs) const { return !operator==(rhs); }

    // arithmetic operations
    Matrix3x3 Matrix3x3::operator+(const Matrix3x3& rhs) const
    {
        Matrix3x3 sum;
        for (size_t row_index = 0; row_index < 3; row_index++)
        {
            for (size_t col_index = 0; col_index < 3; col_index++)
            {
                sum.m[row_index][col_index] = m[row_index][col_index] + rhs.m[row_index][col_index];
            }
        }
        return sum;
    }

    Matrix3x3 Matrix3x3::operator-(const Matrix3x3& rhs) const
    {
        Matrix3x3 diff;
        for (size_t row_index = 0; row_index < 3; row_index++)
        {
            for (size_t col_index = 0; col_index < 3; col_index++)
            {
                diff.m[row_index][col_index] = m[row_index][col_index] - rhs.m[row_index][col_index];
            }
        }
        return diff;
    }

    Matrix3x3 Matrix3x3::operator*(const Matrix3x3& rhs) const
    {
        Matrix3x3 prod;
        for (size_t row_index = 0; row_index < 3; row_index++)
        {
            for (size_t col_index = 0; col_index < 3; col_index++)
            {
                prod.m[row_index][col_index] = m[row_index][0] * rhs.m[0][col_index] +
                                               m[row_index][1] * rhs.m[1][col_index] +
                                               m[row_index][2] * rhs.m[2][col_index];
            }
        }
        return prod;
    }

    // matrix * vector [3x3 * 3x1 = 3x1]
    Vector3 Matrix3x3::operator*(const Vector3& rhs) const
    {
        Vector3 prod;
        for (size_t row_index = 0; row_index < 3; row_index++)
        {
            prod[row_index] = m[row_index][0] * rhs.x + m[row_index][1] * rhs.y + m[row_index][2] * rhs.z;
        }
        return prod;
    }

    Matrix3x3 Matrix3x3::operator-() const
    {
        Matrix3x3 neg;
        for (size_t row_index = 0; row_index < 3; row_index++)
        {
            for (size_t col_index = 0; col_index < 3; col_index++)
                neg[row_index][col_index] = -m[row_index][col_index];
        }
        return neg;
    }

    // matrix * scalar
    Matrix3x3 Matrix3x3::operator*(float scalar) const
    {
        Matrix3x3 prod;
        for (size_t row_index = 0; row_index < 3; row_index++)
        {
            for (size_t col_index = 0; col_index < 3; col_index++)
                prod[row_index][col_index] = scalar * m[row_index][col_index];
        }
        return prod;
    }

    // scalar * matrix
    Matrix3x3 operator*(float scalar, const Matrix3x3& rhs)
    {
        Matrix3x3 prod;
        for (size_t row_index = 0; row_index < 3; row_index++)
        {
            for (size_t col_index = 0; col_index < 3; col_index++)
                prod[row_index][col_index] = scalar * rhs.m[row_index][col_index];
        }
        return prod;
    }

    // utilities
    Matrix3x3 Matrix3x3::transpose() const
    {
        Matrix3x3 transpose_v;
        for (size_t row_index = 0; row_index < 3; row_index++)
        {
            for (size_t col_index = 0; col_index < 3; col_index++)
                transpose_v[row_index][col_index] = m[col_index][row_index];
        }
        return transpose_v;
    }

    bool Matrix3x3::inverse(Matrix3x3& inv_mat, float fTolerance) const
    {
        // Invert a 3x3 using cofactors.  This is about 8 times faster than
        // the Numerical Recipes code which uses Gaussian elimination.

        float det = determinant();
        if (std::fabs(det) <= fTolerance)
            return false;

        float inv_det = 1.0f / det;

        inv_mat = adjugate();
        inv_mat = inv_mat * det;

        return true;
    }

    Matrix3x3 Matrix3x3::inverse(float tolerance) const
    {
        Matrix3x3 inv = Zero;
        inverse(inv, tolerance);
        return inv;
    }

    Matrix3x3 Matrix3x3::adjugate() const
    {
        return Matrix3x3(minor(1, 2, 1, 2),
                         -minor(0, 2, 1, 2),
                         minor(0, 1, 1, 2),
                         -minor(1, 2, 0, 2),
                         minor(0, 2, 0, 2),
                         -minor(0, 1, 0, 2),
                         minor(1, 2, 0, 1),
                         -minor(0, 2, 0, 1),
                         minor(0, 1, 0, 1));
    }

    float Matrix3x3::minor(size_t r0, size_t r1, size_t c0, size_t c1) const
    {
        return m[r0][c0] * m[r1][c1] - m[r1][c0] * m[r0][c1];
    }

    float Matrix3x3::determinant() const
    {
        float cofactor00 = m[1][1] * m[2][2] - m[1][2] * m[2][1];
        float cofactor10 = m[1][2] * m[2][0] - m[1][0] * m[2][2];
        float cofactor20 = m[1][0] * m[2][1] - m[1][1] * m[2][0];

        float det = m[0][0] * cofactor00 + m[0][1] * cofactor10 + m[0][2] * cofactor20;

        return det;
    }

    // https://en.wikipedia.org/wiki/Euler_angles
    // https://www.geometrictools.com/Documentation/EulerAngles.pdf
    // extrinsic angles, ZYX in order
    Vector3 Matrix3x3::toTaitBryanAngles() const
    {
        float thetaX = 0; // phi
        float thetaY = 0; // theta
        float thetaZ = 0; // psi

        if (m[2][0] < 1)
        {
            if (m[2][0] > -1)
            {
                thetaY = asinf(-m[2][0]);
                thetaZ = atan2f(m[1][0], m[0][0]);
                thetaX = atan2f(m[2][1], m[2][2]);
            }
            else  // m[2][0] = -1
            {
                // Not a unique solution : thetaX − thetaZ = atan2(−r12 , r11)
                thetaY = +Math_PI / 2;
                thetaZ = -atan2f(-m[1][2], m[1][1]);
                thetaX = 0;
            }
        }
        else // m[2][0] = +1
        {
            // Not a unique solution : thetaX + thetaZ = atan2(−r12 , r11)
            thetaY = -Math_PI / 2;
            thetaZ = atan2f(-m[1][2], m[1][1]);
            thetaX = 0;
        }

        return Vector3(thetaX, thetaY, thetaZ);
    }

    // https://en.wikipedia.org/wiki/Euler_angles
    // https://www.geometrictools.com/Documentation/EulerAngles.pdf
    void Matrix3x3::fromTaitBryanAngles(const Vector3& taitBryanAngles)
    {
        float phi   = taitBryanAngles.x;
        float theta = taitBryanAngles.y;
        float psi   = taitBryanAngles.z;

        float cx = cosf(phi);
        float cy = cosf(theta);
        float cz = cosf(psi);
        float sx = sinf(phi);
        float sy = sinf(theta);
        float sz = sinf(psi);

        m[0][0] = cy * cz;
        m[0][1] = cz * sx * sy - cx * sz;
        m[0][2] = cx * cz * sy + sx * sz;
        m[1][0] = cy * sz;
        m[1][1] = cx * cz + sx * sy * sz;
        m[1][2] = -cz * sx + cx * sy * sz;
        m[2][0] = -sy;
        m[2][1] = cy * sx;
        m[2][2] = cx * cy;
    }

    void Matrix3x3::toAngleAxis(Vector3& axis, float& radian) const
    {
        double epsilon  = 0.01; // margin to allow for rounding errors
        double epsilon2 = 0.1;  // margin to distinguish between 0 and 180 degrees
        // optional check that input is pure rotation, 'isRotationMatrix' is defined at:
        // https://www.euclideanspace.com/maths/algebra/matrix/orthogonal/rotation/
        if ((std::abs(m[0][1] - m[1][0]) < epsilon) && (std::abs(m[0][2] - m[2][0]) < epsilon) &&
            (std::abs(m[1][2] - m[2][1]) < epsilon))
        {
            // singularity found
            // first check for identity matrix which must have +1 for all terms
            //  in leading diagonaland zero in other terms
            if ((std::abs(m[0][1] + m[1][0]) < epsilon2) && (std::abs(m[0][2] + m[2][0]) < epsilon2) &&
                (std::abs(m[1][2] + m[2][1]) < epsilon2) && (std::abs(m[0][0] + m[1][1] + m[2][2] - 3) < epsilon2))
            {
                // this singularity is identity matrix so angle = 0
                axis   = Vector3(1, 0, 0);
                radian = 0;
                return;
            }
            // otherwise this singularity is angle = 180
            radian    = Math_PI;
            double xx = (m[0][0] + 1) / 2;
            double yy = (m[1][1] + 1) / 2;
            double zz = (m[2][2] + 1) / 2;
            double xy = (m[0][1] + m[1][0]) / 4;
            double xz = (m[0][2] + m[2][0]) / 4;
            double yz = (m[1][2] + m[2][1]) / 4;
            if ((xx > yy) && (xx > zz)) // m[0][0] is the largest diagonal term
            {
                if (xx < epsilon)
                {
                    axis.x = 0;
                    axis.y = 0.7071;
                    axis.z = 0.7071;
                }
                else
                {
                    axis.x = std::sqrt(xx);
                    axis.y = xy / axis.x;
                    axis.z = xz / axis.x;
                }
            }
            else if (yy > zz) // m[1][1] is the largest diagonal term
            {
                if (yy < epsilon)
                {
                    axis.x = 0.7071;
                    axis.y = 0;
                    axis.z = 0.7071;
                }
                else
                {
                    axis.y = std::sqrt(yy);
                    axis.x = xy / axis.y;
                    axis.z = yz / axis.y;
                }
            }
            else // m[2][2] is the largest diagonal term so base result on this
            {
                if (zz < epsilon)
                {
                    axis.x = 0.7071;
                    axis.y = 0.7071;
                    axis.z = 0;
                }
                else
                {
                    axis.z = std::sqrt(zz);
                    axis.x = xz / axis.z;
                    axis.y = yz / axis.z;
                }
            }
            return; // return 180 deg rotation
        }
        // as we have reached here there are no singularities so we can handle normally
        double s = std::sqrt((m[2][1] - m[1][2]) * (m[2][1] - m[1][2]) + (m[0][2] - m[2][0]) * (m[0][2] - m[2][0]) +
                             (m[1][0] - m[0][1]) * (m[1][0] - m[0][1])); // used to normalise
        if (std::abs(s) < 0.001)
            s = 1;
        // prevent divide by zero, should not happen if matrix is orthogonal and should be
        // caught by singularity test above, but I've left it in just in case
        radian = std::acos((m[0][0] + m[1][1] + m[2][2] - 1) / 2);
        axis.x = (m[2][1] - m[1][2]) / s;
        axis.y = (m[0][2] - m[2][0]) / s;
        axis.z = (m[1][0] - m[0][1]) / s;
    }

    // https://www.geometrictools.com/Documentation/EulerAngles.pdf
    void Matrix3x3::fromAngleAxis(const Vector3& axis, const float& radian)
    {
        Vector3 axisNorm = Vector3::normalize(axis);

        float cos_v         = std::cos(radian);
        float sin_v         = std::sin(radian);
        float one_minus_cos = 1.0f - cos_v;
        float x2            = axisNorm.x * axisNorm.x;
        float y2            = axisNorm.y * axisNorm.y;
        float z2            = axisNorm.z * axisNorm.z;
        float xym           = axisNorm.x * axisNorm.y * one_minus_cos;
        float xzm           = axisNorm.x * axisNorm.z * one_minus_cos;
        float yzm           = axisNorm.y * axisNorm.z * one_minus_cos;
        float x_sin_v       = axisNorm.x * sin_v;
        float y_sin_v       = axisNorm.y * sin_v;
        float z_sinv        = axisNorm.z * sin_v;

        m[0][0] = x2 * one_minus_cos + cos_v;
        m[0][1] = xym - z_sinv;
        m[0][2] = xzm + y_sin_v;
        m[1][0] = xym + z_sinv;
        m[1][1] = y2 * one_minus_cos + cos_v;
        m[1][2] = yzm - x_sin_v;
        m[2][0] = xzm - y_sin_v;
        m[2][1] = yzm + x_sin_v;
        m[2][2] = z2 * one_minus_cos + cos_v;
    }

    Matrix3x3 Matrix3x3::fromRotationX(const float& radians)
    {
        float c = std::cos(radians);
        float s = std::sin(radians);
        return Matrix3x3(1, 0, 0, 0, c, -s, 0, s, c);
    }

    Matrix3x3 Matrix3x3::fromRotationY(const float& radians)
    {
        float c = std::cos(radians);
        float s = std::sin(radians);
        return Matrix3x3(c, 0, s, 0, 1, 0, -s, 0, c);
    }

    Matrix3x3 Matrix3x3::fromRotationZ(const float& radians)
    {
        float c = std::cos(radians);
        float s = std::sin(radians);
        return Matrix3x3(c, -s, 0, s, c, 0, 0, 0, 1);
    }

    Matrix3x3 Matrix3x3::fromQuaternion(const Quaternion& quat)
    {
        float fTx  = quat.x + quat.x; // 2x
        float fTy  = quat.y + quat.y; // 2y
        float fTz  = quat.z + quat.z; // 2z
        float fTwx = fTx * quat.w;    // 2xw
        float fTwy = fTy * quat.w;    // 2yw
        float fTwz = fTz * quat.w;    // 2z2
        float fTxx = fTx * quat.x;    // 2x^2
        float fTxy = fTy * quat.x;    // 2xy
        float fTxz = fTz * quat.x;    // 2xz
        float fTyy = fTy * quat.y;    // 2y^2
        float fTyz = fTz * quat.y;    // 2yz
        float fTzz = fTz * quat.z;    // 2z^2

        Matrix3x3 kRot = Matrix3x3::Identity;
        kRot[0][0]     = 1.0f - (fTyy + fTzz); // 1 - 2y^2 - 2z^2
        kRot[0][1]     = fTxy - fTwz;          // 2xy - 2wz
        kRot[0][2]     = fTxz + fTwy;          // 2xz + 2wy
        kRot[1][0]     = fTxy + fTwz;          // 2xy + 2wz
        kRot[1][1]     = 1.0f - (fTxx + fTzz); // 1 - 2x^2 - 2z^2
        kRot[1][2]     = fTyz - fTwx;          // 2yz - 2wx
        kRot[2][0]     = fTxz - fTwy;          // 2xz - 2wy
        kRot[2][1]     = fTyz + fTwx;          // 2yz + 2wx
        kRot[2][2]     = 1.0f - (fTxx + fTyy); // 1 - 2x^2 - 2y^2

        return kRot;
    }

    Matrix3x3 Matrix3x3::scale(const Vector3& scale)
    {
        Matrix3x3 mat = Identity;
        mat.m[0][0]   = scale.x;
        mat.m[1][1]   = scale.y;
        mat.m[2][2]   = scale.z;
        return mat;
    }

    //------------------------------------------------------------------------------------------

    Matrix4x4::Matrix4x4(float arr[4][4])
    {
        m[0][0] = arr[0][0];
        m[0][1] = arr[0][1];
        m[0][2] = arr[0][2];
        m[0][3] = arr[0][3];
        m[1][0] = arr[1][0];
        m[1][1] = arr[1][1];
        m[1][2] = arr[1][2];
        m[1][3] = arr[1][3];
        m[2][0] = arr[2][0];
        m[2][1] = arr[2][1];
        m[2][2] = arr[2][2];
        m[2][3] = arr[2][3];
        m[3][0] = arr[3][0];
        m[3][1] = arr[3][1];
        m[3][2] = arr[3][2];
        m[3][3] = arr[3][3];
    }

    Matrix4x4::Matrix4x4(const float (&float_array)[16])
    {
        m[0][0] = float_array[0];
        m[0][1] = float_array[1];
        m[0][2] = float_array[2];
        m[0][3] = float_array[3];
        m[1][0] = float_array[4];
        m[1][1] = float_array[5];
        m[1][2] = float_array[6];
        m[1][3] = float_array[7];
        m[2][0] = float_array[8];
        m[2][1] = float_array[9];
        m[2][2] = float_array[10];
        m[2][3] = float_array[11];
        m[3][0] = float_array[12];
        m[3][1] = float_array[13];
        m[3][2] = float_array[14];
        m[3][3] = float_array[15];
    }

    Matrix4x4::Matrix4x4(float m00,
                         float m01,
                         float m02,
                         float m03,
                         float m10,
                         float m11,
                         float m12,
                         float m13,
                         float m20,
                         float m21,
                         float m22,
                         float m23,
                         float m30,
                         float m31,
                         float m32,
                         float m33)
    {
        m[0][0] = m00;
        m[0][1] = m01;
        m[0][2] = m02;
        m[0][3] = m03;
        m[1][0] = m10;
        m[1][1] = m11;
        m[1][2] = m12;
        m[1][3] = m13;
        m[2][0] = m20;
        m[2][1] = m21;
        m[2][2] = m22;
        m[2][3] = m23;
        m[3][0] = m30;
        m[3][1] = m31;
        m[3][2] = m32;
        m[3][3] = m33;
    }

    Matrix4x4::Matrix4x4(const Vector4& r0, const Vector4& r1, const Vector4& r2, const Vector4& r3)
    {
        m[0][0] = r0[0];
        m[0][1] = r0[1];
        m[0][2] = r0[2];
        m[0][3] = r0[3];
        m[1][0] = r1[0];
        m[1][1] = r1[1];
        m[1][2] = r1[2];
        m[1][3] = r1[3];
        m[2][0] = r2[0];
        m[2][1] = r2[1];
        m[2][2] = r2[2];
        m[2][3] = r2[3];
        m[3][0] = r3[0];
        m[3][1] = r3[1];
        m[3][2] = r3[2];
        m[3][3] = r3[3];
    }

    void Matrix4x4::fromData(const float (&float_array)[16])
    {
        m[0][0] = float_array[0];
        m[0][1] = float_array[1];
        m[0][2] = float_array[2];
        m[0][3] = float_array[3];
        m[1][0] = float_array[4];
        m[1][1] = float_array[5];
        m[1][2] = float_array[6];
        m[1][3] = float_array[7];
        m[2][0] = float_array[8];
        m[2][1] = float_array[9];
        m[2][2] = float_array[10];
        m[2][3] = float_array[11];
        m[3][0] = float_array[12];
        m[3][1] = float_array[13];
        m[3][2] = float_array[14];
        m[3][3] = float_array[15];
    }

    void Matrix4x4::toData(float (&float_array)[16]) const
    {
        float_array[0]  = m[0][0];
        float_array[1]  = m[0][1];
        float_array[2]  = m[0][2];
        float_array[3]  = m[0][3];
        float_array[4]  = m[1][0];
        float_array[5]  = m[1][1];
        float_array[6]  = m[1][2];
        float_array[7]  = m[1][3];
        float_array[8]  = m[2][0];
        float_array[9]  = m[2][1];
        float_array[10] = m[2][2];
        float_array[11] = m[2][3];
        float_array[12] = m[3][0];
        float_array[13] = m[3][1];
        float_array[14] = m[3][2];
        float_array[15] = m[3][3];
    }

    void Matrix4x4::setMatrix3x3(const Matrix3x3& mat3)
    {
        m[0][0] = mat3.m[0][0];
        m[0][1] = mat3.m[0][1];
        m[0][2] = mat3.m[0][2];
        m[0][3] = 0;
        m[1][0] = mat3.m[1][0];
        m[1][1] = mat3.m[1][1];
        m[1][2] = mat3.m[1][2];
        m[1][3] = 0;
        m[2][0] = mat3.m[2][0];
        m[2][1] = mat3.m[2][1];
        m[2][2] = mat3.m[2][2];
        m[2][3] = 0;
        m[3][0] = 0;
        m[3][1] = 0;
        m[3][2] = 0;
        m[3][3] = 1;
    }

    Matrix4x4::Matrix4x4(const Matrix3x3& mat)
    {
        m[0][0] = mat[0][0];
        m[0][1] = mat[0][1];
        m[0][2] = mat[0][2];
        m[0][3] = 0.f;
        m[1][0] = mat[1][0];
        m[1][1] = mat[1][1];
        m[1][2] = mat[1][2];
        m[1][3] = 0.f;
        m[2][0] = mat[2][0];
        m[2][1] = mat[2][1];
        m[2][2] = mat[2][2];
        m[2][3] = 0.f;
        m[3][0] = 0.f;
        m[3][1] = 0.f;
        m[3][2] = 0.f;
        m[3][3] = 1.f;
    }

    Matrix4x4::Matrix4x4(const Quaternion& rot) { *this = Matrix3x3::fromQuaternion(rot); }

    bool Matrix4x4::operator==(const Matrix4x4& rhs) const
    {
        return !(m[0][0] != rhs.m[0][0] || m[0][1] != rhs.m[0][1] || m[0][2] != rhs.m[0][2] || m[0][3] != rhs.m[0][3] ||
                 m[1][0] != rhs.m[1][0] || m[1][1] != rhs.m[1][1] || m[1][2] != rhs.m[1][2] || m[1][3] != rhs.m[1][3] ||
                 m[2][0] != rhs.m[2][0] || m[2][1] != rhs.m[2][1] || m[2][2] != rhs.m[2][2] || m[2][3] != rhs.m[2][3] ||
                 m[3][0] != rhs.m[3][0] || m[3][1] != rhs.m[3][1] || m[3][2] != rhs.m[3][2] || m[3][3] != rhs.m[3][3]);
    }

    bool Matrix4x4::operator!=(const Matrix4x4& rhs) const
    {
        return m[0][0] != rhs.m[0][0] || m[0][1] != rhs.m[0][1] || m[0][2] != rhs.m[0][2] || m[0][3] != rhs.m[0][3] ||
               m[1][0] != rhs.m[1][0] || m[1][1] != rhs.m[1][1] || m[1][2] != rhs.m[1][2] || m[1][3] != rhs.m[1][3] ||
               m[2][0] != rhs.m[2][0] || m[2][1] != rhs.m[2][1] || m[2][2] != rhs.m[2][2] || m[2][3] != rhs.m[2][3] ||
               m[3][0] != rhs.m[3][0] || m[3][1] != rhs.m[3][1] || m[3][2] != rhs.m[3][2] || m[3][3] != rhs.m[3][3];
    }

    Matrix4x4& Matrix4x4::operator=(const Matrix4x4& rhs)
    {
        for (size_t i = 0; i < 4; i++)
        {
            for (size_t j = 0; j < 4; j++)
            {
                m[i][j] = rhs.m[i][j];
            }
        }
        return *this;
    }

    Matrix4x4& Matrix4x4::operator+=(const Matrix4x4& rhs)
    {
        *this = *this + rhs;
        return *this;
    }

    Matrix4x4& Matrix4x4::operator-=(const Matrix4x4& rhs)
    {
        *this = *this - rhs;
        return *this;
    }

    Matrix4x4& Matrix4x4::operator*=(const Matrix4x4& rhs)
    {
        *this = *this * rhs;
        return *this;
    }

    Matrix4x4& Matrix4x4::operator*=(float s)
    {
        *this = *this * s;
        return *this;
    }

    Matrix4x4& Matrix4x4::operator/=(float s)
    {
        *this = *this / s;
        return *this;
    }

    Matrix4x4& Matrix4x4::operator/=(const Matrix4x4& rhs)
    {
        *this = *this / rhs;
        return *this;
    }

    Matrix4x4 Matrix4x4::operator-() const
    {
        Matrix4x4 mResult;
        for (size_t i = 0; i < 4; i++)
        {
            for (size_t j = 0; j < 4; j++)
            {
                mResult.m[i][j] = -m[i][j];
            }
        }
        return mResult;
    }

    inline Matrix4x4 operator+(const Matrix4x4& lhs, const Matrix4x4& rhs)
    {
        Matrix4x4 mResult;
        for (size_t i = 0; i < 4; i++)
        {
            for (size_t j = 0; j < 4; j++)
            {
                mResult.m[i][j] = lhs.m[i][j] + rhs.m[i][j];
            }
        }
        return mResult;
    }

    inline Matrix4x4 operator-(const Matrix4x4& lhs, const Matrix4x4& rhs)
    {
        Matrix4x4 mResult;
        for (size_t i = 0; i < 4; i++)
        {
            for (size_t j = 0; j < 4; j++)
            {
                mResult.m[i][j] = lhs.m[i][j] - rhs.m[i][j];
            }
        }
        return mResult;
    }

    inline Matrix4x4 operator*(const Matrix4x4& lhs, const Matrix4x4& rhs)
    {
        Matrix4x4 mResult;
        // Cache the invariants in registers
        float x = lhs.m[0][0];
        float y = lhs.m[0][1];
        float z = lhs.m[0][2];
        float w = lhs.m[0][3];
        // Perform the operation on the first row
        mResult.m[0][0] = (rhs.m[0][0] * x) + (rhs.m[1][0] * y) + (rhs.m[2][0] * z) + (rhs.m[3][0] * w);
        mResult.m[0][1] = (rhs.m[0][1] * x) + (rhs.m[1][1] * y) + (rhs.m[2][1] * z) + (rhs.m[3][1] * w);
        mResult.m[0][2] = (rhs.m[0][2] * x) + (rhs.m[1][2] * y) + (rhs.m[2][2] * z) + (rhs.m[3][2] * w);
        mResult.m[0][3] = (rhs.m[0][3] * x) + (rhs.m[1][3] * y) + (rhs.m[2][3] * z) + (rhs.m[3][3] * w);
        // Repeat for all the other rows
        x               = lhs.m[1][0];
        y               = lhs.m[1][1];
        z               = lhs.m[1][2];
        w               = lhs.m[1][3];
        mResult.m[1][0] = (rhs.m[0][0] * x) + (rhs.m[1][0] * y) + (rhs.m[2][0] * z) + (rhs.m[3][0] * w);
        mResult.m[1][1] = (rhs.m[0][1] * x) + (rhs.m[1][1] * y) + (rhs.m[2][1] * z) + (rhs.m[3][1] * w);
        mResult.m[1][2] = (rhs.m[0][2] * x) + (rhs.m[1][2] * y) + (rhs.m[2][2] * z) + (rhs.m[3][2] * w);
        mResult.m[1][3] = (rhs.m[0][3] * x) + (rhs.m[1][3] * y) + (rhs.m[2][3] * z) + (rhs.m[3][3] * w);
        x               = lhs.m[2][0];
        y               = lhs.m[2][1];
        z               = lhs.m[2][2];
        w               = lhs.m[2][3];
        mResult.m[2][0] = (rhs.m[0][0] * x) + (rhs.m[1][0] * y) + (rhs.m[2][0] * z) + (rhs.m[3][0] * w);
        mResult.m[2][1] = (rhs.m[0][1] * x) + (rhs.m[1][1] * y) + (rhs.m[2][1] * z) + (rhs.m[3][1] * w);
        mResult.m[2][2] = (rhs.m[0][2] * x) + (rhs.m[1][2] * y) + (rhs.m[2][2] * z) + (rhs.m[3][2] * w);
        mResult.m[2][3] = (rhs.m[0][3] * x) + (rhs.m[1][3] * y) + (rhs.m[2][3] * z) + (rhs.m[3][3] * w);
        x               = lhs.m[3][0];
        y               = lhs.m[3][1];
        z               = lhs.m[3][2];
        w               = lhs.m[3][3];
        mResult.m[3][0] = (rhs.m[0][0] * x) + (rhs.m[1][0] * y) + (rhs.m[2][0] * z) + (rhs.m[3][0] * w);
        mResult.m[3][1] = (rhs.m[0][1] * x) + (rhs.m[1][1] * y) + (rhs.m[2][1] * z) + (rhs.m[3][1] * w);
        mResult.m[3][2] = (rhs.m[0][2] * x) + (rhs.m[1][2] * y) + (rhs.m[2][2] * z) + (rhs.m[3][2] * w);
        mResult.m[3][3] = (rhs.m[0][3] * x) + (rhs.m[1][3] * y) + (rhs.m[2][3] * z) + (rhs.m[3][3] * w);
        return mResult;
    }

    Matrix4x4 operator*(const Matrix4x4& lhs, float s)
    {
        Matrix4x4 mResult;
        for (size_t i = 0; i < 4; i++)
        {
            for (size_t j = 0; j < 4; j++)
            {
                mResult.m[i][j] = lhs.m[i][j] * s;
            }
        }
        return mResult;
    }

    Matrix4x4 operator/(const Matrix4x4& lhs, float s)
    {
        Matrix4x4 mResult;
        for (size_t i = 0; i < 4; i++)
        {
            for (size_t j = 0; j < 4; j++)
            {
                mResult.m[i][j] = lhs.m[i][j] / s;
            }
        }
        return mResult;
    }

    Matrix4x4 operator/(const Matrix4x4& lhs, const Matrix4x4& rhs)
    {
        Matrix4x4 mResult;
        for (size_t i = 0; i < 4; i++)
        {
            for (size_t j = 0; j < 4; j++)
            {
                mResult.m[i][j] = lhs.m[i][j] / rhs.m[i][j];
            }
        }
        return mResult;
    }

    Matrix4x4 operator*(float s, const Matrix4x4& rhs)
    {
        Matrix4x4 mResult;
        for (size_t i = 0; i < 4; i++)
        {
            for (size_t j = 0; j < 4; j++)
            {
                mResult.m[i][j] = rhs.m[i][j] * s;
            }
        }
        return mResult;
    }

    Vector3 operator*(const Matrix4x4& lhs, Vector3 v)
    {
        Vector3 r;
        float   inv_w = 1.0f / (lhs.m[3][0] * v.x + lhs.m[3][1] * v.y + lhs.m[3][2] * v.z + lhs.m[3][3]);
        r.x           = (lhs.m[0][0] * v.x + lhs.m[0][1] * v.y + lhs.m[0][2] * v.z + lhs.m[0][3]) * inv_w;
        r.y           = (lhs.m[1][0] * v.x + lhs.m[1][1] * v.y + lhs.m[1][2] * v.z + lhs.m[1][3]) * inv_w;
        r.z           = (lhs.m[2][0] * v.x + lhs.m[2][1] * v.y + lhs.m[2][2] * v.z + lhs.m[2][3]) * inv_w;
        return r;
    }

    Vector4 operator*(const Matrix4x4& lhs, Vector4 v)
    {
        float ox = lhs.m[0][0] * v.x + lhs.m[0][1] * v.y + lhs.m[0][2] * v.z + lhs.m[0][3] * v.w;
        float oy = lhs.m[1][0] * v.x + lhs.m[1][1] * v.y + lhs.m[1][2] * v.z + lhs.m[1][3] * v.w;
        float oz = lhs.m[2][0] * v.x + lhs.m[2][1] * v.y + lhs.m[2][2] * v.z + lhs.m[2][3] * v.w;
        float ow = lhs.m[3][0] * v.x + lhs.m[3][1] * v.y + lhs.m[3][2] * v.z + lhs.m[3][3] * v.w;
        return Vector4(ox, oy, oz, ow);
    }

    void Matrix4x4::Up(const Vector3& v)
    {
        m[0][1] = v.x;
        m[1][1] = v.y;
        m[2][1] = v.z;
    }

    void Matrix4x4::Down(const Vector3& v)
    {
        m[0][1] = -v.x;
        m[1][1] = -v.y;
        m[2][1] = -v.z;
    }

    void Matrix4x4::Right(const Vector3& v)
    {
        m[0][0] = v.x;
        m[1][0] = v.y;
        m[2][0] = v.z;
    }

    void Matrix4x4::Left(const Vector3& v)
    {
        m[0][0] = -v.x;
        m[1][0] = -v.y;
        m[2][0] = -v.z;
    }

    void Matrix4x4::Forward(const Vector3& v)
    {
        m[0][2] = -v.x;
        m[1][2] = -v.y;
        m[2][2] = -v.z;
    }

    void Matrix4x4::Backward(const Vector3& v)
    {
        m[0][2] = v.x;
        m[1][2] = v.y;
        m[2][2] = v.z;
    }

    void Matrix4x4::Translation(const Vector3& v)
    {
        m[0][3] = v.x;
        m[1][3] = v.y;
        m[2][3] = v.z;
    }

    // https://opensource.apple.com/source/WebCore/WebCore-514/platform/graphics/transforms/TransformationMatrix.cpp
    bool Matrix4x4::decompose(Vector3& scale, Quaternion& rotation, Vector3& translation)
    {
        translation = Vector3(m[0][3], m[1][3], m[2][3]);

        Vector3 col0 = Vector3(m[0][0], m[1][0], m[2][0]);
        Vector3 col1 = Vector3(m[0][1], m[1][1], m[2][1]);
        Vector3 col2 = Vector3(m[0][2], m[1][2], m[2][2]);
        scale        = Vector3(col0.length(), col1.length(), col2.length());

        Matrix3x3 rotMat = Matrix3x3(Vector3(m[0][0] / scale.x, m[0][1] / scale.y, m[0][1] / scale.z),
                                     Vector3(m[1][0] / scale.x, m[1][1] / scale.y, m[1][1] / scale.z),
                                     Vector3(m[2][0] / scale.x, m[2][1] / scale.y, m[2][1] / scale.z));

        rotation = Quaternion::fromRotationMatrix(rotMat);

        return true;
    }

    Matrix4x4 Matrix4x4::transpose() const
    {
        Matrix4x4 mResult;
        for (size_t i = 0; i < 4; i++)
        {
            for (size_t j = 0; j < 4; j++)
            {
                mResult.m[i][j] = m[j][i];
            }
        }
        return mResult;
    }

    bool Matrix4x4::inverse(Matrix4x4& inv_mat, float fTolerance) const
    {
        // Invert a 3x3 using cofactors.  This is about 8 times faster than
        // the Numerical Recipes code which uses Gaussian elimination.

        float det = determinant();
        if (std::fabs(det) <= fTolerance)
            return false;

        float inv_det = 1.0f / det;

        inv_mat = adjugate();
        inv_mat = inv_mat * det;

        return true;
    }

    Matrix4x4 Matrix4x4::inverse(float tolerance) const
    {
        Matrix4x4 inv = Identity;
        inverse(inv, tolerance);
        return inv;
    }

    Matrix4x4 Matrix4x4::adjugate() const
    {
        return Matrix4x4(minor(1, 2, 3, 1, 2, 3),
                         -minor(0, 2, 3, 1, 2, 3),
                         minor(0, 1, 3, 1, 2, 3),
                         -minor(0, 1, 2, 1, 2, 3),
                         -minor(1, 2, 3, 0, 2, 3),
                         minor(0, 2, 3, 0, 2, 3),
                         -minor(0, 1, 3, 0, 2, 3),
                         minor(0, 1, 2, 0, 2, 3),
                         minor(1, 2, 3, 0, 1, 3),
                         -minor(0, 2, 3, 0, 1, 3),
                         minor(0, 1, 3, 0, 1, 3),
                         -minor(0, 1, 2, 0, 1, 3),
                         -minor(1, 2, 3, 0, 1, 2),
                         minor(0, 2, 3, 0, 1, 2),
                         -minor(0, 1, 3, 0, 1, 2),
                         minor(0, 1, 2, 0, 1, 2));
    }

    float Matrix4x4::minor(size_t r0, size_t r1, size_t r2, size_t c0, size_t c1, size_t c2) const
    {
        return m[r0][c0] * (m[r1][c1] * m[r2][c2] - m[r2][c1] * m[r1][c2]) -
               m[r0][c1] * (m[r1][c0] * m[r2][c2] - m[r2][c0] * m[r1][c2]) +
               m[r0][c2] * (m[r1][c0] * m[r2][c1] - m[r2][c0] * m[r1][c1]);
    }

    float Matrix4x4::determinant() const
    {
        return m[0][0] * minor(1, 2, 3, 1, 2, 3) - m[0][1] * minor(1, 2, 3, 0, 2, 3) +
               m[0][2] * minor(1, 2, 3, 0, 1, 3) - m[0][3] * minor(1, 2, 3, 0, 1, 2);
    }

    Vector3 Matrix4x4::toTaitBryanAngles() const
    {
        Vector3 col0 = Vector3(m[0][0], m[1][0], m[2][0]);
        Vector3 col1 = Vector3(m[0][1], m[1][1], m[2][1]);
        Vector3 col2 = Vector3(m[0][2], m[1][2], m[2][2]);
        Vector3 scale = Vector3(col0.length(), col1.length(), col2.length());

        Matrix3x3 rotMat = Matrix3x3(Vector3(m[0][0] / scale.x, m[0][1] / scale.y, m[0][1] / scale.z),
                                     Vector3(m[1][0] / scale.x, m[1][1] / scale.y, m[1][1] / scale.z),
                                     Vector3(m[2][0] / scale.x, m[2][1] / scale.y, m[2][1] / scale.z));

        return rotMat.toTaitBryanAngles();
    }

    Matrix4x4 Matrix4x4::translation(const Vector3& position)
    {
        return translation(position.x, position.y, position.z);
    }

    Matrix4x4 Matrix4x4::translation(float x, float y, float z)
    {
        Matrix4x4 mResult = Matrix4x4::Identity;
        mResult.m[0][3]   = x;
        mResult.m[1][3]   = y;
        mResult.m[2][3]   = z;
        return mResult;
    }

    Matrix4x4 Matrix4x4::scale(const Vector3& scales) { return Matrix4x4::scale(scales.x, scales.y, scales.z); }

    Matrix4x4 Matrix4x4::scale(float xs, float ys, float zs)
    {
        Matrix4x4 mResult = Matrix4x4::Identity;
        mResult.m[0][0]   = xs;
        mResult.m[1][1]   = ys;
        mResult.m[2][2]   = zs;
        return mResult;
    }

    Matrix4x4 Matrix4x4::scale(float scale) { return Matrix4x4::scale(scale, scale, scale); }

    Matrix4x4 Matrix4x4::rotationX(float radians)
    {
        return Matrix3x3::fromRotationX(radians);
    }

    Matrix4x4 Matrix4x4::rotationY(float radians)
    {
        float c = std::cos(radians);
        float s = std::sin(radians);

        return Matrix3x3(c, 0, s, 0, 1, 0, -s, 0, c);
    }

    Matrix4x4 Matrix4x4::rotationZ(float radians)
    {
        float c = std::cos(radians);
        float s = std::sin(radians);

        return Matrix3x3(c, -s, 0, s, c, 0, 0, 0, 1);
    }

    Matrix4x4 Matrix4x4::fromAxisAngle(const Vector3& axis, float angle)
    {
        Matrix3x3 out;
        out.fromAngleAxis(axis, angle);
        return out;
    }

    Matrix4x4 Matrix4x4::createPerspectiveFieldOfView(float fov, float aspectRatio, float zNearPlane, float zFarPlane)
    {
        Matrix4x4 m = Zero;

        m[0][0] = 1.0f / (aspectRatio * std::tan(fov * 0.5f));
        m[1][1] = 1.0f / std::tan(fov * 0.5f);
        m[2][2] = -zNearPlane / (zNearPlane - zFarPlane);
        m[2][3] = zFarPlane * zNearPlane / (zNearPlane - zFarPlane);
        m[3][2] = -1;

        return m;
    }

    Matrix4x4 Matrix4x4::createPerspective(float width, float height, float zNearPlane, float zFarPlane)
    {
        return createPerspectiveOffCenter(-width * 0.5f, width * 0.5f, -height * 0.5f, height * 0.5f, zNearPlane, zFarPlane);
    }

    Matrix4x4 Matrix4x4::createPerspectiveOffCenter(float left, float right, float bottom, float top, float zNearPlane, float zFarPlane)
    {
        Matrix4x4 m = Zero;

        m[0][0] = -2 * zNearPlane / (right - left);
        m[0][2] = (right + left) / (right - left);
        m[1][1] = -2 * zNearPlane / (top - bottom);
        m[1][2] = (top + bottom) / (top - bottom);
        m[2][2] = -zNearPlane / (zNearPlane - zFarPlane);
        m[2][3] = zFarPlane * zNearPlane / (zNearPlane - zFarPlane);
        m[3][2] = -1;

        return m;
    }

    Matrix4x4 Matrix4x4::createOrthographic(float width, float height, float zNearPlane, float zFarPlane)
    {
        return createOrthographicOffCenter(-width * 0.5f, width * 0.5f, -height * 0.5f, height * 0.5f, zNearPlane, zFarPlane);
    }

    Matrix4x4 Matrix4x4::createOrthographicOffCenter(float left, float right, float bottom, float top, float zNearPlane, float zFarPlane)
    {
        Matrix4x4 m = Zero;

        m[0][0] = 2 / (right - left);
        m[0][3] = -(right + left) / (right - left);
        m[1][1] = 2 / (top - bottom);
        m[1][3] = -(top + bottom) / (top - bottom);
        m[2][2] = 1 / (zFarPlane - zNearPlane);
        m[2][3] = -zNearPlane / (zFarPlane - zNearPlane);
        m[3][3] = 1;

        return m;
    }

    Matrix4x4 Matrix4x4::lookAt(const Vector3& eye, const Vector3& gaze, const Vector3& up)
    {
        Vector3 f = -Vector3::normalize(gaze);
        Vector3 s = Vector3::normalize(Vector3::cross(up, f));
        Vector3 u = Vector3::cross(f, s);

        Matrix4x4 m = Identity;

        m[0][0] = s.x;
        m[0][1] = s.y;
        m[0][2] = s.z;
        m[1][0] = u.x;
        m[1][1] = u.y;
        m[1][2] = u.z;
        m[2][0] = f.x;
        m[2][1] = f.y;
        m[2][2] = f.z;
        m[0][3] = -s.dot(eye);
        m[1][3] = -u.dot(eye);
        m[2][3] = -f.dot(eye);

        return m;
    }

    Matrix4x4 Matrix4x4::createView(const Vector3& position, const Quaternion& orientation) 
    {
        Matrix4x4 viewMatrix = Matrix4x4::Identity;

        // View matrix is:
        //
        //  [ Lx  Uy  Dz  Tx  ]
        //  [ Lx  Uy  Dz  Ty  ]
        //  [ Lx  Uy  Dz  Tz  ]
        //  [ 0   0   0   1   ]
        //
        // Where T = -(Transposed(Rot) * Pos)

        // This is most efficiently done using 3x3 Matrices
        Matrix3x3 rot = Matrix3x3::fromQuaternion(orientation);

        // Make the translation relative to new axes
        Matrix3x3 rotT  = rot.transpose();
        Vector3   trans = -rotT * position;

        // Make final matrix
        viewMatrix.setMatrix3x3(rotT); // fills upper 3x3
        viewMatrix[0][3] = trans.x;
        viewMatrix[1][3] = trans.y;
        viewMatrix[2][3] = trans.z;

        return viewMatrix;
    }

    Matrix4x4 Matrix4x4::createWorld(const Vector3& position, const Vector3& forward, const Vector3& up)
    {
        Vector3 zaxis = Vector3::normalize(-forward);
        Vector3 yaxis = Vector3::normalize(up);
        Vector3 xaxis = Vector3::cross(yaxis, zaxis);
        return Matrix4x4(xaxis.x, yaxis.x, zaxis.x, position.x,
                         xaxis.y, yaxis.y, zaxis.y, position.y,
                         xaxis.z, yaxis.z, zaxis.z, position.z,
                               0,        0,      0,        1.f);
    }

    Matrix4x4 Matrix4x4::fromQuaternion(const Quaternion& quat) { return Matrix3x3::fromQuaternion(quat); }

    Matrix4x4 Matrix4x4::lerp(const Matrix4x4& lhs, const Matrix4x4& rhs, float t)
    {
        Matrix4x4 mResult;
        for (size_t i = 0; i < 4; i++)
        {
            for (size_t j = 0; j < 4; j++)
            {
                mResult[i][j] = lhs[i][j] + (rhs[i][j] - lhs[i][j]) * t;
            }
        }
        return mResult;
    }

    Matrix4x4 Matrix4x4::makeTransform(const Vector3& position, const Vector3& scale, const Quaternion& orientation)
    {
        // Ordering:
        //    1. Scale
        //    2. Rotate
        //    3. Translate

        Matrix4x4 out;

        Matrix3x3 rot3x3 = Matrix3x3::fromQuaternion(orientation);

        // Set up final matrix with scale, rotation and translation
        out.m[0][0] = scale.x * rot3x3[0][0];
        out.m[0][1] = scale.y * rot3x3[0][1];
        out.m[0][2] = scale.z * rot3x3[0][2];
        out.m[0][3] = position.x;
        out.m[1][0] = scale.x * rot3x3[1][0];
        out.m[1][1] = scale.y * rot3x3[1][1];
        out.m[1][2] = scale.z * rot3x3[1][2];
        out.m[1][3] = position.y;
        out.m[2][0] = scale.x * rot3x3[2][0];
        out.m[2][1] = scale.y * rot3x3[2][1];
        out.m[2][2] = scale.z * rot3x3[2][2];
        out.m[2][3] = position.z;

        // No projection term
        out.m[3][0] = 0;
        out.m[3][1] = 0;
        out.m[3][2] = 0;
        out.m[3][3] = 1;

        return out;
    }

    Matrix4x4
    Matrix4x4::makeInverseTransform(const Vector3& position, const Vector3& scale, const Quaternion& orientation)
    {
        Matrix4x4 out;

        // Invert the parameters
        Vector3    inv_translate = -position;
        Vector3    inv_scale(1 / scale.x, 1 / scale.y, 1 / scale.z);
        Quaternion inv_rot = Quaternion::inverse(orientation);

        // Because we're inverting, order is translation, rotation, scale
        // So make translation relative to scale & rotation
        inv_translate = inv_rot * inv_translate; // rotate
        inv_translate *= inv_scale;              // scale

        // Next, make a 3x3 rotation matrix
        Matrix3x3 rot3x3 = Matrix3x3::fromQuaternion(inv_rot);

        // Set up final matrix with scale, rotation and translation
        out.m[0][0] = inv_scale.x * rot3x3[0][0];
        out.m[0][1] = inv_scale.x * rot3x3[0][1];
        out.m[0][2] = inv_scale.x * rot3x3[0][2];
        out.m[0][3] = inv_translate.x;
        out.m[1][0] = inv_scale.y * rot3x3[1][0];
        out.m[1][1] = inv_scale.y * rot3x3[1][1];
        out.m[1][2] = inv_scale.y * rot3x3[1][2];
        out.m[1][3] = inv_translate.y;
        out.m[2][0] = inv_scale.z * rot3x3[2][0];
        out.m[2][1] = inv_scale.z * rot3x3[2][1];
        out.m[2][2] = inv_scale.z * rot3x3[2][2];
        out.m[2][3] = inv_translate.z;

        // No projection term
        out.m[3][0] = 0;
        out.m[3][1] = 0;
        out.m[3][2] = 0;
        out.m[3][3] = 1;

        return out;
    }

    //------------------------------------------------------------------------------------------

    Quaternion& Quaternion::operator*=(const Quaternion& q)
    {
        *this = concatenate(*this, q);
        return *this;
    }

    Quaternion& Quaternion::operator/=(const Quaternion& q)
    {
        Quaternion qInv = inverse(q);
        *this           = concatenate(*this, qInv);
        return *this;
    }

    Quaternion Quaternion::operator-() const { return Quaternion(-x, -y, -z, -w); }

    Quaternion operator+(const Quaternion& q1, const Quaternion& q2)
    {
        return Quaternion(q1.w + q2.w, q1.x + q2.x, q1.y + q2.y, q1.z + q2.z);
    }

    Quaternion operator-(const Quaternion& q1, const Quaternion& q2)
    {
        return Quaternion(q1.w - q2.w, q1.x - q2.x, q1.y - q2.y, q1.z - q2.z);
    }

    Quaternion operator*(const Quaternion& q1, const Quaternion& q2) { return Quaternion::concatenate(q1, q2); }

    Quaternion operator*(const Quaternion& q, float s) { return Quaternion(q.w * s, q.x * s, q.y * s, q.z * s); }

    Quaternion operator/(const Quaternion& q1, const Quaternion& q2)
    {
        Quaternion q2Inv   = Quaternion::inverse(q2);
        Quaternion mResult = Quaternion::concatenate(q1, q2Inv);
        return mResult;
    }

    Quaternion operator*(float s, const Quaternion& q) { return Quaternion(q.w * s, q.x * s, q.y * s, q.z * s); }

    Vector3 operator*(const Quaternion& lhs, Vector3 rhs)
    {
        Quaternion p(1, rhs.x, rhs.y, rhs.z);
        Quaternion q    = lhs;
        Quaternion qInv = Quaternion::inverse(q);
        Quaternion pt   = q * p * qInv;
        return Vector3(pt.x, pt.y, pt.z);
    }

    float Quaternion::length() const { return std::sqrtf(x * x + y * y + z * z + w * w); }

    float Quaternion::squaredLength() const { return x * x + y * y + z * z + w * w; }

    Vector3 Quaternion::toTaitBryanAngles() const
    {
        const float xx = x * x;
        const float yy = y * y;
        const float zz = z * z;

        const float m00 = 1.f - 2.f * yy - 2.f * zz;

        const float m10 = 2 * x * y + 2 * w * z;
        const float m11 = 1 - 2 * xx - 2 * zz;
        const float m12 = 2 * y * z - 2 * w * x;

        const float m20 = 2.f * x * z - 2.f * y * w;
        const float m21 = 2.f * y * z + 2.f * x * w;
        const float m22 = 1.f - 2.f * xx - 2.f * yy;

        float thetaX = 0; // phi
        float thetaY = 0; // theta
        float thetaZ = 0; // psi

        if (m20 < 1)
        {
            if (m20 > -1)
            {
                thetaY = asinf(-m20);
                thetaZ = atan2f(m10, m00);
                thetaX = atan2f(m21, m22);
            }
            else // m[2][0] = -1
            {
                // Not a unique solution : thetaX − thetaZ = atan2(−r12 , r11)
                thetaY = +Math_PI / 2;
                thetaZ = -atan2f(-m12, m11);
                thetaX = 0;
            }
        }
        else // m[2][0] = +1
        {
            // Not a unique solution : thetaX + thetaZ = atan2(−r12 , r11)
            thetaY = -Math_PI / 2;
            thetaZ = atan2f(-m12, m11);
            thetaX = 0;
        }

        return Vector3(thetaX, thetaY, thetaZ);
    }

    Quaternion Quaternion::fromTaitBryanAngles(const Vector3& angles)
    {
        Matrix3x3 rotationMat = Matrix3x3::Identity;
        rotationMat.fromTaitBryanAngles(angles);
        return Quaternion::fromRotationMatrix(rotationMat);
    }

    Quaternion Quaternion::normalize(const Quaternion& q)
    {
        float invLen = 1.0f / std::sqrtf(q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w);
        return Quaternion(q.w * invLen, q.x * invLen, q.y * invLen, q.z * invLen);
    }

    Quaternion Quaternion::conjugate(const Quaternion& q) { return Quaternion(q.w , - q.x, -q.y, -q.z); }

    Quaternion Quaternion::inverse(const Quaternion& q)
    {
        float invLenSqr = 1.0f / q.squaredLength();
        return conjugate(q) * invLenSqr;
    }

    Quaternion Quaternion::fromAxes(const Vector3& x_axis, const Vector3& y_axis, const Vector3& z_axis)
    {
        Matrix3x3 rotMat;
        rotMat.fromAxes(x_axis, y_axis, z_axis);
        return Quaternion::fromRotationMatrix(rotMat);
    }

    Quaternion Quaternion::fromAxisAngle(const Vector3& axis, float angle)
    {
        float   c     = std::cos(angle * 0.5f);
        float   s     = std::sin(angle * 0.5f);
        Vector3 uaxis = Vector3::normalize(axis);
        return Quaternion(c, uaxis.x * s, uaxis.y * s, uaxis.z * s);
    }

    Quaternion Quaternion::fromRotationMatrix(const Matrix4x4& rotation)
    {
        // Algorithm in Ken Shoemake's article in 1987 SIGGRAPH course notes
        // article "Quaternion Calculus and Fast Animation".
        // https://web.mit.edu/2.998/www/QuaternionReport1.pdf

        float trace = rotation[0][0] + rotation[1][1] + rotation[2][2];

        // |w| > 1/2, may as well choose w > 1/2
        float root = std::sqrt(trace + 1.0f); // 2w
        float w    = 0.5f * root;
        root       = 0.5f / root; // 1/(4w)
        float x    = (rotation[2][1] - rotation[1][2]) * root;
        float y    = (rotation[0][2] - rotation[2][0]) * root;
        float z    = (rotation[1][0] - rotation[0][1]) * root;

        return Quaternion(w, x, y, z);
    }

    Quaternion Quaternion::fromRotationMatrix(const Matrix3x3& rotation)
    {
        // Algorithm in Ken Shoemake's article in 1987 SIGGRAPH course notes
        // article "Quaternion Calculus and Fast Animation".
        // https://web.mit.edu/2.998/www/QuaternionReport1.pdf

        float trace = rotation[0][0] + rotation[1][1] + rotation[2][2];

        // |w| > 1/2, may as well choose w > 1/2
        float root = std::sqrt(trace + 1.0f); // 2w
        float w    = 0.5f * root;
        root       = 0.5f / root; // 1/(4w)
        float x    = (rotation[2][1] - rotation[1][2]) * root;
        float y    = (rotation[0][2] - rotation[2][0]) * root;
        float z    = (rotation[1][0] - rotation[0][1]) * root;

        return Quaternion(w, x, y, z);
    }

    Quaternion Quaternion::lerp(const Quaternion& q1, const Quaternion& q2, float t) { return (1 - t) * q1 + t * q2; }

    Quaternion Quaternion::slerp(const Quaternion& q1, const Quaternion& q2, float t)
    {
        // https://www.geometrictools.com/Documentation/Quaternions.pdf
        // https://en.wikipedia.org/wiki/Slerp

        float angle = Quaternion::angle(q1, q2);
        float sa    = std::sin(angle);
        float sal   = std::sin((1 - t) * angle);
        float sar   = std::sin(t * angle);
        return sal / sa * q1 + sar / sa * q2;
    }

    Quaternion Quaternion::concatenate(const Quaternion& q1, const Quaternion& q2)
    {
        Quaternion mResult;
        mResult.w = q1.w * q2.w - q1.x * q2.x - q1.y * q2.y - q1.z * q2.z;
        mResult.x = q1.w * q2.x + q1.x * q2.w + q1.y * q2.z - q1.z * q2.y;
        mResult.y = q1.w * q2.y - q1.x * q2.z + q1.y * q2.w + q1.z * q2.x;
        mResult.z = q1.w * q2.z + q1.x * q2.y - q1.y * q2.x + q1.z * q2.w;
        return mResult;
    }

    Quaternion Quaternion::fromToRotation(const Vector3& fromDir, const Vector3& toDir)
    {
        // Melax, "The Shortest Arc Quaternion", Game Programming Gems, Charles River Media (2000).
        const Vector3 f = Vector3::normalize(fromDir);
        const Vector3 t = Vector3::normalize(toDir);

        const float dot = Vector3::dot(f, t);
        if (dot >= 1.0f)
        {
            return Quaternion::Identity;
        }
        else if (dot <= -1.0f)
        {
            Vector3 axis = Vector3::cross(f, Vector3::Right);
            if (axis.squaredLength() <= FLT_EPSILON)
            {
                axis = Vector3::cross(f, Vector3::Up);
            }
            return fromAxisAngle(axis, Math_PI);
        }
        else
        {
            Vector3     c = Vector3::cross(f, t);
            const float s = std::sqrtf((1.f + dot) * 2.f);
            Quaternion  result;
            result.x = c.x / s;
            result.y = c.y / s;
            result.z = c.z / s;
            result.w = s * 0.5f;
            return result;
        }
    }

    Quaternion Quaternion::lookRotation(const Vector3& forward, const Vector3& up)
    {
        Quaternion q1 = fromToRotation(Vector3::Forward, forward);
        Vector3    C  = Vector3::cross(forward, up);
        if (C.squaredLength() <= FLT_EPSILON)
        {
            // forward and up are co-linear
            return q1;
        }
        Vector3    U  = q1 * Vector3::Up;
        Quaternion q2 = fromToRotation(U, up);
        return q2 * q1;
    }

    float Quaternion::angle(const Quaternion& q1, const Quaternion& q2)
    {
        // We can use the conjugate here instead of inverse assuming q1 & q2 are normalized.
        Quaternion  R    = Quaternion::conjugate(q1) * q2;
        const float rs   = R.w;
        float       rLen = Vector3(R.x, R.y, R.z).length();
        return 2.0f * std::atan2f(rLen, rs);
    }

    //------------------------------------------------------------------------------------------

    AxisAlignedBox::AxisAlignedBox(const Vector3& center, const Vector3& half_extent) { update(center, half_extent); }

    void AxisAlignedBox::merge(const AxisAlignedBox& axis_aligned_box)
    { 
        merge(axis_aligned_box.getMinCorner());
        merge(axis_aligned_box.getMaxCorner());
    }

    void AxisAlignedBox::merge(const Vector3& new_point)
    {
        m_min_corner = Vector3::min(m_min_corner, new_point);
        m_max_corner = Vector3::max(m_max_corner, new_point);

        m_center      = 0.5f * (m_min_corner + m_max_corner);
        m_half_extent = m_center - m_min_corner;
    }

    void AxisAlignedBox::update(const Vector3& center, const Vector3& half_extent)
    {
        m_center      = center;
        m_half_extent = half_extent;
        m_min_corner  = center - half_extent;
        m_max_corner  = center + half_extent;
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

        R.f = Math::clamp(r, 0.0f, kMaxVal) * kF32toF16;
        G.f = Math::clamp(g, 0.0f, kMaxVal) * kF32toF16;
        B.f = Math::clamp(b, 0.0f, kMaxVal) * kF32toF16;

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
        float _r = Math::clamp(r, 0.0f, kMaxVal);
        float _g = Math::clamp(g, 0.0f, kMaxVal);
        float _b = Math::clamp(b, 0.0f, kMaxVal);

        // Compute the maximum channel, no less than 1.0*2^-15
        float MaxChannel = Math::max(Math::max(_r, _g), Math::max(_b, kMinVal));

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