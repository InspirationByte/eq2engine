//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Debug text drawer system
//
// TODO:	rewrite some parts, add geometry rendering
//			fix line rendering perfomance
//			add geometry drawer
//////////////////////////////////////////////////////////////////////////////////

#pragma once

#define DBGOVERLAY_INTERFACE_VERSION "DebugOverlay_002"

#if !defined(_RETAIL) && !defined(_PROFILE)
#define ENABLE_DEBUG_DRAWING
#endif

class IEqFont;
class IGPURenderPassRecorder;

using OnDebugDrawFn = EqFunction<bool(IGPURenderPassRecorder* rendPassRecorder)>;

struct DDGraphBucket
{
	DDGraphBucket() = default;

	DDGraphBucket(const char* name, const ColorRGB &color, float maxValue, float updateTime = 0.0f, bool dynamicScaling = false)
		: name(name), color(color), maxValue(maxValue), updateTime(updateTime), dynamic(dynamicScaling)
	{
	}

	struct DbgGraphValue
	{
		float value;
		uint color;
	};

	Array<DbgGraphValue>	values{ PP_SL };
	uint					cursor{ 0 };

	EqString				name;
	ColorRGB				color{ 0.25f };
	float					maxValue{ 1.0f };

	float					updateTime{ 0.0f };
	float					remainingTime{ 0.0f };

	bool					dynamic{ false };
};

struct DDNodeBase
{
	DDNodeBase(PPSourceLine sl) : sl(sl) {}

	PPSourceLine	sl;
	int				nameHash;
	uint			frameindex;
	float			lifetime{ 0.0f };
	bool			dispatch{ false };

	DDNodeBase&		Time(float t) { lifetime = t; return *this; }
	DDNodeBase&		Name(const char* name) { nameHash = StringId24(name); return *this; }
};

struct DDText3D : DDNodeBase
{
	~DDText3D()
	{
		if (!dispatch)
			Dispatch();
	}

	DDText3D(PPSourceLine sl) : DDNodeBase(sl) {}

	void Dispatch();

	DDText3D& Text(const char* fmt, ...)
	{
		va_list argptr;
		va_start(argptr, fmt);
		text.Append(EqString::FormatV(fmt, argptr));
		va_end(argptr);

		return *this;
	}
	DDText3D& Position(const Vector3D& v) { origin = v; return *this; }
	DDText3D& Color(const MColor& v) { color = v.pack(); return *this; }
	DDText3D& Distance(float v) { dist = v; return *this; }

	EqString		text;
	Vector3D		origin;
	float			dist{ -1 };
	uint			color{ color_white.pack() };
};

struct DDBox : DDNodeBase
{
	~DDBox()
	{
		if (!dispatch)
			Dispatch();
	}

	DDBox(PPSourceLine sl) : DDNodeBase(sl) {}

	void Dispatch();

	DDBox& Box(const BoundingBox& bbox) { mins = bbox.minPoint; maxs = bbox.maxPoint; return *this; }
	DDBox& CenterSize(const Vector3D& center, const Vector3D& size) { mins = center - size; maxs = center + size; return *this; }
	DDBox& Mins(const Vector3D& v) { mins = v; return *this; }
	DDBox& Maxs(const Vector3D& v) { maxs = v; return *this; }
	DDBox& Color(const MColor& v) { color = v.pack(); return *this; }

	Vector3D		mins;
	Vector3D		maxs;
	uint			color{ color_white.pack() };
};

struct DDOrientedBox : DDNodeBase
{
	~DDOrientedBox()
	{
		if (!dispatch)
			Dispatch();
	}

	DDOrientedBox(PPSourceLine sl) : DDNodeBase(sl) {}

	void Dispatch();

	DDOrientedBox& Mins(const Vector3D& v) { mins = v; return *this; }
	DDOrientedBox& Maxs(const Vector3D& v) { maxs = v; return *this; }
	DDOrientedBox& Box(const BoundingBox& bbox) { mins = bbox.minPoint; maxs = bbox.maxPoint; return *this; }
	DDOrientedBox& CenterSize(const Vector3D& center, const Vector3D& size) { mins = center - size; maxs = center + size; return *this; }
	DDOrientedBox& Position(const Vector3D& v) { position = v; return *this; }
	DDOrientedBox& Rotation(const Quaternion& r) { rotation = r; return *this; }
	DDOrientedBox& Color(const MColor& v) { color = v.pack(); return *this; }

