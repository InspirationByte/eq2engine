//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Debug text drawer system
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "render/IDebugOverlay.h"

class ITexture;
using ITexturePtr = CRefPtr<ITexture>;

struct DebugTextNode_t
{
	EqString	pszText;
	uint		color{ color_white.pack() };
};

struct DebugFadingTextNode_t
{
	EqString	pszText;
	uint		color{ color_white.pack() };
	float		initialLifetime{ 0.0f };
	float		lifetime{ 0.0f };
};

struct DebugNodeBase
{
	int		nameHash;
	uint	frameindex;
	float	lifetime{ 0.0f };
};

struct DebugText3DNode_t : public DebugNodeBase
{
	EqString	pszText;
	Vector3D	origin;
	float		dist{ 25.0f };
	uint		color{ color_white.pack() };
};

struct DebugBoxNode_t : public DebugNodeBase
{
	Vector3D mins;
	Vector3D maxs;
	uint color{ color_white.pack() };
};

struct DebugOriBoxNode_t : public DebugNodeBase
{
	Vector3D mins, maxs;
	Quaternion rotation{ qidentity };
	Vector3D position;
	uint color{ color_white.pack() };
};

struct DebugSphereNode_t : public DebugNodeBase
{
	Vector3D origin;
	float radius{ 1.0f };
	uint color{ color_white.pack() };
};

struct DebugCylinderNode_t : public DebugNodeBase
{
	Vector3D origin;
	float radius{ 1.0f };
	float height{ 1.0f };
	uint color{ color_white.pack() };
};

struct DebugLineNode_t : public DebugNodeBase
{
	Vector3D start;
	Vector3D end;
	uint color1{ color_white.pack() };
	uint color2{ color_white.pack() };
};

struct DebugPolyNode_t : public DebugNodeBase
{
	FixedArray<Vector3D, 20> verts;
	uint color{ color_white.pack()};
};

struct DebugVolumeNode_t : public DebugNodeBase
{
	FixedArray<Plane, 20> planes;
	uint color{ color_white.pack() };
};

struct DebugDrawFunc_t : public DebugNodeBase
{
	OnDebugDrawFn func;
};

class CDebugOverlay : public IDebugOverlay
{
public:
	IEqFont*						GetFont();

	void							Init(bool hidden = true);
	void							Shutdown();

	void							Text(const MColor& color, char const* fmt, ...);
	void							TextFadeOut(int position, const MColor& color, float fFadeTime, char const* fmt, ...);

	void							Text3D(const Vector3D& origin, float dist, const MColor& color, const char* text, float fTime = 0.0f, int hashId = 0);
	void							Line3D(const Vector3D& start, const Vector3D& end, const MColor& color1, const MColor& color2, float fTime = 0.0f, int hashId = 0);
	void							Box3D(const Vector3D& mins, const Vector3D& maxs, const MColor& color1, float fTime = 0.0f, int hashId = 0);
	void							Cylinder3D(const Vector3D& position, float radius, float height, const MColor& color, float fTime = 0.0f, int hashId = 0);
	void							OrientedBox3D(const Vector3D& mins, const Vector3D& maxs, const Vector3D& position, const Quaternion& rotation, const MColor& color, float fTime = 0.0f, int hashId = 0);
	void							Sphere3D(const Vector3D& position, float radius, const MColor& color, float fTime = 0.0f, int hashId = 0);
	void							Polygon3D(const Vector3D& v0, const Vector3D& v1, const Vector3D& v2, const MColor& color, float fTime = 0.0f, int hashId = 0);
	void							Polygon3D(ArrayCRef<Vector3D> verts, const MColor& color, float fTime = 0.0f, int hashId = 0);
	void							Volume3D(ArrayCRef<Plane> planes, const MColor& color, float fTime = 0.0f, int hashId = 0);

	void							Draw2DFunc(const OnDebugDrawFn& func, float fTime = 0.0f, int hashId = 0);
	void							Draw3DFunc(const OnDebugDrawFn& func, float fTime = 0.0f, int hashId = 0);

	void							Graph_DrawBucket(DbgGraphBucket* pBucket);
	void							Graph_AddValue(DbgGraphBucket* pBucket, float value);

	void							SetMatrices(const Matrix4x4& proj, const Matrix4x4& view);
	void							Draw(int winWide, int winTall, float timescale = 1.0f);

	static void						OnShowTextureChanged(ConVar* pVar, char const* pszOldValue);
private:
	void							CleanOverlays();
	bool							CheckNodeLifetime(DebugNodeBase& node);

	Array<DebugTextNode_t>			m_TextArray{ PP_SL };
	Array<DebugText3DNode_t>		m_Text3DArray{ PP_SL };

	List<DebugFadingTextNode_t>		m_LeftTextFadeArray{ PP_SL };
	Array<DebugFadingTextNode_t>	m_RightTextFadeArray{ PP_SL };

	Array<DebugLineNode_t>			m_LineList{ PP_SL };
	Array<DebugBoxNode_t>			m_BoxList{ PP_SL };
	Array<DebugCylinderNode_t>		m_CylinderList{ PP_SL };
	Array<DebugOriBoxNode_t>		m_OrientedBoxList{ PP_SL };
	Array<DebugSphereNode_t>		m_SphereList{ PP_SL };

	Array<DbgGraphBucket*>			m_graphbuckets{ PP_SL };
	Array<DebugPolyNode_t>			m_polygons{ PP_SL };
	Array<DebugVolumeNode_t>		m_volumes{ PP_SL };

	Array<DebugDrawFunc_t>			m_draw2DFuncs{ PP_SL };
	Array<DebugDrawFunc_t>			m_draw3DFuncs{ PP_SL };

	Map<int, uint>					m_newNames{ PP_SL };
	ITexturePtr						m_dbgTexture;

	CEqTimer						m_timer;
	IEqFont*						m_debugFont{ nullptr };
	IEqFont*						m_debugFont2{ nullptr };

	Matrix4x4						m_projMat;
	Matrix4x4						m_viewMat;

	Volume							m_frustum;
	float							m_frameTime{ 0.0f };
	uint							m_frameId{ 0 };
};
