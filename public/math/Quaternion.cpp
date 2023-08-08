//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Quaternion math class
//////////////////////////////////////////////////////////////////////////////////

#include "math_common.h"
#include "Quaternion.h"

#pragma warning(push)
#pragma warning(disable:4244)

Quaternion::Quaternion(const float Wx, const float Wy)
{
	float cx,sx,cy,sy;

	SinCos(Wx * 0.5f, &sx, &cx);
	SinCos(Wy * 0.5f, &sy, &cy);

	x = -cy * sx;
	y =  cx * sy;
	z =  sx * sy;

	w =  cx * cy;
}

Quaternion::Quaternion(const float Wx, const float Wy, const float Wz)
{
	double cr,sr,cp,sp,cy,sy;

	SinCos(Wx * 0.5f, &sr, &cr);
	SinCos(Wy * 0.5f, &sp, &cp);
	SinCos(Wz * 0.5f, &sy, &cy);

	double cpcy = cp * cy;
	double spcy = sp * cy;
	double cpsy = cp * sy;
	double spsy = sp * sy;

	x = (float)(sr * cpcy - cr * spsy);
	y = (float)(cr * spcy + sr * cpsy);
	z = (float)(cr * cpsy - sr * spcy);
	w = (float)(cr * cpcy + sr * spsy);

	normalize();
}

Quaternion::Quaternion(const Matrix3x3 &matrix)
{

// METHOD 3

	float trace, s;

    trace = matrix(0,0) + matrix(1,1) + matrix(2,2);
    if (trace > 0.0f)
    {
        s = sqrtf(trace + 1.0f);
        w = s * 0.5f;
        s = 0.5f / s;

        x = (matrix(2,1) - matrix(1,2)) * s;
        y = (matrix(0,2) - matrix(2,0)) * s;
        z = (matrix(1,0) - matrix(0,1)) * s;
    }
    else
    {
		constexpr const float TRACE_QZERO_TOLERANCE = 0.1f;

        int biggest;
        enum { A, E, I };
        if (matrix(0,0) > matrix(1,1))
        {
            if (matrix(2,2) > matrix(0,0))
                biggest = I;
            else
                biggest = A;
        }
        else
        {
            if (matrix(2,2) > matrix(0,0))
                biggest = I;
            else
                biggest = E;
        }

        // in the unusual case the original trace fails to produce a good sqrt, try others...
        switch (biggest)
        {
        case A:
            s = sqrtf(matrix(0,0) - (matrix(1,1) + matrix(2,2)) + 1.0f);
            if (s > TRACE_QZERO_TOLERANCE)
            {
                x = s * 0.5f;
                s = 0.5f / s;
                w = (matrix(2,1) - matrix(1,2)) * s;
                y = (matrix(0,1) + matrix(1,0)) * s;
                z = (matrix(0,2) + matrix(2,0)) * s;
                break;
            }
            // I
            s = sqrtf(matrix(2,2) - (matrix(0,0) + matrix(1,1)) + 1.0f);
            if (s > TRACE_QZERO_TOLERANCE)
            {
                z = s * 0.5f;
                s = 0.5f / s;
                w = (matrix(1,0) - matrix(0,1)) * s;
                x = (matrix(2,0) + matrix(0,2)) * s;
                y = (matrix(2,1) + matrix(1,2)) * s;
                break;
            }
            // E
            s = sqrtf(matrix(1,1) - (matrix(2,2) + matrix(0,0)) + 1.0f);
            if (s > TRACE_QZERO_TOLERANCE)
            {
                y = s * 0.5f;
                s = 0.5f / s;
                w = (matrix(0,2) - matrix(2,0)) * s;
                z = (matrix(1,2) + matrix(2,1)) * s;
                x = (matrix(1,0) + matrix(0,1)) * s;
                break;
            }
            break;
        case E:
            s = sqrtf(matrix(1,1) - (matrix(2,2) + matrix(0,0)) + 1.0f);
            if (s > TRACE_QZERO_TOLERANCE)
            {
                y = s * 0.5f;
                s = 0.5f / s;
                w = (matrix(0,2) - matrix(2,0)) * s;
                z = (matrix(1,2) + matrix(2,1)) * s;
                x = (matrix(1,0) + matrix(0,1)) * s;
                break;
            }
            // I
            s = sqrtf(matrix(2,2) - (matrix(0,0) + matrix(1,1)) + 1.0f);
            if (s > TRACE_QZERO_TOLERANCE)
            {
                z = s * 0.5f;
                s = 0.5f / s;
                w = (matrix(1,0) - matrix(0,1)) * s;
                x = (matrix(2,0) + matrix(0,2)) * s;
                y = (matrix(2,1) + matrix(1,2)) * s;
                break;
            }
            // A
            s = sqrtf(matrix(0,0) - (matrix(1,1) + matrix(2,2)) + 1.0f);
            if (s > TRACE_QZERO_TOLERANCE)
            {
                x = s * 0.5f;
                s = 0.5f / s;
                w = (matrix(2,1) - matrix(1,2)) * s;
                y = (matrix(0,1) + matrix(1,0)) * s;
                z = (matrix(0,2) + matrix(2,0)) * s;
                break;
            }
            break;
        case I:
            s = sqrtf(matrix(2,2) - (matrix(0,0) + matrix(1,1)) + 1.0f);
            if (s > TRACE_QZERO_TOLERANCE)
            {
                z = s * 0.5f;
                s = 0.5f / s;
                w = (matrix(1,0) - matrix(0,1)) * s;
                x = (matrix(2,0) + matrix(0,2)) * s;
                y = (matrix(2,1) + matrix(1,2)) * s;
                break;
            }
            // A
            s = sqrtf(matrix(0,0) - (matrix(1,1) + matrix(2,2)) + 1.0f);
            if (s > TRACE_QZERO_TOLERANCE)
            {
                x = s * 0.5f;
                s = 0.5f / s;
                w = (matrix(2,1) - matrix(1,2)) * s;
                y = (matrix(0,1) + matrix(1,0)) * s;
                z = (matrix(0,2) + matrix(2,0)) * s;
                break;
            }
            // E
            s = sqrtf(matrix(1,1) - (matrix(2,2) + matrix(0,0)) + 1.0f);
            if (s > TRACE_QZERO_TOLERANCE)
            {
                y = s * 0.5f;
                s = 0.5f / s;
                w = (matrix(0,2) - matrix(2,0)) * s;
                z = (matrix(1,2) + matrix(2,1)) * s;
                x = (matrix(1,0) + matrix(0,1)) * s;
                break;
            }
            break;
        }
    }
}


