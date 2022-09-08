//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Vector math base (2D,3D,4D)
//////////////////////////////////////////////////////////////////////////////////

#pragma once

struct half
{
	unsigned short sh;

	half() {};
    half(const float x);
	operator float () const;
};

// Vector 2D
template <class T>
struct TVec2D
{
	T x, y;

	TVec2D<T>() {}

	template <class T2>
	TVec2D(const TVec2D<T2>& other) 
		: x((T)other.x), y((T)other.y) {}

	TVec2D<T>(const T ixy) 
		: x(ixy), y(ixy)
	{}

	TVec2D<T>(const T ix, const T iy)
		: x(ix), y(iy)
	{}

	operator T *() const { return (T *) &x; }

	template <class T2>
	void operator += (const T2 s);

	template <class T2>
	void operator += (const TVec2D<T2> &v);

	template <class T2>
	void operator -= (const T2 s);

	template <class T2>
	void operator -= (const TVec2D<T2> &v);

	template <class T2>
	void operator *= (const T2 s);

	template <class T2>
	void operator *= (const TVec2D<T2> &v);

	template <class T2>
	void operator /= (const T2 s);

	template <class T2>
	void operator /= (const TVec2D<T2> &v);
};

template <typename T, typename T2>
TVec2D<T> operator + (const TVec2D<T> &u, const TVec2D<T2> &v);

template <typename T, typename T2>
TVec2D<T> operator + (const TVec2D<T> &v, const T2 s);

template <typename T, typename T2>
TVec2D<T> operator + (const T s, const TVec2D<T2> &v);

template <typename T, typename T2>
TVec2D<T> operator - (const TVec2D<T> &u, const TVec2D<T2> &v);

template <typename T, typename T2>
TVec2D<T> operator - (const TVec2D<T> &v, const T2 s);

template <typename T, typename T2>
TVec2D<T> operator - (const T s, const TVec2D<T2> &v);

template <typename T>
TVec2D<T> operator - (const TVec2D<T> &v);

template <typename T, typename T2>
TVec2D<T> operator * (const TVec2D<T> &u, const TVec2D<T2> &v);

template <typename T, typename T2>
TVec2D<T> operator * (const T s, const TVec2D<T2> &v);

template <typename T, typename T2>
TVec2D<T> operator * (const TVec2D<T> &v, const T2 s);

template <typename T, typename T2>
TVec2D<T> operator / (const TVec2D<T> &u, const TVec2D<T2> &v);

template <typename T, typename T2>
TVec2D<T> operator / (const TVec2D<T> &v, const T2 s);

template <typename T, typename T2>
TVec2D<T> operator / (const T s, const TVec2D<T2> &v);

template <typename T, typename T2>
bool operator <= (const TVec2D<T> &u, const TVec2D<T2> &v);

template <typename T, typename T2>
bool operator >= (const TVec2D<T> &u, const TVec2D<T2> &v);

template <typename T, typename T2>
bool operator < (const TVec2D<T> &u, const TVec2D<T2> &v);

template <typename T, typename T2>
bool operator > (const TVec2D<T> &u, const TVec2D<T2> &v);

template <typename T, typename T2>
bool operator == (const TVec2D<T> &u, const TVec2D<T2> &v);

/* --------------------------------------------------------------------------------- */

template <class T>
struct TVec3D
{
	T x, y, z;

	TVec3D<T>() {}

	template <class T2>
	TVec3D(const TVec3D<T2>& other) 
		: x((T)other.x), y((T)other.y), z((T)other.z)
	{}

	TVec3D<T>(const T ixyz)
		: x(ixyz), y(ixyz), z(ixyz)
	{}

	TVec3D<T>(const T ix, const T iy, const T iz)
		: x(ix), y(iy), z(iz)
	{}

	TVec3D<T>(const TVec2D<T>& iv, const T iz)
		: x(iv.x), y(iv.y), z(iz)
	{}

	TVec3D<T>(const T ix, const TVec2D<T>& iv)
		: x(ix), y(iv.x), z(iv.y)
	{}

	operator T *() const { return (T *) &x; }

	const TVec2D<T>& xy() const { return *(TVec2D<T>*)&x; }
	const TVec2D<T>& yz() const { return *(TVec2D<T>*)&y; }
	TVec2D<T> xz() const { return TVec2D<T>(x, z); }

	template <class T2>
	void operator += (const T2 s);

