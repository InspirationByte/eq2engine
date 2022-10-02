//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Debug text drawer system
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "render/IDebugOverlay.h"

struct DebugTextNode_t
{
	EqString pszText;
	uint color{ color_white.pack() };
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
	Quaternion rotation{ identity() };
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
	Vector3D v0, v1, v2;
	uint color{ color_white.pack()};
};

struct DebugDrawFunc_t
{
	OnDebugDrawFn func;
	void* arg;
};

class CDebugOverlay : public IDebugOverlay
{
public:
	IEqFont*						GetFont();

	void							Init(bool hidden = true);

	void							Text(const ColorRGBA &color, char const *fmt,...);
	void							TextFadeOut(int position, const ColorRGBA &color,float fFadeTime, char const *fmt,...);

	void							Text3D(const Vector3D &origin, float dist, const ColorRGBA &color, const char* text, float fTime = 0.0f, int hashId = 0);
	void							Line3D(const Vector3D &start, const Vector3D &end, const ColorRGBA &color1, const ColorRGBA &color2, float fTime = 0.0f, int hashId = 0);
	void							Box3D(const Vector3D &mins, const Vector3D &maxs, const ColorRGBA &color1, float fTime = 0.0f, int hashId = 0);
	void							Cylinder3D(const Vector3D& position, float radius, float height, const ColorRGBA& color, float fTime = 0.0f, int hashId = 0);
	void							OrientedBox3D(const Vector3D &mins, const Vector3D &maxs, const Vector3D& position, const Quaternion& rotation, const ColorRGBA &color, float fTime = 0.0f, int hashId = 0);
	void							Sphere3D(const Vector3D& position, float radius, const ColorRGBA &color, float fTime = 0.0f, int hashId = 0);
	void							Polygon3D(const Vector3D &v0, const Vector3D &v1,const Vector3D &v2, const Vector4D &color, float fTime = 0.0f, int hashId = 0);

	void							Draw2DFunc(OnDebugDrawFn func, void* args);
	void							Draw3DFunc(OnDebugDrawFn func, void* args );

	void							Graph_DrawBucket(debugGraphBucket_t* pBucket);
	void							Graph_AddValue( debugGraphBucket_t* pBucket, float value);

	void							SetMatrices( const Matrix4x4 &proj, const Matrix4x4 &view );
	void							Draw( int winWide, int winTall, float timescale = 1.0f );
private:
	void							CleanOverlays();
	bool							CheckNodeLifetime(DebugNodeBase& node);

	Array<DebugTextNode_t>			m_TextArray{ PP_SL };
	Array<DebugText3DNode_t>		m_Text3DArray{ PP_SL };

	List<DebugFadingTextNode_t>		m_LeftTextFadeArray;
	Array<DebugFadingTextNode_t>	m_RightTextFadeArray{ PP_SL };

	Array<DebugLineNode_t>			m_LineList{ PP_SL };
	Array<DebugBoxNode_t>			m_BoxList{ PP_SL };
	Array<DebugCylinderNode_t>		m_CylinderList{ PP_SL };
	//Array<DebugOriBoxNode_t>		m_OrientedBoxList{ PP_SL };
	Array<DebugSphereNode_t>		m_SphereList{ PP_SL };

	Array<debugGraphBucket_t*>		m_graphbuckets{ PP_SL };
	Array<DebugPolyNode_t>			m_polygons{ PP_SL };

	Array<DebugDrawFunc_t>			m_draw2DFuncs{ PP_SL };
	Array<DebugDrawFunc_t>			m_draw3DFuncs{ PP_SL };

	Map<int, uint>					m_newNames{ PP_SL };

	CEqTimer						m_timer;
	IEqFont*						m_pDebugFont{ nullptr };

	Matrix4x4						m_projMat;
	Matrix4x4						m_viewMat;

	Volume							m_frustum;
	float							m_frameTime{ 0.0f };
	uint							m_frameId{ 0 };
};