Quaternion::Quaternion(const Vector4D &v)
{
	w = v.w;
	x = v.x;
	y = v.y;
	z = v.z;
}


Quaternion::Quaternion(const float a, const Vector3D& axis)
{
	double sa = sin( a * 0.5f);

	x = axis.x * sa;
	y = axis.y * sa;
	z = axis.z * sa;

	w = cosf( a * 0.5f);
}

void Quaternion::operator += (const Quaternion &v)
{
	x += v.x;
	y += v.y;
	z += v.z;
	w += v.w;
}

void Quaternion::operator -= (const Quaternion &v)
{
	x -= v.x;
	y -= v.y;
	z -= v.z;
	w -= v.w;
}

void Quaternion::operator *= (const float s)
{
	x *= s;
	y *= s;
	z *= s;
	w *= s;
}

void Quaternion::operator /= (const float d)
{
	x /= d;
	y /= d;
	z /= d;
	w /= d;
}

void Quaternion::normalize()
{
	const float slen = w * w + x * x + y * y + z * z;

	if(slen == 1.0f)
		return;

	if(slen == 0.0f)
	{
		*this = identity();
		return;
	}

	const float invLen = 1.0f / sqrtf(slen);

	x *= invLen;
	y *= invLen;
	z *= invLen;
	w *= invLen;
}

void Quaternion::fastNormalize()
{
	float invLen = rsqrtf(w * w + x * x + y * y + z * z);

	x *= invLen;
	y *= invLen;
	z *= invLen;
	w *= invLen;
}

Vector4D& Quaternion::asVector4D() const
{
    return *(Vector4D*)this;
}

Quaternion operator ! (const Quaternion &q)
{
	return Quaternion(q.w, -q.x, -q.y, -q.z);
}

bool Quaternion::isNan() const
{
	return (IsNaN(x) || IsNaN(y) || IsNaN(z) || IsNaN(w) );
}

Quaternion operator + (const Quaternion &u, const Quaternion &v)
{
	return Quaternion(u.w + v.w, u.x + v.x, u.y + v.y, u.z + v.z);
}

Quaternion operator - (const Quaternion &u, const Quaternion &v)
{
	return Quaternion(u.w - v.w, u.x - v.x, u.y - v.y, u.z - v.z);
}

Quaternion operator - (const Quaternion &v)
{
	return Quaternion(-v.w, -v.x, -v.y, -v.z);
}