	template <class T2>
	void operator += (const TVec3D<T2> &v);

	template <class T2>
	void operator -= (const T2 s);

	template <class T2>
	void operator -= (const TVec3D<T2> &v);

	template <class T2>
	void operator *= (const T2 s);

	template <class T2>
	void operator *= (const TVec3D<T2> &v);

	template <class T2>
	void operator /= (const T2 s);

	template <class T2>
	void operator /= (const TVec3D<T2> &v);
};

template <typename T, typename T2>
TVec3D<T> operator + (const TVec3D<T> &u, const TVec3D<T2> &v);

template <typename T, typename T2>
TVec3D<T> operator + (const TVec3D<T> &v, const T2 s);

template <typename T, typename T2>
TVec3D<T> operator + (const T s, const TVec3D<T2> &v);

template <typename T, typename T2>
TVec3D<T> operator - (const TVec3D<T> &u, const TVec3D<T2> &v);

template <typename T, typename T2>
TVec3D<T> operator - (const TVec3D<T> &v, const T2 s);

template <typename T, typename T2>
TVec3D<T> operator - (const T s, const TVec3D<T2> &v);

template <typename T>
TVec3D<T> operator - (const TVec3D<T> &v);

template <typename T, typename T2>
TVec3D<T> operator * (const TVec3D<T> &u, const TVec3D<T2> &v);

template <typename T, typename T2>
TVec3D<T> operator * (const T s, const TVec3D<T2> &v);

template <typename T, typename T2>
TVec3D<T> operator * (const TVec3D<T> &v, const T2 s);

template <typename T, typename T2>
TVec3D<T> operator / (const TVec3D<T> &u, const TVec3D<T2> &v);

template <typename T, typename T2>
TVec3D<T> operator / (const TVec3D<T> &v, const T2 s);

template <typename T, typename T2>
TVec3D<T> operator / (const T s, const TVec3D<T2> &v);

template <typename T, typename T2>
TVec3D<T> operator ^ (const TVec3D<T> &u, const TVec3D<T2> &v);

template <typename T, typename T2>
T operator & ( const TVec3D<T>& u, const TVec3D<T2>& v );

template <typename T, typename T2>
bool operator == (const TVec3D<T> &u, const TVec3D<T2> &v);

template <typename T, typename T2>
bool operator <= (const TVec3D<T> &u, const TVec3D<T2> &v);

template <typename T, typename T2>
bool operator >= (const TVec3D<T> &u, const TVec3D<T2> &v);

template <typename T, typename T2>
bool operator < (const TVec3D<T> &u, const TVec3D<T2> &v);

template <typename T, typename T2>
bool operator > (const TVec3D<T> &u, const TVec3D<T2> &v);


/* --------------------------------------------------------------------------------- */

template <class T>
struct TVec4D
{
	T x, y, z, w;

	TVec4D<T>() {}

	template <class T2>
	TVec4D(const TVec4D<T2>& other) 
		: x((T)other.x), y((T)other.y), z((T)other.z), w((T)other.w) 
	{}

	TVec4D<T>(const T ixyzw)
		: x(ixyzw), y(ixyzw), z(ixyzw), w(ixyzw)
	{}

	TVec4D<T>(const T ix, const T iy, const T iz, const T iw)
		: x(ix), y(iy), z(iz), w(iw)
	{}

	TVec4D<T>(const TVec2D<T>& iv, const T iz, const T iw)
		: x(iv.x), y(iv.y), z(iz), w(iw)
	{}

	TVec4D<T>(const T ix, const TVec2D<T>& iv, const T iw)
		: x(ix), y(iv.x), z(iv.y), w(iw)
	{}

	TVec4D<T>(const T ix, const T iy, const TVec2D<T>& iv)
		: x(ix), y(iy), z(iv.x), w(iv.y)
	{}

	TVec4D<T>(const TVec2D<T>& iv0, const TVec2D<T>& iv1)
		: x(iv0.x), y(iv0.y), z(iv1.x), w(iv1.y)
	{}

	TVec4D<T>(const TVec3D<T>& iv, const T iw)
		: x(iv.x), y(iv.y), z(iv.z), w(iw)
	{}

	TVec4D<T>(const T ix, const TVec3D<T>& iv)
		: x(ix), y(iv.x), z(iv.y), w(iv.z)
	{}

