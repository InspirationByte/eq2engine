//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Convex polyhedra, for engine/editor/compiler
//////////////////////////////////////////////////////////////////////////////////

#ifndef EQBRUSH_H
#define EQBRUSH_H

#include "BrushFace.h"

#include "materialsystem/IMaterialSystem.h"
#include "math/Volume.h"
#include "math/BoundingBox.h"
#include "utils/DkLinkedList.h"

#include "world.h"

enum ClassifyPoly_e
{
	CPL_FRONT = 0,
	CPL_SPLIT,
	CPL_BACK,
	CPL_ONPLANE,
};

class CBrushPrimitive;
struct winding_t;

struct winding_t
{
	brushFace_t						face;
	//DkList<lmodeldrawvertex_t>		drawVerts;

	DkList<int>						vertex_ids;		// vertex ids given by verts from GetBrushVerts / GetIntersectionWithPlanes alone 

	CBrushPrimitive*				brush;
	int								faceId;

	// calculates the texture coordinates for this 
	void							CalculateTextureCoordinates(lmodeldrawvertex_t* verts, int vertCount);

	// sorts the vertices into Triangle FAN
	bool							SortIndices();

	Vector3D						GetCenter() const;
	ClassifyPoly_e					Classify(winding_t *w) const;

	// returns vertex index of brush
	int								CheckRayIntersectionWithVertex(const Vector3D &start, const Vector3D &dir, float vertexScale);
	bool							CheckRayIntersection(const Vector3D &start, const Vector3D &dir, Vector3D &intersectionPos);
	void							Transform(const Matrix4x4& mat, bool textureLock);
};

//-------------------------------------------------------------------

// editable brush class
class CBrushPrimitive
{
	friend struct winding_t;

public:

	// creates a brush from volume, e.g a selection box
	static CBrushPrimitive*			CreateFromVolume(const Volume& volume, IMaterial* material);

	//-------------------------------------------------------------------
	CBrushPrimitive();
	~CBrushPrimitive();

	void							OnRemove();

	// draw brush
	void							Render(int nViewRenderFlags);
	void							RenderGhost(int face = -1);
	void							RenderGhostCustom(const DkList<Vector3D>& verts, int face = -1);

	// rendering bbox
	const BoundingBox&				GetBBox() const	{return m_bbox;}
	void							CalculateVerts(DkList<Vector3D>& verts);
	const DkList<Vector3D>&			GetVerts() const { return m_verts; }

	int								GetFaceCount() const { return m_windingFaces.numElem(); }
	brushFace_t*					GetFace(int nFace) const { return (brushFace_t*)&m_windingFaces[nFace].face; }

	winding_t*						GetFacePolygon(int nFace) const { return (winding_t*)&m_windingFaces[nFace]; }
	winding_t*						GetFacePolygonById(int faceId) const;
	
	float							CheckLineIntersection(const Vector3D &start, const Vector3D &end, Vector3D &intersectionPos, int& face);

	bool							IsPointInside(const Vector3D &point, int ignorePlane = -1);
	bool							IsBrushIntersectsAABB(CBrushPrimitive *pBrush);
	
	bool							IsWindingFullyInsideBrush(winding_t* pWinding);
	bool							IsWindingIntersectsBrush(winding_t* pWinding);
	bool							IsTouchesBrush(winding_t* pWinding);

	// Updates geometry. Required after individual winding modifications or AddFace
	bool							Update();

	bool							Transform(const Matrix4x4& mat, bool textureLock);
	void							AddFace(brushFace_t &face);
	bool							AdjustFacesByVerts(const DkList<Vector3D>& verts, bool textureLock);	// must be same vertex indices from GetVerts

	// copies this object
	CBrushPrimitive*				Clone();
	void							CutBrushByPlane(Plane &plane, CBrushPrimitive** ppNewBrush);

	// saves this object
	bool							WriteObject(IVirtualStream*	pStream);
	bool							ReadObject(IVirtualStream*	pStream);

	// stores object in keyvalues
	void							SaveToKeyValues(kvkeybase_t* pSection);
	bool							LoadFromKeyValues(kvkeybase_t* pSection);

protected:
	// calculates the vertices from faces
	bool							AssignWindingVertices();
	void							ValidateWindings();
	void							UpdateRenderBuffer();

	BoundingBox						m_bbox;
	DkList<Vector3D>				m_verts;
	DkList<winding_t>				m_windingFaces;

	IVertexBuffer*					m_pVB;
};


#endif // EQBRUSH_H