	Vector3D		mins, maxs;
	Quaternion		rotation{ qidentity };
	Vector3D		position;
	uint			color{ color_white.pack() };
};

struct DDSphere : DDNodeBase
{
	~DDSphere()
	{
		if (!dispatch)
			Dispatch();
	}

	DDSphere(PPSourceLine sl) : DDNodeBase(sl) {}

	void Dispatch();

	DDSphere& Position(const Vector3D& v) { origin = v; return *this; }
	DDSphere& Radius(float v) { radius = v; return *this; }
	DDSphere& Color(const MColor& v) { color = v.pack(); return *this; }
	DDSphere& Fill(bool value) { fill = value; return *this; }

	Vector3D		origin;
	float			radius{ 1.0f };
	uint			color{ color_white.pack() };
	bool			fill{ false };
};

struct DDCylinder : DDNodeBase
{
	~DDCylinder()
	{
		if (!dispatch)
			Dispatch();
	}

	DDCylinder(PPSourceLine sl) : DDNodeBase(sl) {}

	void Dispatch();

	DDCylinder& Position(const Vector3D& v) { origin = v; return *this; }
	DDCylinder& Radius(float v) { radius = v; return *this; }
	DDCylinder& Height(float v) { height = v; return *this; }
	DDCylinder& Color(const MColor& v) { color = v.pack(); return *this; }

	Vector3D		origin;
	float			radius{ 1.0f };
	float			height{ 1.0f };
	uint			color{ color_white.pack() };
};

struct DDLine : DDNodeBase
{
	~DDLine()
	{
		if (!dispatch)
			Dispatch();
	}

	DDLine(PPSourceLine sl) : DDNodeBase(sl) {}

	void Dispatch();

	DDLine& Start(const Vector3D& v) { start = v; return *this; }
	DDLine& End(const Vector3D& v) { end = v; return *this; }
	DDLine& Color(const MColor& v) { color1 = color2 = v.pack(); return *this; }
	DDLine& ColorStart(const MColor& v) { color1 = v.pack(); return *this; }
	DDLine& ColorEnd(const MColor& v) { color2 = v.pack(); return *this; }

	Vector3D		start;
	Vector3D		end;
	uint			color1{ color_white.pack() };
	uint			color2{ color_white.pack() };
};

struct DDPoly : DDNodeBase
{
	~DDPoly()
	{
		if (!dispatch)
			Dispatch();
	}

	DDPoly(PPSourceLine sl) : DDNodeBase(sl) {}

	void Dispatch();

	DDPoly& Point(const Vector3D& v) { verts.append(v); return *this; }
	DDPoly& Points(const ArrayCRef<Vector3D> _verts) { verts.append(_verts.ptr(), _verts.numElem()); return *this; }
	DDPoly& Color(const MColor& v) { color = v.pack(); return *this; }
	DDPoly& Outline(bool v = true) { outline = v; return *this; }

	FixedArray<Vector3D, 20> verts;
	uint			color{ color_white.pack() };
	bool			outline{ false };
};

struct DDVolume : DDNodeBase
{
	~DDVolume()
	{
		if (!dispatch)
			Dispatch();
	}

	DDVolume(PPSourceLine sl) : DDNodeBase(sl) {}

	void Dispatch();

	DDVolume& Planes(const ArrayCRef<Plane> _planes) { planes.append(_planes.ptr(), _planes.numElem()); return *this; }
	DDVolume& Volume(const Volume& volume) { planes.append(volume.GetPlanes().ptr(), volume.GetPlanes().numElem()); return *this; }
	DDVolume& Color(const MColor& v) { color = v.pack(); return *this; }

	FixedArray<Plane, 20> planes;
	uint			color{ color_white.pack() };
};

#define DbgText3D()		DDText3D(PP_SL)
#define DbgBox()		DDBox(PP_SL)
#define DbgOriBox()		DDOrientedBox(PP_SL)
#define DbgSphere()		DDSphere(PP_SL)
#define DbgCylinder()	DDCylinder(PP_SL)
#define DbgLine()		DDLine(PP_SL)
#define DbgPoly()		DDPoly(PP_SL)
#define DbgVolume()		DDVolume(PP_SL)