	operator T *() const { return (T *) &x; }

	// TVec3D<T>& xyz() { return *(TVec3D<T>*)&x; }
	// TVec3D<T>& yzw() { return *(TVec3D<T>*)&y; }

	const TVec3D<T>& xyz() const { return *(TVec3D<T>*)&x; }
	const TVec3D<T>& yzw() const { return *(TVec3D<T>*)&y; }

	const TVec2D<T>& xy() const { return *(TVec2D<T>*)&x; }
	const TVec2D<T>& yz() const { return *(TVec2D<T>*)&y; }
	const TVec2D<T>& zw() const { return *(TVec2D<T>*)&z; }

	TVec2D<T> xz() const { return TVec2D<T>(x, z); }
	TVec2D<T> xw() const { return TVec2D<T>(x, w); }
	TVec2D<T> yw() const { return TVec2D<T>(y, w); }

	template <class T2>
	void operator += (const T2 s);

	template <class T2>
	void operator += (const TVec4D<T2> &v);

	template <class T2>
	void operator -= (const T2 s);

	template <class T2>
	void operator -= (const TVec4D<T2> &v);

	template <class T2>
	void operator *= (const T2 s);

	template <class T2>
	void operator *= (const TVec4D<T2> &v);

	template <class T2>
	void operator /= (const T2 s);

	template <class T2>
	void operator /= (const TVec4D<T2> &v);
};

template <typename T, typename T2>
TVec4D<T> operator + (const TVec4D<T> &u, const TVec4D<T2> &v);

template <typename T, typename T2>
TVec4D<T> operator + (const TVec4D<T> &v, const T2 s);

template <typename T, typename T2>
TVec4D<T> operator + (const T s, const TVec4D<T2> &v);

template <typename T, typename T2>
TVec4D<T> operator - (const TVec4D<T> &u, const TVec4D<T2> &v);

template <typename T, typename T2>
TVec4D<T> operator - (const TVec4D<T> &v, const T2 s);

template <typename T, typename T2>
TVec4D<T> operator - (const T s, const TVec4D<T2> &v);

template <typename T>
TVec4D<T> operator - (const TVec4D<T> &v);

template <typename T, typename T2>
TVec4D<T> operator * (const TVec4D<T> &u, const TVec4D<T2> &v);

template <typename T, typename T2>
TVec4D<T> operator * (const T s, const TVec4D<T2> &v);

template <typename T, typename T2>
TVec4D<T> operator * (const TVec4D<T> &v, const T2 s);

template <typename T, typename T2>
TVec4D<T> operator / (const TVec4D<T> &u, const TVec4D<T2> &v);

template <typename T, typename T2>
TVec4D<T> operator / (const TVec4D<T> &v, const T2 s);

template <typename T, typename T2>
TVec4D<T> operator / (const T s, const TVec4D<T2> &v);

template <typename T, typename T2>
bool operator == (const TVec4D<T> &u, const TVec4D<T2> &v);

// define common vector types
using Vector2D  = TVec2D<float>;
using Vector3D  = TVec3D<float>;
using Vector4D  = TVec4D<float>;
using IVector2D = TVec2D<int>;
using IVector3D = TVec3D<int>;
using IVector4D = TVec4D<int>;

/* --------------------------------------------------------------------------------- */

// compares vector2 with epsilon
template <typename T>
bool		compare_epsilon(const TVec2D<T> &u, const TVec2D<T> &v, const T eps);

// compares vector3 with epsilon
template <typename T>
bool		compare_epsilon(const TVec3D<T> &u, const TVec3D<T> &v, const T eps);

// compares vector4 with epsilon
template <typename T>
bool		compare_epsilon(const TVec4D<T> &u, const TVec4D<T> &v, const T eps);

// vector2 dot product
template <typename T>
T			dot(const TVec2D<T> &u, const TVec2D<T> &v);

// vector3 dot product
template <typename T>
T			dot(const TVec3D<T> &u, const TVec3D<T> &v);

// vector4 dot product
template <typename T>
T			dot(const TVec4D<T> &u, const TVec4D<T> &v);

// linear interpolation by single value
template <typename T>
T			lerp(const T u, const T v, const float x);

// quadratic bezier interpolation
float		bezierQuadratic(const float p0, const float p1, const float p2, const float t);

// cubic bezier interpolation
float		bezierCubic(const float p0, const float t0, const float t1, const float p1, const float s);

