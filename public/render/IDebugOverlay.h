//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Debug text drawer system
//
// TODO:	rewrite some parts, add geometry rendering
//			fix line rendering perfomance
//			add geometry drawer
//////////////////////////////////////////////////////////////////////////////////

#pragma once

#define DBGOVERLAY_INTERFACE_VERSION "DebugOverlay_001"

class IEqFont;

struct debugGraphBucket_t
{
	debugGraphBucket_t() = default;

	debugGraphBucket_t(const char* name, const ColorRGB &color, float maxValue, float updateTime = 0.0f, bool dynamicScaling = false)
		: name(name), color(color), maxValue(maxValue), updateTime(updateTime), dynamic(dynamicScaling)
	{
	}

	struct graphVal_t
	{
		float value;
		uint color;
	};

	EqString								name;
	ColorRGB								color{ 0.25f };
	float									maxValue{ 1.0f };

	float									updateTime{ 0.0f };
	float									remainingTime{ 0.0f };

	bool									dynamic{ false };

	Array<graphVal_t>						values{ PP_SL };
	uint									cursor{ 0 };
};

typedef void (*OnDebugDrawFn)( void* );

class IDebugOverlay
{
public:
	virtual ~IDebugOverlay() = default;
	virtual IEqFont*			GetFont() = 0;

	virtual void				Init(bool hidden = true) = 0;

	virtual void				Text(const ColorRGBA &color, char const *fmt, ...) = 0;
	virtual void				TextFadeOut(int position, const ColorRGBA &color, float fFadeTime, char const *fmt, ...) = 0;

	virtual void				Text3D(const Vector3D &origin, float distance, const ColorRGBA &color, const char* text, float fTime = 0.0f, int hashId = 0) = 0;
	virtual void				Line3D(const Vector3D &start, const Vector3D &end, const ColorRGBA &color1, const ColorRGBA &color2, float fTime = 0.0f, int hashId = 0) = 0;
	virtual void				Box3D(const Vector3D &mins, const Vector3D &maxs, const ColorRGBA &color, float fTime = 0.0f, int hashId = 0) = 0;
	virtual void				Cylinder3D(const Vector3D& position, float radius, float height, const ColorRGBA& color, float fTime = 0.0f, int hashId = 0) = 0;
	virtual void				OrientedBox3D(const Vector3D& mins, const Vector3D& maxs, const Vector3D& position, const Quaternion& rotation, const ColorRGBA& color, float fTime = 0.0f, int hashId = 0) = 0;
	virtual void				Sphere3D(const Vector3D& position, float radius, const ColorRGBA &color, float fTime = 0.0f, int hashId = 0) = 0;
	virtual void				Polygon3D(const Vector3D &v0, const Vector3D &v1,const Vector3D &v2, const Vector4D &color, float fTime = 0.0f, int hashId = 0) = 0;

	virtual void				Draw2DFunc( OnDebugDrawFn func, void* args = nullptr) = 0;
	virtual void				Draw3DFunc( OnDebugDrawFn func, void* args = nullptr) = 0;

	//virtual void				Line2D(Vector2D &start, Vector2D &end, ColorRGBA &color1, ColorRGBA &color2, float fTime = 0.0f) = 0;
	//virtual void				Box2D(Vector2D &mins, Vector2D &maxs, ColorRGBA &color, float fTime = 0.0f) = 0;

	virtual void				Graph_DrawBucket(debugGraphBucket_t* pBucket) = 0;
	virtual void				Graph_AddValue( debugGraphBucket_t* pBucket, float value) = 0;

	virtual void				SetMatrices( const Matrix4x4 &proj, const Matrix4x4 &view ) = 0;
	virtual void				Draw(int winWide, int winTall, float timeScale = 1.0f) = 0;	// draws debug overlay.

	void						Draw(const Matrix4x4 &proj, const Matrix4x4 &view, int winWide, int winTall) { SetMatrices(proj, view); Draw(winWide, winTall); }
};

extern IDebugOverlay *debugoverlay;

typedef struct DbgText3DBuilder
{
	~DbgText3DBuilder() 
	{
		debugoverlay->Text3D(origin, dist, MColor(color), pszText, lifetime, hashId);
	}

	DbgText3DBuilder& Text(const char* fmt, ...) 
	{
		va_list argptr;
		va_start(argptr, fmt);
		pszText = EqString::FormatVa(fmt, argptr);
		va_end(argptr);

		return *this;
	}
	DbgText3DBuilder& Position(const Vector3D& v) { origin = v; return *this; }
	DbgText3DBuilder& Color(const MColor& v) { color = v.pack(); return *this; }
	DbgText3DBuilder& Distance(float v) { dist = v; return *this; }
	DbgText3DBuilder& Time(float t) { lifetime = t; return *this; }
	DbgText3DBuilder& Name(const char* name) { hashId = StringToHash(name); return *this; }

private:
	EqString	pszText;
	Vector3D	origin;
	float		dist{ 25.0f };
	float		lifetime{ 0.0f };
	uint		color{ color_white.pack() };
	int			hashId{ 0 };
} DbgText3D;

typedef struct DbgBoxBuilder
{
	~DbgBoxBuilder()
	{
		debugoverlay->Box3D(mins, maxs, MColor(color), lifetime, hashId);
	}

	DbgBoxBuilder& Box(const BoundingBox& bbox) { mins = bbox.minPoint; maxs = bbox.maxPoint; return *this; }
	DbgBoxBuilder& CenterSize(const Vector3D& center, const Vector3D& size) { mins = center - size; maxs = center + size; return *this; }
	DbgBoxBuilder& Mins(const Vector3D& v) { mins = v; return *this; }
	DbgBoxBuilder& Maxs(const Vector3D& v) { maxs = v; return *this; }
	DbgBoxBuilder& Color(const MColor& v) { color = v.pack(); return *this; }
	DbgBoxBuilder& Time(float t) { lifetime = t; return *this; }
	DbgBoxBuilder& Name(const char* name) { hashId = StringToHash(name); return *this; }

private:
	Vector3D mins;
	Vector3D maxs;
	uint color{ color_white.pack() };
	float lifetime{ 0.0f };
	int hashId{ 0 };
} DbgBox;

