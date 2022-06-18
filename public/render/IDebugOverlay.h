//////////////////////////////////////////////////////////////////////////////////
// Copyright � Inspiration Byte
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
	debugGraphBucket_t()
	{
		remainingTime = 0.0f;
	}

	debugGraphBucket_t(const char* name, const ColorRGB &color, float maxValue, float updateTime = 0.0f, bool dynamicScaling = false)
		: name(name), color(color), maxValue(maxValue), updateTime(updateTime), dynamic(dynamicScaling)
	{
	}

	struct graphPoint_t
	{
		float value;
		ColorRGB color;
	};

	EqString								name;
	ColorRGB								color{ 0.25f };
	float									maxValue{ 1.0f };

	float									updateTime{ 0.0f };
	float									remainingTime{ 0.0f };

	bool									dynamic{ false };

	FixedList<graphPoint_t, 100>	points;
};

typedef void (*OnDebugDrawFn)( void* );

class IDebugOverlay
{
public:
	virtual ~IDebugOverlay() = default;
	virtual IEqFont*			GetFont() = 0;

	virtual void				Init(bool hidden = true) = 0;

	virtual void				Text(const ColorRGBA &color, char const *fmt,...) = 0;
	virtual void				TextFadeOut(int position, const ColorRGBA &color, float fFadeTime, char const *fmt,...) = 0;

	virtual void				Text3D(const Vector3D &origin, float distance, const ColorRGBA &color, float fTime, char const *fmt,...) = 0;

	virtual void				Line3D(const Vector3D &start, const Vector3D &end, const ColorRGBA &color1, const ColorRGBA &color2, float fTime = 0.0f) = 0;
	virtual void				Box3D(const Vector3D &mins, const Vector3D &maxs, const ColorRGBA &color, float fTime = 0.0f) = 0;
	virtual void				OrientedBox3D(const Vector3D& mins, const Vector3D& maxs, const Vector3D& position, const Quaternion& rotation, const ColorRGBA& color, float fTime = 0.0f) = 0;
	virtual void				Sphere3D(const Vector3D& position, float radius, const ColorRGBA &color, float fTime = 0.0f) = 0;
	virtual void				Polygon3D(const Vector3D &v0, const Vector3D &v1,const Vector3D &v2, const Vector4D &color, float fTime = 0.0f) = 0;

	virtual void				Draw2DFunc(OnDebugDrawFn func, void* args = nullptr) = 0;
	virtual void				Draw3DFunc( OnDebugDrawFn func, void* args = nullptr) = 0;

	//virtual void				Line2D(Vector2D &start, Vector2D &end, ColorRGBA &color1, ColorRGBA &color2, float fTime = 0.0f) = 0;
	//virtual void				Box2D(Vector2D &mins, Vector2D &maxs, ColorRGBA &color, float fTime = 0.0f) = 0;

	virtual void				Graph_DrawBucket(debugGraphBucket_t* pBucket) = 0;
	virtual void				Graph_AddValue( debugGraphBucket_t* pBucket, float value) = 0;

	virtual void				SetMatrices( const Matrix4x4 &proj, const Matrix4x4 &view ) = 0;
	virtual void				Draw(int winWide, int winTall) = 0;	// draws debug overlay.

	void						Draw(const Matrix4x4 &proj, const Matrix4x4 &view, int winWide, int winTall) { SetMatrices(proj, view); Draw(winWide, winTall); }
};

extern IDebugOverlay *debugoverlay;