class IDebugOverlay
{
public:
	virtual ~IDebugOverlay() = default;
	virtual IEqFont*	GetFont() = 0;

	virtual void		Init(bool hidden = true) = 0;
	virtual void		Shutdown() = 0;

	virtual void		Clear() = 0;

	virtual void		Text(const MColor& color, char const* fmt, ...) = 0;
	virtual void		TextFadeOut(int position, const MColor& color, float fFadeTime, char const* fmt, ...) = 0;

	virtual void		Add(DDText3D& prim) = 0;
	virtual void		Add(DDLine& prim) = 0;
	virtual void		Add(DDBox& prim) = 0;
	virtual void		Add(DDCylinder& prim) = 0;
	virtual void		Add(DDOrientedBox& prim) = 0;
	virtual void		Add(DDSphere& prim) = 0;
	virtual void		Add(DDPoly& prim) = 0;
	virtual void		Add(DDVolume& prim) = 0;

	virtual void		Draw2DFunc( const OnDebugDrawFn& func, float fTime = 0.0f, int hashId = 0) = 0;
	virtual void		Draw3DFunc( const OnDebugDrawFn& func, float fTime = 0.0f, int hashId = 0) = 0;

	virtual void		Graph_DrawBucket(DDGraphBucket* pBucket) = 0;
	virtual void		Graph_AddValue( DDGraphBucket* pBucket, float value) = 0;

	virtual void		SetMatrices( const Matrix4x4 &proj, const Matrix4x4 &view ) = 0;
	virtual void		Draw(int winWide, int winTall, float timeScale = 1.0f) = 0;

	// Old interfaces for compatibility

	void		Text3D(const Vector3D& origin, float distance, const MColor& color, const char* text, float fTime = 0.0f, int hashId = 0, PPSourceLine sl = PPSourceLine::Empty());
	void		Line3D(const Vector3D& start, const Vector3D& end, const MColor& color1, const MColor& color2, float fTime = 0.0f, int hashId = 0, PPSourceLine sl = PPSourceLine::Empty());
	void		Box3D(const Vector3D& mins, const Vector3D& maxs, const MColor& color, float fTime = 0.0f, int hashId = 0, PPSourceLine sl = PPSourceLine::Empty());
	void		Cylinder3D(const Vector3D& position, float radius, float height, const MColor& color, float fTime = 0.0f, int hashId = 0, PPSourceLine sl = PPSourceLine::Empty());
	void		OrientedBox3D(const Vector3D& mins, const Vector3D& maxs, const Vector3D& position, const Quaternion& rotation, const MColor& color, float fTime = 0.0f, int hashId = 0, PPSourceLine sl = PPSourceLine::Empty());
	void		Sphere3D(const Vector3D& position, float radius, const MColor& color, float fTime = 0.0f, int hashId = 0, PPSourceLine sl = PPSourceLine::Empty());
	void		Polygon3D(const Vector3D& v0, const Vector3D& v1, const Vector3D& v2, const MColor& color, bool outline = false, float fTime = 0.0f, int hashId = 0, PPSourceLine sl = PPSourceLine::Empty());
	void		Polygon3D(ArrayCRef<Vector3D> verts, const MColor& color, bool outline = false, float fTime = 0.0f, int hashId = 0, PPSourceLine sl = PPSourceLine::Empty());
	void		Volume3D(ArrayCRef<Plane> planes, const MColor& color, float fTime = 0.0f, int hashId = 0, PPSourceLine sl = PPSourceLine::Empty());
};

extern IDebugOverlay* debugoverlay;

inline void DDText3D::Dispatch()
{
	dispatch = true; debugoverlay->Add(*this);
}

inline void DDBox::Dispatch()
{
	dispatch = true; debugoverlay->Add(*this);
}

inline void DDOrientedBox::Dispatch()
{
	dispatch = true; debugoverlay->Add(*this);
}

inline void DDSphere::Dispatch()
{
	dispatch = true; debugoverlay->Add(*this);
}

inline void DDCylinder::Dispatch()
{
	dispatch = true; debugoverlay->Add(*this);
}

inline void DDLine::Dispatch()
{
	dispatch = true; debugoverlay->Add(*this);
}

inline void DDPoly::Dispatch()
{
	dispatch = true; debugoverlay->Add(*this);
}

