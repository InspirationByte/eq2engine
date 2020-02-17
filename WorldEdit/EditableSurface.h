//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Editable mesh class
//////////////////////////////////////////////////////////////////////////////////


#ifndef EDITABLEMESH_H
#define EDITABLEMESH_H

#include "BaseEditableObject.h"
#include "materialsystem/IMaterialSystem.h"
#include "eqlevel.h"

enum EditableSurfaceType_e
{
	SURFACE_INVALID = -1,	// invalid surfaces. Can't be rendered

	SURFACE_NORMAL,		// normal surface. All properties are modifiable. This type also used by terrain patches. Texture coordinates are from dialog
	SURFACE_MODEL,		// imported (as model, so only the texture coordinates can't be modified)
};

struct tesselationdata_t
{
	tesselationdata_t()
	{
		nTesselation = -1;

		nVerts = 0;
		nIndices = 0;

		pVertexData = NULL;
		pIndexData = NULL;
	}

	void Destroy();

	int					nTesselation;

	eqlevelvertex_t*	pVertexData;
	int*				pIndexData;

	int					nVerts;
	int					nIndices;
};

/*
struct detaillayer_t
{
	Vector4D*		vertexData;
	detailtype_t
};
*/

// The basic editable mesh. Used for static models. Also used for terrain patches
class CEditableSurface : public CBaseEditableObject
{
public:
	CEditableSurface();		// default constructor will tell you to load new mesh
	~CEditableSurface();

	void					Render(int nViewRenderFlags);
	void					RenderGhost(Vector3D &addpos, Vector3D &addscale, Vector3D &addrot, bool use_center = false, Vector3D &center = vec3_zero);

	Matrix4x4				GetRenderWorldTransform() {return identity4();}

	// TODO: Undoable stuff:
	// bool					RecordUndoData(actiondata_t* action);
	// bool					RestoreFromUndoAction(actiondata_t* action);

	// primitive builders
	void					MakePolygon();																	// creates a polygon
	void					MakeTerrainPatch(int wide, int tall, int tesselation = -1);											// creates terrain path (plane)
	void					MakeCustomTerrainPatch(int wide, int tall, eqlevelvertex_t* vertices, int tesselation = -1);			// creates custom terrain path
	void					MakeCustomMesh(eqlevelvertex_t* verts, int* indices, int nVerts, int nIndices);	// creates custom mesh

	//bool					LoadMeshDialog();
	//bool					LoadFromDSM(char* filename);

	// begin/end modification (end unlocks VB)
	void					BeginModify();		// modification notification
	void					EndModify();		// ends modification, flushes VBOs and rebuilds bbox

	
	int						GetVertexCount();	// vertex count
	int						GetIndexCount();	// index count

	eqlevelvertex_t*		GetVertices();		// returns vertices. NOTE: don't modify mesh without BeginModify\EndModify
	int*					GetIndices();		// returns indices. NOTE: don't modify mesh without BeginModify\EndModify

	IVertexBuffer*			GetVertexBuffer();	// returns vertex buffer
	IIndexBuffer*			GetIndexBuffer();	// returns index buffer

	void					FlipFaces();

	Vector3D				GetBoundingBoxMins();	// returns bbox
	Vector3D				GetBoundingBoxMaxs();

	// rendering bbox
	Vector3D				GetBBoxMins();					// min bbox dimensions
	Vector3D				GetBBoxMaxs();					// max bbox dimensions

	void					RebuildBoundingBox();				// warning: slow!

	EditableSurfaceType_e	GetSurfaceType();					// returns surface type

	// basic mesh modifications
	void					Translate(Vector3D &position);		// moves all verts of model
	void					Scale(Vector3D &scale, bool use_center = false, Vector3D &scale_center = vec3_zero);				// scales all verts of model
	void					Rotate(Vector3D &rotation_angles, bool use_center = false, Vector3D &rotation_center = vec3_zero);	// rotates all verts of model

	EditableType_e			GetType() {return m_objtype;}

	// texture data
	void					SetMaterial(IMaterial* pMaterial);		// sets new material
	IMaterial*				GetMaterial();							// returns material

	void					UpdateTextureCoords(Vector3D &vMovement = vec3_zero);

	float					CheckLineIntersection(const Vector3D &start, const Vector3D &end,Vector3D &outPos);

	int						GetInstersectingTriangle(const Vector3D &start, const Vector3D &end);

	void					OnRemove(bool bOnLevelCleanup = false);

	// saves this object
	bool					WriteObject(IVirtualStream*	pStream);

	// read this object
	bool					ReadObject(IVirtualStream*	pStream);

	// stores object in keyvalues
	void					SaveToKeyValues(kvkeybase_t* pSection);

	// stores object in keyvalues
	bool					LoadFromKeyValues(kvkeybase_t* pSection);

	// returns surface textures count
	int						GetSurfaceTextureCount() {return 1;}

	// returns texture
	Texture_t*				GetSurfaceTexture(int id) {return &m_surftex;}

	// updates texturing
	void					UpdateSurfaceTextures() {UpdateTextureCoords();}

	// called when whole level builds
	//void					BuildObject(level_build_data_t* pLevelBuildData);

	// copies this object
	CBaseEditableObject*	CloneObject();

	void					CutSurfaceByPlane(Plane &plane, CEditableSurface** ppSurface);

	// add surface geometry, and use this material
	void					AddSurfaceGeometry(CEditableSurface* pSurface);

protected:

	void					Destroy();

	IVertexBuffer*			m_pVB;			// static vertex buffer with supporting only the data modification
	IIndexBuffer*			m_pIB;			// non-modifiable index buffer

	eqlevelvertex_t*		m_pVertexData;	// modifiable vertex data
	int*					m_pIndexData;

	int						m_numAllVerts;
	int						m_numAllIndices;

	int						m_nModifyTimes;

	BoundingBox				m_bbox;

	EditableSurfaceType_e	m_surftype;
	EditableType_e			m_objtype;

	// only for normal and terrain patch
	Texture_t				m_surftex;

	//Vector3D				m_vecNormal;

	// special info for terrain patch
	int						m_nWide;
	int						m_nTall;

	tesselationdata_t		m_tesselation;
};

#endif // EDITABLEMESH_H