typedef struct DbgOriBoxBuilder
{
	~DbgOriBoxBuilder()
	{
		debugoverlay->OrientedBox3D(mins, maxs, position, rotation, MColor(color), lifetime, hashId);
	}	
	
	DbgOriBoxBuilder& Mins(const Vector3D& v) { mins = v; return *this; }
	DbgOriBoxBuilder& Maxs(const Vector3D& v) { maxs = v; return *this; }
	DbgOriBoxBuilder& Position(const Vector3D& v) { position = v; return *this; }
	DbgOriBoxBuilder& Rotation(const Quaternion& r) { rotation = r; return *this; }
	DbgOriBoxBuilder& Color(const MColor& v) { color = v.pack(); return *this; }
	DbgOriBoxBuilder& Time(float t) { lifetime = t; return *this; }
	DbgOriBoxBuilder& Name(const char* name) { hashId = StringToHash(name); return *this; }

private:
	Vector3D mins, maxs;
	Quaternion rotation{ identity() };
	Vector3D position;
	uint color{ color_white.pack() };
	float lifetime{ 0.0f };
	int hashId{ 0 };
} DbgOriBox;

typedef struct DbgSphereBuilder
{
	~DbgSphereBuilder()
	{
		debugoverlay->Sphere3D(origin, radius, MColor(color), lifetime, hashId);
	}

	DbgSphereBuilder& Position(const Vector3D& v) { origin = v; return *this; }
	DbgSphereBuilder& Radius(float v) { radius = v; return *this; }
	DbgSphereBuilder& Color(const MColor& v) { color = v.pack(); return *this; }
	DbgSphereBuilder& Time(float t) { lifetime = t; return *this; }
	DbgSphereBuilder& Name(const char* name) { hashId = StringToHash(name); return *this; }

private:
	Vector3D origin;
	float radius{ 1.0f };
	uint color{ color_white.pack() };
	float lifetime{ 0.0f };
	int hashId{ 0 };
} DbgSphere;

typedef struct DbgCylinderBuilder
{
	~DbgCylinderBuilder()
	{
		debugoverlay->Cylinder3D(origin, radius, height, MColor(color), lifetime, hashId);
	}

	DbgCylinderBuilder& Position(const Vector3D& v) { origin = v; return *this; }
	DbgCylinderBuilder& Radius(float v) { radius = v; return *this; }
	DbgCylinderBuilder& Height(float v) { height = v; return *this; }
	DbgCylinderBuilder& Color(const MColor& v) { color = v.pack(); return *this; }
	DbgCylinderBuilder& Time(float t) { lifetime = t; return *this; }
	DbgCylinderBuilder& Name(const char* name) { hashId = StringToHash(name); return *this; }

private:
	Vector3D origin;
	float radius{ 1.0f };
	float height{ 1.0f };
	uint color{ color_white.pack() };
	float lifetime{ 0.0f };
	int hashId{ 0 };
} DbgCylinder;

typedef struct DbgLineBuilder
{
	~DbgLineBuilder()
	{
		debugoverlay->Line3D(start, end, MColor(color1), MColor(color2), lifetime, hashId);
	}

	DbgLineBuilder& Start(const Vector3D& v) { start = v; return *this; }
	DbgLineBuilder& End(const Vector3D& v) { end = v; return *this; }
	DbgLineBuilder& Color(const MColor& v) { color1 = color2 = v.pack(); return *this; }
	DbgLineBuilder& ColorStart(const MColor& v) { color1 = v.pack(); return *this; }
	DbgLineBuilder& ColorEnd(const MColor& v) { color2 = v.pack(); return *this; }
	DbgLineBuilder& Time(float t) { lifetime = t; return *this; }
	DbgLineBuilder& Name(const char* name) { hashId = StringToHash(name); return *this; }

private:
	Vector3D start;
	Vector3D end;
	uint color1{ color_white.pack() };
	uint color2{ color_white.pack() };
	float lifetime{ 0.0f };
	int hashId{ 0 };
} DbgLine;

typedef struct DbgPolyBuilder
{
	~DbgPolyBuilder()
	{
		debugoverlay->Polygon3D(v0, v1, v2, MColor(color), lifetime, hashId);
	}

	DbgPolyBuilder& Point1(const Vector3D& v) { v0 = v; return *this; }
	DbgPolyBuilder& Point2(const Vector3D& v) { v1 = v; return *this; }
	DbgPolyBuilder& Point3(const Vector3D& v) { v2 = v; return *this; }
	DbgPolyBuilder& Points(const Vector3D& _v0, const Vector3D& _v1, const Vector3D& _v2) { v0 = _v0; v1 = _v1; v2 = _v2; return *this; }
	DbgPolyBuilder& Color(const MColor& v) { color = v.pack(); return *this; }
	DbgPolyBuilder& Time(float t) { lifetime = t; return *this; }
	DbgPolyBuilder& Name(const char* name) { hashId = StringToHash(name); return *this; }

private:
	Vector3D v0, v1, v2;
	uint color{ color_white.pack() };
	float lifetime{ 0.0f };
	int hashId{ 0 };
} DbgPoly;