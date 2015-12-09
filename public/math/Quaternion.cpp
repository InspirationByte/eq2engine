//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Quaternion math class
//////////////////////////////////////////////////////////////////////////////////

#include "DkMath.h"
#include "Matrix.h"

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

Quaternion::Quaternion(const Matrix4x4 &matrix)
{
	float diag = matrix(0,0) + matrix(1,1) + matrix(2,2) + 1;

	if( diag > 0.0f )
	{
		float fScale = sqrtf(diag) * 2.0f; // get scale from diagonal

		float s = 1.0f / fScale;

		x = ( matrix(2,1) - matrix(1,2)) * s;
		y = ( matrix(0,2) - matrix(2,0)) * s;
		z = ( matrix(1,0) - matrix(0,1)) * s;
		w = 0.25f * fScale;
	}
	else
	{
		if ( matrix(0,0) > matrix(1,1) && matrix(0,0) > matrix(2,2))
		{
			// 1st element of diag is greatest value
			// find scale according to 1st element, and double it
			float fScale = sqrtf( 1.0f + matrix(0,0) - matrix(1,1) - matrix(2,2)) * 2.0f;

			float s = 1.0f / fScale;

			x = 0.25f * fScale;
			y = (matrix(0,1) + matrix(1,0)) * s;
			z = (matrix(2,0) + matrix(0,2)) * s;
			w = (matrix(2,1) - matrix(1,2)) * s;
		}
		else if ( matrix(1,1) > matrix(2,2))
		{
			// 2nd element of diag is greatest value
			// find scale according to 2nd element, and double it
			float fScale = sqrtf( 1.0f + matrix(1,1) - matrix(0,0) - matrix(2,2)) * 2.0f;

			float s = 1.0f / fScale;

			x = (matrix(0,1) + matrix(1,0) ) * s;
			y = 0.25f * fScale;
			z = (matrix(1,2) + matrix(2,1) ) * s;
			w = (matrix(0,2) - matrix(2,0) ) * s;
		}
		else
		{
			// 3rd element of diag is greatest value
			// find scale according to 3rd element, and double it
			float fScale = sqrtf( 1.0f + matrix(2,2) - matrix(0,0) - matrix(1,1)) * 2.0f;

			float s = 1.0f / fScale;

			x = (matrix(0,2) + matrix(2,0)) * s;
			y = (matrix(1,2) + matrix(2,1)) * s;
			z = 0.25f * fScale;
			w = (matrix(1,0) - matrix(0,1)) * s;
		}
	}

	normalize();

	/*
	// From Jason Shankel, (C) 2000.
	// this version is not compatible with my hardware skinning

	double Tr = matrix.rows[0][0] + matrix(1,1) + matrix(2,2) + 1;
	double fourD;
	double q[4];

	static int i,j,k;
	if (Tr >= 1.0)
	{
		fourD = 2.0*sqrt(Tr);
		q[3] = fourD/4.0;
		q[0] = (matrix.rows[2][1] - matrix.rows[1][2]) / fourD;
		q[1] = (matrix.rows[0][2] - matrix.rows[2][0]) / fourD;
		q[2] = (matrix.rows[1][0] - matrix.rows[0][1]) / fourD;
	}
	else
	{
		if (matrix.rows[0][0] > matrix.rows[1][1])
			i = 0;
		else
			i = 1;

		if (matrix.rows[2][2] > matrix.rows[i][i])
			i = 2;

		j = (i+1)%3;
		k = (j+1)%3;
		fourD = 2.0*sqrt(matrix.rows[i][i] - matrix.rows[j][j] - matrix.rows[k][k] + 1.0);
		q[i] = fourD / 4.0;
		q[j] = (matrix.rows[j][i] + matrix.rows[i][j]) / fourD;
		q[k] = (matrix.rows[k][i] + matrix.rows[i][k]) / fourD;
		q[3] = (matrix.rows[j][k] - matrix.rows[k][j]) / fourD;
	}

	x = q[0];
	y = q[1];
	z = q[2];
	w = q[3];
	*/
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
	float slen = w * w + x * x + y * y + z * z;

	if(slen == 1.0f)
		return;

	if(slen == 0.0f)
	{
		*this = identity();
		return;
	}

	float invLen = 1.0f / sqrtf(slen);

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

Vector4D Quaternion::asVector4D()
{
    return Vector4D(x,y,z,w);
}

#ifdef _WIN32
#define isnan _isnan
#endif

bool Quaternion::isNan() const
{
	return (isnan(x) || isnan(y) || isnan(z) || isnan(w) );
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


Quaternion slerp(const Quaternion &q0, const Quaternion &q1, const float t)
{
	double cosTheta = q0.w * q1.w + q0.x * q1.x + q0.y * q1.y + q0.z * q1.z;

	if (fabs(1 - cosTheta) < 0.001f)
	{
		return q0 * (1 - t) + q1 * t;
	}
	else
	{
		double theta = acos(cosTheta);
		return (q0 * sin((1 - t) * theta) + q1 * sin(t * theta)) / sin(theta);
	}
}


Quaternion scerp(const Quaternion &q0, const Quaternion &q1, const Quaternion &q2, const Quaternion &q3, const float t)
{
	return slerp(slerp(q1, q2, t), slerp(q0, q3, t), 2 * t * (1 - t));
}

// ------------------------------------------------------------------------------

float length(const Quaternion &q)
{
	return sqrt(q.w * q.w + q.x * q.x + q.y * q.y + q.z * q.z);
}

Vector3D eulers(const Quaternion &q)
{
	/*
	float sqw = q.w*q.w;
	float sqx = q.x*q.x;
	float sqy = q.y*q.y;
	float sqz = q.z*q.z;

	return Vector3D(atan2f(2.0f * ( q.y * q.z + q.x * q.w ) , ( -sqx - sqy + sqz + sqw )),
					-asinf(2.0f * ( q.x * q.z - q.y * q.w )),
					atan2f(2.0f * ( q.x * q.y + q.z * q.w ) , (  sqx - sqy - sqz + sqw )));
	*/
	/*
	double	r11, r21, r31, r32, r33, r12, r13;

	Vector3D out;

	double sqw = (double)q.w * (double)q.w;
	double sqx = (double)q.x * (double)q.x;
	double sqy = (double)q.y * (double)q.y;
	double sqz = (double)q.z * (double)q.z;

	r11 = sqw + sqx - sqy - sqz;
	r21 = 2.0 * ((double)q.x*(double)q.y + (double)q.w*(double)q.z);
	r31 = 2.0 * ((double)q.x*(double)q.z - (double)q.w*(double)q.y);
	r32 = 2.0 * ((double)q.y*(double)q.z + (double)q.w*(double)q.x);
	r33 = sqw - sqx - sqy + sqz;

	double tmp = abs(r31);
	if(tmp > 0.999999)
	{
		r12 = 2.0 * ((double)q.x*(double)q.y - (double)q.w*(double)q.z);
		r13 = 2.0 * ((double)q.x*(double)q.z + (double)q.w*(double)q.y);

		out.x = 0.0f; //roll
		out.y = (float) (-(PI*0.5) * r31/tmp); // pitch
		out.z = (float) atan2(-r12, -r31*r13); // yaw
		return out;
	}

	out.x = (float) atan2(r32, r33); // roll
	out.y = (float) asin(-r31);		 // pitch
	out.z = (float) atan2(r21, r11); // yaw
	return out;
	*/

	const double sqw = q.w*q.w;
	const double sqx = q.x*q.x;
	const double sqy = q.y*q.y;
	const double sqz = q.z*q.z;
	const double test = 2.0 * (q.y*q.w - q.x*q.z);

	Vector3D euler;

	if (dsimilar(test, 1.0, 0.000001))
	{
		// heading = rotation about z-axis
		euler.z = (float) (-2.0*atan2(q.x, q.w));
		// bank = rotation about x-axis
		euler.x = 0;
		// attitude = rotation about y-axis
		euler.y = (float) (PI/2.0);
	}
	else if (dsimilar(test, -1.0, 0.000001))
	{
		// heading = rotation about z-axis
		euler.z = (float) (2.0*atan2(q.x, q.w));
		// bank = rotation about x-axis
		euler.x = 0;
		// attitude = rotation about y-axis
		euler.y = (float) (PI/-2.0);
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

// compares quaternion with epsilon
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
    float	len = 1 - q.x*q.x - q.y*q.y - q.z*q.z;

    if ( len < F_EPS )
        q.w = 0;
    else
        q.w = -sqrt ( len );
}

void axisAngle(Quaternion& q, Vector3D &axis, float &angle)
{
	float scale = sqrtf(q.x*q.x + q.y*q.y + q.z*q.z);

	if (scale == 0.0f || q.w > 1.0f || q.w < -1.0f)
	{
		angle = 0.0f;
		axis.x = 0.0f;
		axis.y = 1.0f;
		axis.z = 0.0f;
	}
	else
	{
		float invscale = 1.0f / scale;
		angle = 2.0f * acosf(q.w);
		axis.x = q.x * invscale;
		axis.y = q.y * invscale;
		axis.z = q.z * invscale;
	}
}

Quaternion identity()
{
	return Quaternion(1.0f,0.0f,0.0f,0.0f);
}

#pragma warning(pop)