// vector2 linear interpolation by single value
template <typename T>
TVec2D<T>	lerp(const TVec2D<T> &u, const TVec2D<T> &v, const float x);

// vector3 linear interpolation by single value
template <typename T>
TVec3D<T>	lerp(const TVec3D<T> &u, const TVec3D<T> &v, const float x);

// vector4 linear interpolation by single value
template <typename T>
TVec4D<T>	lerp(const TVec4D<T> &u, const TVec4D<T> &v, const float x);

// vector2 linear interpolation by vector value
template <typename T>
TVec2D<T>	lerp(const TVec2D<T> &u, const TVec2D<T> &v, const TVec2D<T> &x);

// vector3 linear interpolation by vector value
template <typename T>
TVec3D<T>	lerp(const TVec3D<T> &u, const TVec3D<T> &v, const TVec3D<T> &x);

// vector4 linear interpolation by vector value
template <typename T>
TVec4D<T>	lerp(const TVec4D<T> &u, const TVec4D<T> &v, const TVec4D<T> &x);

// cubic interpolation
template <typename T>
T			cerp(const T u0, const T u1, const T u2, const T u3, float x);

// vector2 cubic interpolation
template <typename T>
TVec2D<T>	cerp(const TVec2D<T> &u0, const TVec2D<T> &u1, const TVec2D<T> &u2, const TVec2D<T> &u3, float x);

// vector3 cubic interpolation
template <typename T>
TVec3D<T>	cerp(const TVec3D<T> &u0, const TVec3D<T> &u1, const TVec3D<T> &u2, const TVec3D<T> &u3, float x);

// vector4 cubic interpolation
template <typename T>
TVec4D<T>	cerp(const TVec4D<T> &u0, const TVec4D<T> &u1, const TVec4D<T> &u2, const TVec4D<T> &u3, float x);

// sign comparison
template <typename T>
T			sameSign(T a, T b);

// returns sign of value (-1 or 1)
template <typename T>
T			sign(const T v);

// returns vector2 sign of value (-1 or 1)
template <typename T>
TVec2D<T>	sign(const TVec2D<T> &v);

// returns vector3 sign of value (-1 or 1)
template <typename T>
TVec3D<T>	sign(const TVec3D<T> &v);

// returns vector4 sign of value (-1 or 1)
template <typename T>
TVec4D<T>	sign(const TVec4D<T> &v);

// returns value + step with error correction of difference overflowing a target value
template <typename T>
T			approachValue(T v, T t, T s);

// clamps value (v) to min (c0) and max (c1)
template <typename T, typename T2>
T			clamp(const T v, const T2 c0, const T2 c1);

// clamps vector2 value (v) to min (c0) and max (c1)
template <typename T, typename T2>
TVec2D<T>	clamp(const TVec2D<T> &v, const T2 c0, const T2 c1);

// clamps vector2 value (v) to min vector2 (c0) and max vector2 (c1)
template <typename T>
TVec2D<T>	clamp(const TVec2D<T> &v, const TVec2D<T> &c0, const TVec2D<T> &c1);

// clamps vector3 value (v) to min (c0) and max (c1)
template <typename T, typename T2>
TVec3D<T>	clamp(const TVec3D<T> &v, const T2 c0, const T2 c1);

// clamps vector3 value (v) to min vector3 (c0) and max vector3 (c1)
template <typename T>
TVec3D<T>	clamp(const TVec3D<T> &v, const TVec3D<T> &c0, const TVec3D<T> &c1);

// clamps vector2 value (v) to min (c0) and max (c1)
template <typename T, typename T2>
TVec4D<T>	clamp(const TVec4D<T> &v, const T2 c0, const T2 c1);

// clamps vector4 value (v) to min vector4 (c0) and max vector4 (c1)
template <typename T>
TVec4D<T>	clamp(const TVec4D<T> &v, const TVec4D<T> &c0, const TVec4D<T> &c1);

// OpenGL-like function
#define		saturate(x) clamp(x, 0.0f, 1.0f)

// normalizes vector2
template <typename T>
TVec2D<T>	normalize(const TVec2D<T> &v);

// normalizes vector3
template <typename T>
TVec3D<T>	normalize(const TVec3D<T> &v);

// normalizes vector4
template <typename T>
TVec4D<T>	normalize(const TVec4D<T> &v);