inline void DDVolume::Dispatch()
{
	dispatch = true; debugoverlay->Add(*this);
}

inline void IDebugOverlay::Text3D(const Vector3D &origin, float dist, const MColor& color, const char* text, float fTime, int hashId, PPSourceLine sl)
{
#ifdef ENABLE_DEBUG_DRAWING
	auto dd = DDText3D(sl).Position(origin).Distance(dist).Color(color).Text(text);
	dd.nameHash = hashId;
	dd.Time(fTime);
#endif // ENABLE_DEBUG_DRAWING
}

inline void IDebugOverlay::Box3D(const Vector3D &mins, const Vector3D &maxs, const MColor& color, float fTime, int hashId, PPSourceLine sl)
{
#ifdef ENABLE_DEBUG_DRAWING
	auto dd = DDBox(sl).Mins(mins).Maxs(maxs).Color(color);
	dd.nameHash = hashId;
	dd.Time(fTime);
#endif // ENABLE_DEBUG_DRAWING
}

inline void IDebugOverlay::Cylinder3D(const Vector3D& position, float radius, float height, const MColor& color, float fTime, int hashId, PPSourceLine sl)
{
#ifdef ENABLE_DEBUG_DRAWING
	auto dd = DDCylinder(sl).Position(position).Radius(radius).Height(height).Color(color);
	dd.nameHash = hashId;
	dd.Time(fTime);
#endif // ENABLE_DEBUG_DRAWING
}

inline void IDebugOverlay::Line3D(const Vector3D &start, const Vector3D &end, const MColor& color1, const MColor& color2, float fTime, int hashId, PPSourceLine sl)
{
#ifdef ENABLE_DEBUG_DRAWING
	auto dd = DDLine(sl).Start(start).End(end).ColorStart(color1).ColorEnd(color2);
	dd.nameHash = hashId;
	dd.Time(fTime);
#endif // ENABLE_DEBUG_DRAWING
}

inline void IDebugOverlay::OrientedBox3D(const Vector3D& mins, const Vector3D& maxs, const Vector3D& position, const Quaternion& rotation, const MColor& color, float fTime, int hashId, PPSourceLine sl)
{
#ifdef ENABLE_DEBUG_DRAWING
	auto dd = DDOrientedBox(sl).Mins(mins).Maxs(maxs).Position(position).Rotation(rotation).Color(color);
	dd.nameHash = hashId;
	dd.Time(fTime);
#endif // ENABLE_DEBUG_DRAWING
}

inline void IDebugOverlay::Sphere3D(const Vector3D& position, float radius, const MColor& color, float fTime, int hashId, PPSourceLine sl)
{
#ifdef ENABLE_DEBUG_DRAWING
	auto dd = DDSphere(sl).Position(position).Radius(radius).Color(color);
	dd.nameHash = hashId;
	dd.Time(fTime);
#endif // ENABLE_DEBUG_DRAWING
}

inline void IDebugOverlay::Polygon3D(const Vector3D &v0, const Vector3D &v1,const Vector3D &v2, const MColor& color, bool outline, float fTime, int hashId, PPSourceLine sl)
{
#ifdef ENABLE_DEBUG_DRAWING
	auto dd = DDPoly(sl).Point(v0).Point(v1).Point(v2).Color(color).Outline(outline);
	dd.nameHash = hashId;
	dd.Time(fTime);
#endif // ENABLE_DEBUG_DRAWING
}

inline void IDebugOverlay::Polygon3D(ArrayCRef<Vector3D> verts, const MColor& color, bool outline, float fTime, int hashId, PPSourceLine sl)
{
#ifdef ENABLE_DEBUG_DRAWING
	auto dd = DDPoly(sl).Points(verts).Color(color).Outline(outline);
	dd.nameHash = hashId;
	dd.Time(fTime);
#endif // ENABLE_DEBUG_DRAWING
}

inline void IDebugOverlay::Volume3D(ArrayCRef<Plane> planes, const MColor& color, float fTime, int hashId, PPSourceLine sl)
{
#ifdef ENABLE_DEBUG_DRAWING
	auto dd = DDVolume(sl).Planes(planes).Color(color);
	dd.nameHash = hashId;
	dd.Time(fTime);
#endif // ENABLE_DEBUG_DRAWING
}