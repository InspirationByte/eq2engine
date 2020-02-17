//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Convex polyhedra, for engine/editor/compiler
//////////////////////////////////////////////////////////////////////////////////

#ifndef EQBRUSH_H
#define EQBRUSH_H

#include "BaseEditableObject.h"
#include "materialsystem/imaterialsystem.h"
#include "math/volume.h"
#include "math/boundingbox.h"
#include "utils/dklinkedlist.h"
#include "eqlevel.h"

enum ClassifyPoly_e
{
	CPL_FRONT = 0,
	CPL_SPLIT,
	CPL_BACK,
	CPL_ONPLANE,
};

class CEditableSurface;
class CEditableBrush;
struct EqBrushWinding_t;

inline void CopyWinding(const EqBrushWinding_t *from, EqBrushWinding_t *to);

struct EqBrushWinding_t
{
	// calculates the texture coordinates for this 
	void							CalculateTextureCoordinates();

	// sorts the vertices, makes them as triangle list
	bool							SortVerticesAsTriangleList();

	// splits the face by this face, and results a
	void							Split(const EqBrushWinding_t *w, EqBrushWinding_t *front, EqBrushWinding_t *back );

	// classifies the next face over this
	ClassifyPoly_e					Classify(EqBrushWinding_t *w);

	// makes editable surface from this winding (and no way back)
	CEditableSurface*				MakeEditableSurface();

	CEditableBrush*					pAssignedBrush;

	// assingned face
	Texture_t*						pAssignedFace;

	// vertex data
	DkList<eqlevelvertex_t>			vertices;

	// make valid assignment
	EqBrushWinding_t & operator = (const EqBrushWinding_t &u)
	{
		if(this != &u)
			CopyWinding(&u, this);

		return *this;
	}
};

inline void CopyWinding(const EqBrushWinding_t *from, EqBrushWinding_t *to)
{
	to->pAssignedBrush = from->pAssignedBrush;
	to->pAssignedFace = from->pAssignedFace;

	for(int i = 0; i < from->vertices.numElem(); i++)
		to->vertices.append(from->vertices[i]);
}

void MakeInfiniteWinding(EqBrushWinding_t &w, Plane &plane);

// editable brush class
class CEditableBrush : public CBaseEditableObject
{
public:
	CEditableBrush();
	~CEditableBrush();

// CBaseEditableObject members

	void							OnRemove(bool bOnLevelCleanup = false);

	EditableType_e					GetType() {return EDITABLE_BRUSH;}

	// basic mesh modifications
	void							Translate(Vector3D &position);		// moves all verts of model
	void							Scale(Vector3D &scale, bool use_center, Vector3D &scale_center);				// scales all verts of model
	void							Rotate(Vector3D &rotation_angles, bool use_center, Vector3D &rotation_center);	// rotates all verts of model

	// draw brush
	void							Render(int nViewRenderFlags);

	void							RenderGhost(Vector3D &addpos, Vector3D &addscale, Vector3D &addrot, bool use_center = false, Vector3D &center = vec3_zero);

	Vector3D						GetBoundingBoxMins()	{return bbox.minPoint;}
	Vector3D						GetBoundingBoxMaxs()	{return bbox.maxPoint;}

	// rendering bbox
	Vector3D						GetBBoxMins()	{return bbox.minPoint;}
	Vector3D						GetBBoxMaxs()	{return bbox.maxPoint;}

	float							CheckLineIntersection(const Vector3D &start, const Vector3D &end, Vector3D &intersectionPos);

	// returns surface textures count
	int								GetSurfaceTextureCount() {return faces.numElem();}

	// returns texture
	Texture_t*						GetSurfaceTexture(int id) {return &faces[id];}

	// updates texturing
	void							UpdateSurfaceTextures();

	// saves this object
	bool							WriteObject(IVirtualStream*	pStream);

	// read this object
	bool							ReadObject(IVirtualStream*	pStream);

	// stores object in keyvalues
	void							SaveToKeyValues(kvkeybase_t* pSection);

	// stores object in keyvalues
	bool							LoadFromKeyValues(kvkeybase_t* pSection);

	// called when whole level builds
//	void							BuildObject(level_build_data_t* pLevelBuildData);

// brush members

	// calculates bounding box for this brush
	void							CalculateBBOX();

	// is brush aabb intersects another brush aabb
	bool							IsBrushIntersectsAABB(CEditableBrush *pBrush);

	bool							IsPointInside_Epsilon(Vector3D &point, float eps);

	bool							IsPointInside(Vector3D &point);

	bool							IsWindingFullyInsideBrush(EqBrushWinding_t* pWinding);

	bool							IsWindingIntersectsBrush(EqBrushWinding_t* pWinding);

	bool							IsTouchesBrush(EqBrushWinding_t* pWinding);

	int								GetFaceCount() {return faces.numElem();}
	Texture_t*						GetFace(int nFace) {return &faces[nFace];}
	EqBrushWinding_t*				GetFacePolygon(int nFace) {return &polygons[nFace];}

	void							UpdateRenderData();
	void							UpdateRenderBuffer();

	void							AddFace(Texture_t &face);

	void							CutBrushByPlane(Plane &plane, CEditableBrush** ppNewBrush);

	void							RemoveEmptyFaces();

	// calculates the vertices from faces
	bool							CreateFromPlanes();

	// copies this object
	CBaseEditableObject*			CloneObject();

	// returns current object's flags
	int						GetFlags();

protected:

	// sort to draw
	void							SortVertsToDraw();

	BoundingBox bbox;
	DkList<Texture_t>				faces;
	DkList<EqBrushWinding_t>		polygons;

	IVertexBuffer*					m_pVB;

	int								m_nAdditionalFlags;
};

// creates a brush from volume, e.g a selection box
CEditableBrush* CreateBrushFromVolume(Volume *pVolume);


#endif // EQBRUSH_H