// normalizes vector2 (fast)
template <typename T>
TVec2D<T>	fastNormalize(const TVec2D<T> &v);

// normalizes vector3 (fast)
template <typename T>
TVec3D<T>	fastNormalize(const TVec3D<T> &v);

// normalizes vector4 (fast)
template <typename T>
TVec4D<T>	fastNormalize(const TVec4D<T> &v);

// computes length of vector2
template <typename T>
T			length(const TVec2D<T> &v);

// computes length of vector3
template <typename T>
T			length(const TVec3D<T> &v);

// computes length of vector4
template <typename T>
T			length(const TVec4D<T> &v);

// computes squared length of vector2
template <typename T>
T			lengthSqr(const TVec2D<T> &v);

// computes squared length of vector3
template <typename T>
T			lengthSqr(const TVec3D<T> &v);

// computes squared length of vector4
template <typename T>
T			lengthSqr(const TVec4D<T> &v);

// reflects vector3 point by normal
template <typename T>
TVec3D<T>	reflect(const TVec3D<T> &v, const TVec3D<T> &normal);

// computes distance from u to v vector2
template <typename T>
T			distance(const TVec2D<T> &u, const TVec2D<T> &v);

// computes distance from u to v vector3
template <typename T>
T			distance(const TVec3D<T> &u, const TVec3D<T> &v);

// computes distance from u to v vector4
template <typename T>
T			distance(const TVec4D<T> &u, const TVec4D<T> &v);

// performs normalized balance vector2
template <typename T>
TVec2D<T>	balance(const TVec2D<T> &v);

// performs normalized balance vector3
template <typename T>
TVec3D<T>	balance(const TVec3D<T> &v);

// performs normalized balance vector4
template <typename T>
TVec4D<T>	balance(const TVec4D<T> &v);

// computes direction vectors from angles (degrees)
void		AngleVectors(const Vector3D &angles,Vector3D *forward,Vector3D *right = nullptr,Vector3D *up = nullptr);

// computes angles from direction vector
Vector3D	VectorAngles(const Vector3D &forward);

// computes other direction vectors from one
void		VectorVectors( const Vector3D &forward, Vector3D &right, Vector3D &up );

// rotates vector by degree angles around selected center
void		VectorRotate( const Vector3D &in1, const Vector3D &angles, Vector3D *out, Vector3D *center = 0);

// rotates vector around y axis
Vector3D	RotateXZBy(const Vector3D &vector, float degrees, Vector3D &center);

// rotates vector around z axis
Vector3D	RotateXYBy(const Vector3D &vector, float degrees, Vector3D &center);

// rotates vector around x axis
Vector3D	RotateYZBy(const Vector3D &vector, float degrees, Vector3D &center);

// vector multiply and add
template <typename T>
void		VectorMA(const TVec3D<T> &veca, T scale, const TVec3D<T> &vecb, TVec3D<T> &vecc);

// computes distance to plane
template <typename T>
T			planeDistance(const TVec3D<T> &normal, const T offset, const TVec3D<T> &point);

// computes distance to plane (TVec4D<T> threats as plane equation)
template <typename T>
T			planeDistance(const TVec4D<T> &plane, const TVec3D<T> &point);

// computes S-Curve
template <typename T>
T			sCurve(const T t);

// computes fractal of value
float		fract(float value);

// crossproduct of two vectors
template <typename T>
TVec3D<T>	cross(const TVec3D<T> &u, const TVec3D<T> &v);

// returns interpolation offset (a.k.a backwards lerp)
template <typename T>
T			lineProjection(const TVec2D<T> &start, const TVec2D<T> &end, const TVec2D<T> &point);

// returns interpolation offset (a.k.a backwards lerp)
template <typename T>
T			lineProjection(const TVec3D<T> &start, const TVec3D<T> &end, const TVec3D<T> &point);

template <typename T>
bool		fixNormal(TVec3D<T> &vec);

static const TVec2D<float> vec2_zero = TVec2D<float>(0.0f);
static const TVec3D<float> vec3_zero = TVec3D<float>(0.0f);
static const TVec4D<float> vec4_zero = TVec4D<float>(0.0f);

static const TVec3D<float> vec3_undef = TVec3D<float>(F_UNDEF);
static const TVec3D<float> vec3_infinity = TVec3D<float>(F_INFINITY);