Quaternion operator * (const Quaternion &u, const Quaternion &v)
{
	Quaternion tmp;

	tmp.w = (v.w * u.w) - (v.x * u.x) - (v.y * u.y) - (v.z * u.z);
	tmp.x = (v.w * u.x) + (v.x * u.w) + (v.y * u.z) - (v.z * u.y);
	tmp.y = (v.w * u.y) + (v.y * u.w) + (v.z * u.x) - (v.x * u.z);
	tmp.z = (v.w * u.z) + (v.z * u.w) + (v.x * u.y) - (v.y * u.x);

	return tmp;
}

Quaternion operator * (const float scalar, const Quaternion &v)
{
	return Quaternion(v.w * scalar, v.x * scalar, v.y * scalar, v.z * scalar);
}

Quaternion operator * (const Vector3D& v, const Quaternion &q)
{
	return Quaternion(- v.x * q.x - v.y * q.y - v.z * q.z,	// w
						v.x * q.w + v.y * q.z - v.z * q.y,	// x
						v.y * q.w + v.z * q.x - v.x * q.z,	// y
						v.z * q.w + v.x * q.y - v.y * q.x);	// z
}

Quaternion operator * (const Quaternion &v, const float s)
{
	return Quaternion(v.w * s, v.x * s, v.y * s, v.z * s);
}

Quaternion operator / (const Quaternion &v, const float d)
{
	return Quaternion(v.w / d, v.x / d, v.y / d, v.z / d);
}


Quaternion slerp(const Quaternion &x, const Quaternion &y, const float a)
{
	double cosTheta = dot(x.asVector4D(), y.asVector4D());

	Quaternion z = y;

	// If cosTheta < 0, the interpolation will take the long way around the sphere. 
	// To fix this, one quat must be negated.
	if (cosTheta < 0)
	{
		z = -y;
		cosTheta = -cosTheta;
	}

	if (fabs(1.0f - cosTheta) < F_EPS)
	{
		// perform linear interpolation
		return x * (1.0 - a) + y * a;
	}
	else
	{
		double theta = acos(cosTheta);
		return (x * sin((1.0 - a) * theta) + z * sin(a * theta)) / sin(theta);
	}
}


Quaternion scerp(const Quaternion &q0, const Quaternion &q1, const Quaternion &q2, const Quaternion &q3, const float t)
{
	return slerp(slerp(q1, q2, t), slerp(q0, q3, t), 2.0 * t * (1.0 - t));
}

// inverses quaternion
Quaternion inverse(const Quaternion& q)
{
	const float len = 1.0f / length(q);
	const Quaternion cq = !q; // find conjugate

	return Quaternion(cq.w * len, cq.x * len, cq.y * len, cq.z * len);
}			
// ------------------------------------------------------------------------------

float length(const Quaternion &q)
{
	return sqrt(q.w * q.w + q.x * q.x + q.y * q.y + q.z * q.z);
}

void threeAxisRot(float r11, float r12, float r21, float r31, float r32, float* res)
{
	res[0] = atan2f(r31, r32);
	res[1] = asinf(r21);
	res[2] = atan2f(r11, r12);
}


void quaternionToEulers(const Quaternion& q, EQuatRotationSequence seq, float res[3])
{
	switch(seq)
	{
		case QuatRot_zyx:
		{
			threeAxisRot(2 * (q.x * q.y + q.w * q.z),
				q.w * q.w + q.x * q.x - q.y * q.y - q.z * q.z,
				-2 * (q.x * q.z - q.w * q.y),
				2 * (q.y * q.z + q.w * q.x),
				q.w * q.w - q.x * q.x - q.y * q.y + q.z * q.z, 
				res);
			break;
		}
		case QuatRot_zxy:
		{
			threeAxisRot(-2 * (q.x * q.y - q.w * q.z),
				q.w * q.w - q.x * q.x + q.y * q.y - q.z * q.z,
				2 * (q.y * q.z + q.w * q.x),
				-2 * (q.x * q.z - q.w * q.y),
				q.w * q.w - q.x * q.x - q.y * q.y + q.z * q.z, 
				res);

			break;
		}
		case QuatRot_yxz:
		{
			threeAxisRot(2 * (q.x * q.z + q.w * q.y),
					 q.w * q.w - q.x * q.x - q.y * q.y + q.z * q.z,
					-2 * (q.y * q.z - q.w * q.x),
					 2 * (q.x * q.y + q.w * q.z),
					 q.w * q.w - q.x * q.x + q.y * q.y - q.z * q.z, 
				res);
			break;
		}
		case QuatRot_yzx:
		{
			threeAxisRot(-2 * (q.x * q.z - q.w * q.y),
				q.w * q.w + q.x * q.x - q.y * q.y - q.z * q.z,
				2 * (q.x * q.y + q.w * q.z),
				-2 * (q.y * q.z - q.w * q.x),
				q.w * q.w - q.x * q.x + q.y * q.y - q.z * q.z,
				res);

			break;
		}
		case QuatRot_xyz:
		{
			threeAxisRot(-2 * (q.y * q.z - q.w * q.x),
				q.w * q.w - q.x * q.x - q.y * q.y + q.z * q.z,
				2 * (q.x * q.z + q.w * q.y),
				-2 * (q.x * q.y - q.w * q.z),
				q.w * q.w + q.x * q.x - q.y * q.y - q.z * q.z, 
				res);

			break;
		}
		case QuatRot_xzy:
		{
			threeAxisRot(2 * (q.y * q.z + q.w * q.x),
				q.w * q.w - q.x * q.x + q.y * q.y - q.z * q.z,
				-2 * (q.x * q.y - q.w * q.z),
				2 * (q.x * q.z + q.w * q.y),
				q.w * q.w + q.x * q.x - q.y * q.y - q.z * q.z, 
				res);
			break;
		}
		default:
		{
			res[0] = 0.0f;
			res[1] = 0.0f;
			res[2] = 0.0f;
		}
	}
}

