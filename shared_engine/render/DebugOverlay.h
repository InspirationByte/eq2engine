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
	Vector4D color;
	EqString pszText;
};

struct DebugFadingTextNode_t
{
	ColorRGBA	color;
	EqString	pszText;

	// It will be set and decremented each frame
	float		lifetime;

	float		initialLifetime;
};

struct DebugText3DNode_t
{
	Vector3D	origin;
	float		dist;
	float		lifetime;
	ColorRGBA	color;

	EqString	pszText;
};

struct DebugBoxNode_t
{
	Vector3D mins;
	Vector3D maxs;
	ColorRGBA color;
	float lifetime;
};

struct DebugOriBoxNode_t
{
	Vector3D mins;
	Vector3D maxs;
	Quaternion rotation;
	Vector3D position;
	ColorRGBA color;
	float lifetime;
};

struct DebugSphereNode_t
{
	Vector3D origin;
	float radius;
	ColorRGBA color;
	float lifetime;
};

struct DebugLineNode_t
{
	Vector3D start;
	Vector3D end;
	ColorRGBA color1;
	ColorRGBA color2;
	float lifetime;
};

struct DebugPolyNode_t
{
	Vector3D v0;
	Vector3D v1;
	Vector3D v2;
	ColorRGBA color;
	float lifetime;
};

struct DebugDrawFunc_t
{
	OnDebugDrawFn func;
	void* arg;
};

class CDebugOverlay : public IDebugOverlay
{
public:
									CDebugOverlay();

	IEqFont*						GetFont();

	void							Init(bool hidden = true);

	void							Text(const ColorRGBA &color, char const *fmt,...);
	void							TextFadeOut(int position, const ColorRGBA &color,float fFadeTime, char const *fmt,...);

	void							Text3D(const Vector3D &origin, float dist, const ColorRGBA &color, float fTime, char const *fmt,...);

	void							Line3D(const Vector3D &start, const Vector3D &end, const ColorRGBA &color1, const ColorRGBA &color2, float fTime = 0.0f);
	void							Box3D(const Vector3D &mins, const Vector3D &maxs, const ColorRGBA &color1, float fTime = 0.0f);
	void							OrientedBox3D(const Vector3D &mins, const Vector3D &maxs, const Vector3D& position, const Quaternion& rotation, const ColorRGBA &color, float fTime = 0.0f);
	void							Sphere3D(const Vector3D& position, float radius, const ColorRGBA &color, float fTime = 0.0f);
	void							Polygon3D(const Vector3D &v0, const Vector3D &v1,const Vector3D &v2, const Vector4D &color, float fTime = 0.0f);

	void							Draw2DFunc(OnDebugDrawFn func, void* args);
	void							Draw3DFunc(OnDebugDrawFn func, void* args );

	void							Graph_DrawBucket(debugGraphBucket_t* pBucket);
	void							Graph_AddValue( debugGraphBucket_t* pBucket, float value);

	void							SetMatrices( const Matrix4x4 &proj, const Matrix4x4 &view );
	void							Draw( int winWide, int winTall );
private:
	void							CleanOverlays();

	Array<DebugTextNode_t>			m_TextArray;
	Array<DebugText3DNode_t>		m_Text3DArray;

	List<DebugFadingTextNode_t>		m_LeftTextFadeArray;
	Array<DebugFadingTextNode_t>	m_RightTextFadeArray;

	Array<DebugBoxNode_t>			m_BoxList;
	//Array<DebugOriBoxNode_t>		m_OrientedBoxList;
	Array<DebugSphereNode_t>		m_SphereList;
	Array<DebugLineNode_t>			m_LineList;

	Array<DebugBoxNode_t>			m_FastBoxList;
	//Array<DebugOriBoxNode_t>		m_FastOrientedBoxList;
	Array<DebugSphereNode_t>		m_FastSphereList;
	Array<DebugLineNode_t>			m_FastLineList;

	Array<debugGraphBucket_t*>		m_graphbuckets;
	Array<DebugPolyNode_t>			m_polygons;

	Array<DebugDrawFunc_t>			m_draw2DFuncs;
	Array<DebugDrawFunc_t>			m_draw3DFuncs;

	CEqTimer						m_timer;

	Threading::CEqMutex				m_mutex;
	IEqFont*						m_pDebugFont;

	Matrix4x4						m_projMat;
	Matrix4x4						m_viewMat;

	Volume							m_frustum;
	float							m_frameTime;
};
