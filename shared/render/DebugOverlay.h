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
	EqString	text;
	uint		color{ color_white.pack() };
};

struct DebugFadingTextNode_t
{
	EqString	text;
	uint		color{ color_white.pack() };
	float		initialLifetime{ 0.0f };
	float		lifetime{ 0.0f };
};

struct DebugDrawFunc_t : public DDNodeBase
{
	DebugDrawFunc_t(PPSourceLine sl) : DDNodeBase(sl) {}
	OnDebugDrawFn func;
};

class CDebugOverlay : public IDebugOverlay
{
public:
	IEqFont*						GetFont();

	void							Init(bool hidden = true);
	void							Shutdown();

	void							Clear();

	void							Text(const MColor& color, char const* fmt, ...);
	void							TextFadeOut(int position, const MColor& color, float fFadeTime, char const* fmt, ...);

	void							Add(DDText3D& prim);
	void							Add(DDLine& prim);
	void							Add(DDBox& prim);
	void							Add(DDCylinder& prim);
	void							Add(DDOrientedBox& prim);
	void							Add(DDSphere& prim);
	void							Add(DDPoly& prim);
	void							Add(DDVolume& prim);

	void							Draw2DFunc(const OnDebugDrawFn& func, float fTime = 0.0f, int hashId = 0);
	void							Draw3DFunc(const OnDebugDrawFn& func, float fTime = 0.0f, int hashId = 0);

	void							Graph_DrawBucket(DDGraphBucket* pBucket);
	void							Graph_AddValue(DDGraphBucket* pBucket, float value);

	void							SetMatrices(const Matrix4x4& proj, const Matrix4x4& view);
	void							Draw(int winWide, int winTall, float timescale = 1.0f);

	static void						OnShowTextureChanged(ConVar* pVar, char const* pszOldValue);
private:
	void							CleanOverlays();
	bool							CheckNodeLifetime(DDNodeBase& node);

	Array<DebugTextNode_t>			m_TextArray{ PP_SL };

	List<DebugFadingTextNode_t>		m_LeftTextFadeArray{ PP_SL };
	Array<DebugFadingTextNode_t>	m_RightTextFadeArray{ PP_SL };
	Array<DDGraphBucket*>			m_graphbuckets{ PP_SL };

	Array<DDText3D>					m_Text3DArray{ PP_SL };
	Array<DDLine>					m_LineList{ PP_SL };
	Array<DDBox>					m_BoxList{ PP_SL };
	Array<DDCylinder>				m_CylinderList{ PP_SL };
	Array<DDOrientedBox>			m_OrientedBoxList{ PP_SL };
	Array<DDSphere>					m_SphereList{ PP_SL };
	Array<DDPoly>					m_polygons{ PP_SL };
	Array<DDVolume>					m_volumes{ PP_SL };

	Array<DebugDrawFunc_t>			m_draw2DFuncs{ PP_SL };
	Array<DebugDrawFunc_t>			m_draw3DFuncs{ PP_SL };

	Map<int, uint>					m_newNames{ PP_SL };
	ITexturePtr						m_dbgTexture;

	CEqTimer						m_timer;
	IEqFont*						m_debugFont{ nullptr };
	IEqFont*						m_debugFont2{ nullptr };

	Matrix4x4						m_projMat{ identity4 };
	Matrix4x4						m_viewMat{ identity4 };
	Vector3D						m_viewPos{ vec3_zero };

	Volume							m_frustum;
	float							m_frameTime{ 0.0f };
	uint							m_frameId{ 0 };
};