Vector3D eulersXYZ(const Quaternion &q)
{
	constexpr const double Q_D_EPS = 0.000001;

	const double sqw = q.w*q.w;
	const double sqx = q.x*q.x;
	const double sqy = q.y*q.y;
	const double sqz = q.z*q.z;
	const double test = 2.0 * (q.y*q.w - q.x*q.z);

	Vector3D euler;

	if (dsimilar(test, 1.0, Q_D_EPS))
	{
		// heading = rotation about z-axis
		euler.z = (float) (-2.0*atan2(q.x, q.w));
		// bank = rotation about x-axis
		euler.x = 0;
		// attitude = rotation about y-axis
		euler.y = (float) (M_PI_D * 0.5);
	}
	else if (dsimilar(test, -1.0, Q_D_EPS))
	{
		// heading = rotation about z-axis
		euler.z = (float) (2.0 * atan2(q.x, q.w));
		// bank = rotation about x-axis
		euler.x = 0;
		// attitude = rotation about y-axis
		euler.y = (float) (M_PI_D * -0.5);
	}
	else
	{
		// heading = rotation about z-axis
		euler.z = (float) atan2(2.0 * (q.x*q.y +q.z*q.w),(sqx - sqy - sqz + sqw));
		// bank = rotation about x-axis
		euler.x = (float) atan2(2.0 * (q.y*q.z +q.x*q.w),(-sqx - sqy + sqz + sqw));
		// attitude = rotation about y-axis
		euler.y = (float) asin( clamp(test, -1.0, 1.0) );
	}

	return euler;
}

// compares two quaternions with epsilon
bool compare_epsilon(const Quaternion &u, const Quaternion &v, const float eps)
{
	// FIXME: use quaternion difference
	return	fsimilar(u.x,v.x, eps) &&
			fsimilar(u.y,v.y, eps) &&
			fsimilar(u.z,v.z, eps) &&
			fsimilar(u.w,v.w, eps);
}

void renormalize(Quaternion& q)
{
	const float len = 1.0f - q.x*q.x - q.y*q.y - q.z*q.z;

	if ( len < F_EPS )
		q.w = 0;
	else
		q.w = -sqrtf( len );
}

void axisAngle(const Quaternion& q, Vector3D &axis, float &angle)
{
	const float scale = sqrtf(q.x*q.x + q.y*q.y + q.z*q.z);

	if (scale == 0.0f || q.w > 1.0f || q.w < -1.0f)
	{
		angle = 0.0f;
		axis.x = 0.0f;
		axis.y = 1.0f;
		axis.z = 0.0f;
	}
	else
	{
		const float invscale = 1.0f / scale;
		angle = 2.0f * acosf(q.w);
		axis.x = q.x * invscale;
		axis.y = q.y * invscale;
		axis.z = q.z * invscale;
	}
}

// vector rotation
Vector3D rotateVector( const Vector3D& p, const Quaternion& q )
{
    Quaternion temp = q * Quaternion( 0.0f, p.x, p.y, p.z );
    return ( temp * !q ).asVector4D().xyz();
}

Quaternion identity()
{
	return Quaternion(1.0f,0.0f,0.0f,0.0f);
}

#pragma warning(pop)