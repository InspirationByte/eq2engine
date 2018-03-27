//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2016
//////////////////////////////////////////////////////////////////////////////////
// Description: Level object
//////////////////////////////////////////////////////////////////////////////////

#ifndef LEVELOBJECT_H
#define LEVELOBJECT_H

#include "math/Vector.h"
#include "utils/KeyValues.h"
#include "refcounted.h"
#include "ppmem.h"

#include "DrvSynDecals.h"

#ifdef EDITOR
#include "../DriversEditor/EditorPreviewable.h"
#endif // EDITOR

//-----------------------------------------------------------------------------------------

class CEqBulletIndexedMesh;
class CLevelRegion;
class IMaterial;
struct dsmmodel_t;
class IVertexBuffer;
class IIndexBuffer;
class IVertexFormat;

struct lmodeldrawvertex_s
{
	lmodeldrawvertex_s()
	{
		position = vec3_zero;
		normal = vec3_up;
		texcoord = vec2_zero;
	}

	lmodeldrawvertex_s(Vector3D& p, Vector3D& n, Vector2D& t )
	{
		position = p;
		normal = n;
		texcoord = t;
	}

	Vector3D		position;
	Vector2D		texcoord;

	Vector3D		normal;
};

// comparsion operator
inline bool operator == (const lmodeldrawvertex_s &u, const lmodeldrawvertex_s &v)
{
	if(u.position == v.position && u.texcoord == v.texcoord && u.normal == v.normal)
		return true;

	return false;
}

ALIGNED_TYPE(lmodeldrawvertex_s,4) lmodeldrawvertex_t;

struct lmodel_batch_t
{
	PPMEM_MANAGED_OBJECT()

	lmodel_batch_t()
	{
		startVertex = 0;
		numVerts = 0;
		startIndex = 0;
		numIndices = 0;

		pMaterial = NULL;
	}

	uint32		startVertex;
	uint32		numVerts;
	uint32		startIndex;
	uint32		numIndices;

	IMaterial*	pMaterial;
};

//-----------------------------------------------------------------------------------------

class CLevelModel : public RefCountedObject
{
	friend class CGameLevel;
	friend class CLevelRegion;
	friend class CLevObjectDef;
	friend class CLayerModel;
	friend class CBuildingModelGenerator;

	friend class CEditorLevelRegion;
public:
	PPMEM_MANAGED_OBJECT()

							CLevelModel();
	virtual					~CLevelModel();

	void					Ref_DeleteObject() {}

	void					Cleanup();		// cleans up all including render data
	void					ReleaseData();	// releases data but keeps batchs and VBO

	void					GetDecalPolygons( decalprimitives_t& polys, const Matrix4x4& transform );

	void					Render(int nDrawFlags, const BoundingBox& aabb);

	bool					CreateFrom(dsmmodel_t* pModel);
	bool					GenereateRenderData();
	void					PreloadTextures();

	void					Load(IVirtualStream* stream);
	void					Save(IVirtualStream* stream) const;

	const BoundingBox&		GetAABB() const {return m_bbox;}
protected:

	void					CreateCollisionObject( regionObject_t* ref );

	void					GeneratePhysicsData(bool isGround = false);

	CEqBulletIndexedMesh*	m_physicsMesh;

	//-------------------------------------------------------------------

#ifdef EDITOR
	float					Ed_TraceRayDist(const Vector3D& start, const Vector3D& dir);
#endif

	BoundingBox				m_bbox;

	int						m_level;	// editor parameter - model layer location (0 - ground, 1 - sand, 2 - trees, 3 - others)

	lmodel_batch_t*			m_batches;
	lmodel_batch_t*			m_phybatches;
	int						m_numBatches;

	lmodeldrawvertex_t*		m_verts;		// this is NULL after GeneratePhysicsData
	Vector3D*				m_physVerts;	// this is used after GeneratePhysicsData
	uint16*					m_indices;

	bool					m_hasTransparentSubsets;

	int						m_numVerts;
	int						m_numIndices;

	IVertexBuffer*			m_vertexBuffer;
	IIndexBuffer*			m_indexBuffer;
	IVertexFormat*			m_format;
};

//-----------------------------------------------------------------------------------------
// Level region data
//-----------------------------------------------------------------------------------------

class IEqModel;

struct regObjectInstance_t
{
	Quaternion		rotation;
	Vector4D		position;

	//int			lightIndex[16];		// light indexes to fetch from texture
};

struct levObjInstanceData_t
{
	levObjInstanceData_t()
	{
		numInstances = 0;
	}

	regObjectInstance_t		instances[MAX_MODEL_INSTANCES];
	int						numInstances;
};

// only omni lights
struct wlight_t
{
	Vector4D	position;	// position + radius
	ColorRGBA	color;		// color + intensity
};

struct wglow_t
{
	int			type;
	Vector4D	position;
	ColorRGBA	color;
};

struct wlightdata_t
{
	DkList<wlight_t>	m_lights;
	DkList<wglow_t>		m_glows;
};

void LoadDefLightData( wlightdata_t& out, kvkeybase_t* sec );
bool DrawDefLightData( Matrix4x4& objDefMatrix, const wlightdata_t& data, float brightness );

// model container, for editor
#ifdef EDITOR
class CLevObjectDef : public CEditorPreviewable
#else
class CLevObjectDef
#endif // EDITOR
{
public:
	PPMEM_MANAGED_OBJECT()

	CLevObjectDef();
	~CLevObjectDef();

#ifdef EDITOR
	void					RefreshPreview();
#endif // EDITOR
	void					Render( float lodDistance, const BoundingBox& bbox, bool preloadMaterials = false, int nRenderFlags = 0);

	EqString				m_name;
	levObjectDefInfo_t		m_info;

	//-----------------------------

	CLevelModel*			m_model;		// static model

	//-----------------------------

	IEqModel*				m_defModel;
	kvkeybase_t*			m_defKeyvalues;
	levObjInstanceData_t*	m_instData;

	EqString				m_defType;
	wlightdata_t			m_lightData;
};


#endif // LEVELOBJECT_H