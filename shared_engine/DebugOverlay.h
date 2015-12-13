//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Debug text drawer system
//////////////////////////////////////////////////////////////////////////////////

#ifndef DEBUG_TEXT
#define DEBUG_TEXT

#include "IDebugOverlay.h"

#include "utils/DkList.h"
#include "utils/DkLinkedList.h"
#include "utils/eqstring.h"
#include "IFont.h"
#include "math/Vector.h"

struct DebugTextNode_t
{
	Vector4D color;
	EqString pszText;
};

struct DebugFadingTextNode_t
{
	ColorRGBA	color;
	EqString		pszText;

	// It will be set and decremented each frame
	float		m_fFadeTime;
};

struct DebugText3DNode_t
{
	Vector3D	origin;
	float		dist;
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

class CDebugOverlay : public IDebugOverlay
{
public:
									CDebugOverlay();

	IEqFont*						GetFont();

	void							Init();
	void							Text(const ColorRGBA &color, char const *fmt,...);
	void							TextFadeOut(int position, const ColorRGBA &color,float fFadeTime, char const *fmt,...);

	void							Text3D(const Vector3D &origin, const ColorRGBA &color, char const *fmt,...);
	void							Text3D(const Vector3D &origin, float dist, const ColorRGBA &color, char const *fmt,...);

	void							Line3D(const Vector3D &start, const Vector3D &end, const ColorRGBA &color1, const ColorRGBA &color2, float fTime = 0.0f);
	void							Box3D(const Vector3D &mins, const Vector3D &maxs, const ColorRGBA &color1, float fTime = 0.0f);
	void							Polygon3D(const Vector3D &v0, const Vector3D &v1,const Vector3D &v2, const Vector4D &color, float fTime = 0.0f);

	//void							Line2D(Vector2D &start, Vector2D &end, ColorRGBA &color1, ColorRGBA &color2, float fTime = 0.0f);
	//void							Box2D(Vector2D &mins, Vector2D &maxs, ColorRGBA &color1, float fTime = 0.0f);

	debugGraphBucket_t*				Graph_AddBucket( const char* pszName, const ColorRGBA &color, float fMaxValue, float fUpdateTime = 0.0f);
	void							Graph_RemoveBucket(debugGraphBucket_t* pBucket);
	void							Graph_AddValue( debugGraphBucket_t* pBucket, float value);
	debugGraphBucket_t*				Graph_FindBucket(const char* pszName);

	void							RemoveAllGraphs();

	void							Draw(const Matrix4x4 &proj, const Matrix4x4 &view, int winWide, int winTall);
private:
	void							CleanOverlays();

	IEqFont*						m_pDebugFont;

	DkList<DebugTextNode_t>			m_TextArray;
	DkList<DebugText3DNode_t>		m_Text3DArray;

	DkLinkedList<DebugFadingTextNode_t>	m_LeftTextFadeArray;
	DkList<DebugFadingTextNode_t>	m_RightTextFadeArray;

	DkList<DebugBoxNode_t>			m_BoxList;
	DkList<DebugLineNode_t>			m_LineList;

	DkList<DebugBoxNode_t>			m_FastBoxList;
	DkList<DebugLineNode_t>			m_FastLineList;

	DkList<debugGraphBucket_t*>		m_graphbuckets;
	DkList<DebugPolyNode_t>			m_polygons;

	float							m_frametime;
	float							m_oldtime;

	Threading::CEqMutex				m_mutex;
};

#endif //DEBUG_TEXT