static const TVec3D<float> vec3_right = TVec3D<float>(1.0f, 0.0f, 0.0f);
static const TVec3D<float> vec3_up = TVec3D<float>(0.0f, 1.0f, 0.0f);
static const TVec3D<float> vec3_forward = TVec3D<float>(0.0f, 0.0f, 1.0f);

//-------------------------------------------------------------------

// FIXME: remove color vectors?
using ColorRGB = TVec3D<float>;
using ColorRGBA = TVec4D<float>;

struct MColor
{
	union {
		TVec4D<float> v;
		struct {
			float r, g, b, a;
		};
	};

	MColor() {}


	MColor(uint u)
		: r(float(u & 255) / 255.0f), g(float((u >> 8) & 255) / 255.0f), b(float((u >> 16) & 255) / 255.0f), a(float((u >> 24) & 255) / 255.0f) {}

	MColor(uint u, bool _ARGB)
		: b(float(u & 255) / 255.0f), g(float((u >> 8) & 255) / 255.0f), r(float((u >> 16) & 255) / 255.0f), a(float((u >> 24) & 255) / 255.0f) {}

	MColor(float r, float g, float b)
		: r(r), g(g), b(b), a(1.0f) {}

	MColor(float r, float g, float b, float a)
		: r(r), g(g), b(b), a(a) {}

	MColor(const Vector3D& rgb)
		: r(rgb.x), g(rgb.y), b(rgb.z), a(1.0f) {}

	MColor(const Vector3D& rgb, float a)
		: r(rgb.x), g(rgb.y), b(rgb.z), a(a) {}

	MColor(const Vector4D& rgba)
		: v(rgba) {}

	MColor& operator=(uint u)
	{
		r = float(u & 255) / 255.0f;
		g = float(u >> 8 & 255) / 255.0f;
		b = float(u >> 16 & 255) / 255.0f;
		a = float(u >> 16 & 255) / 255.0f;
		return *this;
	}

	MColor& operator=(const Vector4D& rgba)
	{ 
		v = rgba;
		return *this;
	}

	ColorRGB& rgb()						{ return *(Vector3D*)&r; }
	const ColorRGB& rgb() const			{ return *(Vector3D*)&r; }

	ColorRGBA& rgba()					{ return v; }
	const ColorRGBA& rgba() const		{ return v; }

	operator const ColorRGB&() const { return *(Vector3D*)&r; }
	operator const ColorRGBA&() const { return v; }

	uint pack() const 
	{
		uint cr = r * 255.0f;
		uint cg = g * 255.0f;
		uint cb = b * 255.0f;
		uint ca = a * 255.0f;
		return cr | (cg << 8) | (cb << 16) | (ca << 24);
	}
};

// converts Ting-point colors to single UINT color (as in D3D)
uint		toRGBA(const MColor& u);

// converts Ting-point colors to single UINT color (as in D3D)
uint		toBGRA(const MColor& u);

template <typename CHAR_T>
MColor		hexToColor3(CHAR_T* rgb);

template <typename CHAR_T>
MColor		hexToColor4(CHAR_T* rgba);

MColor		rgbeToRGB(unsigned char* rgbe);
uint		rgbToRGBE8(const MColor& rgb);
uint		rgbToRGB9E5(const MColor& rgb);

// pre-defined colors
static const MColor color_white		= MColor(0xFFFFFFFF, true);
static const MColor color_red		= MColor(0xFFFF0000, true);
static const MColor color_orange	= MColor(0xFFFFA500, true);
static const MColor color_yellow	= MColor(0xFFFFFF00, true);
static const MColor color_magenta	= MColor(0xFFFF00FF, true);
static const MColor color_purple	= MColor(0xFF800080, true);
static const MColor color_blue		= MColor(0xFF0000FF, true);
static const MColor color_green		= MColor(0xFF009000, true);
static const MColor color_cyan		= MColor(0xFF00FFFF, true);
static const MColor color_brown		= MColor(0xFFA52A2A, true);
static const MColor color_gray		= MColor(0xFF808080, true);

#ifndef VDEG2RAD
#	define VDEG2RAD( v )	Vector3D(DEG2RAD(v.x),DEG2RAD(v.y),DEG2RAD(v.z))
#endif

#ifndef VRAD2DEG
#	define VRAD2DEG( v )	Vector3D(RAD2DEG(v.x),RAD2DEG(v.y),RAD2DEG(v.z))
#endif