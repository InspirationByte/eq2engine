//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Editable decal class
//////////////////////////////////////////////////////////////////////////////////


#ifndef EDITABLEDECAL_H
#define EDITABLEDECAL_H

#include "BaseEditableObject.h"
#include "materialsystem/IMaterialSystem.h"
#include "eqlevel.h"

// The basic editable mesh. Used for static models. Also used for terrain patches
class CEditableDecal : public CBaseEditableObject
{
public:
	CEditableDecal();		// default constructor will tell you to load new mesh
	~CEditableDecal();

	void					Render(int nViewRenderFlags);
	void					RenderGhost(Vector3D &addpos, Vector3D &addscale, Vector3D &addrot, bool use_center = false, Vector3D &center = vec3_zero);

	Matrix4x4				GetRenderWorldTransform();

	Vector3D				GetBoundingBoxMins();	// returns bbox
	Vector3D				GetBoundingBoxMaxs();

	// rendering bbox
	Vector3D				GetBBoxMins();					// min bbox dimensions
	Vector3D				GetBBoxMaxs();					// max bbox dimensions

	void					RebuildBoundingBox();				// warning: slow!

	// basic mesh modifications
	void					Translate(Vector3D &position);		// moves all verts of model
	void					Scale(Vector3D &scale, bool use_center = false, Vector3D &scale_center = vec3_zero);				// scales all verts of model
	void					Rotate(Vector3D &rotation_angles, bool use_center = false, Vector3D &rotation_center = vec3_zero);	// rotates all verts of model

	EditableType_e			GetType() {return EDITABLE_DECAL;}

	void					UpdateDecalGeom();

	float					CheckLineIntersection(const Vector3D &start, const Vector3D &end,Vector3D &outPos);

	void					OnRemove(bool bOnLevelCleanup);

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
	void					UpdateSurfaceTextures() {BeginModify();UpdateDecalGeom();EndModify();}

	// called when whole level builds
	//void					BuildObject(level_build_data_t* pLevelBuildData);

	// copies this object
	CBaseEditableObject*	CloneObject();

	void					BeginModify();
	void					EndModify();

protected:

	void					ClipDecal(Plane& plane);

	void					Destroy();

	IIndexBuffer*			m_pIB;			// non-modifiable index buffer
	IVertexBuffer*			m_pVB;			// static vertex buffer with supporting only the data modification

	eqlevelvertex_t*		m_pVertexData;	// modifiable vertex data
	int*					m_pIndexData;

	int						m_numAllVerts;
	int						m_numAllIndices;

	Vector3D				m_bbox[2];

	// only for normal and terrain patch
	Texture_t				m_surftex;

	// decal normal
	//Vector3D				m_vecNormal;

	int						m_nModifyTimes;
};

#endif // EDITABLEDECAL_H