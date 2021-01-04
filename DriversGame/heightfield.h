//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Heightfield
//////////////////////////////////////////////////////////////////////////////////

#ifndef HEIGHTFIELD_H
#define HEIGHTFIELD_H

#include "materialsystem/IMaterialSystem.h"
#include "IVirtualStream.h"
#include "occluderSet.h"
#include "TextureAtlas.h"

#include "DrvSynDecals.h"

#include "math/DkMath.h"

#ifdef EDITOR
#include "../DriversEditor/EditorActionHistory.h"
#endif // EDITOR

#define HFIELD_HEIGHT_STEP		(0.1f)
#define HFIELD_POINT_SIZE		(4.0f)
#define HFIELD_MIN_POINTS		64			// means default is 32x32 meters
#define HFIELD_SUBDIVIDE		(16.0f)

#define NEIGHBOR_OFFS_XDX(x, f)	{x-f, x,   x+f, x,   x-f, x+f, x+f, x-f}		// neighbours
#define NEIGHBOR_OFFS_YDY(y, f)	{y,   y-f, y,   y+f, y-f, y-f, y+f, y+f}

enum NEIGHBOR_OFFS8
{
	LEFT,
	BOTTOM,
	RIGHT,
	TOP,
	LEFTBOTTOM,
	RIGHTBOTTOM,
	RIGHTTOP,
	LEFTTOP
};

#define NEIGHBOR_OFFS_SXDX(x, f)	{x, x-f, x, x+f, x, x-f, x+f, x+f, x-f}	// self and neighbours
#define NEIGHBOR_OFFS_SYDY(y, f)	{y, y, y-f, y, y+f, y-f, y-f, y+f, y+f}

#define NEIGHBOR_OFFS_X(x)		{x-1, x, x+1, x}		// non-diagonal
#define NEIGHBOR_OFFS_Y(y)		{y, y-1, y, y+1}

// bottom left, bottom right, top right, top left
#define NEIGHBOR_OFFS_DX(x, f)	{x-f, x+f, x+f, x-f}	// diagonal (used to generate vertices)
#define NEIGHBOR_OFFS_DY(y, f)	{y-f, y-f, y+f, y+f}

#define ROLLING_VALUE(x, max) ((x < 0) ? max+x : ((x >= max) ? x-max : x ))

enum EFieldTileFlag
{
	EHTILE_EMPTY				= (1 << 0),		// don't generate geometry
	EHTILE_ROTATE_QUAD			= (1 << 1),		// rotate quad triangle indices
	EHTILE_DETACHED				= (1 << 2),		// detach and don't change height
	EHTILE_ADDWALL				= (1 << 3),		// adds wall on detached

	EHTILE_COLLIDE_WALL			= (1 << 5),		// don't add collision to wall
	EHTILE_NOCOLLIDE			= (1 << 4),		// don't add collision
	EHTILE_ROTATABLE			= (1 << 6),		// supports rotation
};

struct hfieldtile_s
{
	hfieldtile_s()
	{
		height = 0;
		texture = -1;
		flags = 0;
		rotatetex = 0;
		atlasIdx = 0;
		transition = 0;
	}

	int8	flags;				// EFieldTileFlag
	int8	texture;			// 128 textures per height field
	short	height;				// height of point
	short	rotatetex : 6;		// rotate texture (+1 = 90 degress)
	short	atlasIdx : 8;		// 512 atlas entries
	short	transition : 2;		// texture transition bits, one is reserved
};

ALIGNED_TYPE(hfieldtile_s,4) hfieldtile_t;

struct hfielddrawvertex_s
{
	hfielddrawvertex_s()
	{
		position = vec3_zero;
		normal = vec3_up;
		texcoord = vec2_zero;
		lighting = 1.0f;
	}

	hfielddrawvertex_s(const Vector3D& p, const Vector3D& n, const Vector2D& t )
	{
		position = p;
		normal = n;
		texcoord = t;
		lighting = 1.0f;
	}

	Vector3D		position;
	TVec2D<half>	texcoord;

	TVec3D<half>	normal;
	half			lighting;
	//Matrix3x3	tbn;
};

ALIGNED_TYPE(hfielddrawvertex_s,4) hfielddrawvertex_t;

struct hfieldmaterial_t
{
	hfieldmaterial_t()
	{
#ifdef EDITOR
		used = false;
#endif // EDITOR
	}

	~hfieldmaterial_t()
	{
	}

	IMaterial*		material;

#ifdef EDITOR
	bool			used;
#endif // EDITOR
};

struct hfieldbatch_t
{
	hfieldbatch_t()
	{
		materialBundle = NULL;
		flags = 0;
	}

	DkList<hfielddrawvertex_t>	verts;
	DkList<Vector3D>			physicsVerts;
	DkList<uint32>				indices;

	BoundingBox					bbox;

	int							sx;
	int							sy;

	int							flags;

	hfieldmaterial_t*			materialBundle;
};

class btStridingMeshInterface;
class CEqCollisionObject;

struct hfieldPhysicsData_t
{
	PPMEM_MANAGED_OBJECT_TAG("hfieldPhysData")

	DkList<CEqCollisionObject*>			m_collisionObjects;
	DkList<btStridingMeshInterface*>	m_meshes;
	DkList<hfieldbatch_t*>				m_batches;
};

enum EHFieldGeometryGenerateMode
{
	HFIELD_GEOM_RENDER = 0,
	HFIELD_GEOM_PHYSICS,
	HFIELD_GEOM_DEBUG		// same as render, but texture coordinates are ok
};

class CHeightTileField
{
public:
	PPMEM_MANAGED_OBJECT()

						CHeightTileField();
				virtual ~CHeightTileField();

	void						Init(int size = HFIELD_MIN_POINTS, const IVector2D& regionPos = IVector2D(-1));
	void						Destroy();
	void						UnloadMaterials();

	void						ReadOnlyPoints( IVirtualStream* stream );
	void						ReadOnlyMaterials( IVirtualStream* stream );

	void						ReadFromStream( IVirtualStream* stream );

	int							WriteToStream( IVirtualStream* stream );

	// get/set
	bool						SetPointMaterial(int x, int y, IMaterial* pMaterial, int atlIdx = 0);

	hfieldtile_t*				GetTile(int x, int y) const;										// returns tile for modifying
	hfieldtile_t*				GetTile_CheckFlag(int x, int y, int flag, bool enabled) const;	// tile with flag checking, for modify

	hfieldtile_t*				GetTileAndNeighbourField(int x, int y, CHeightTileField** field) const;	// returns tile for modifying

	void						GetTileTBN(int x, int y, Vector3D& tang, Vector3D& binorm, Vector3D& norm) const; // returns tile normal

	// returns face at position
	bool						PointAtPos(const Vector3D& pos, int& x, int& y) const;

	void						Generate( EHFieldGeometryGenerateMode mode, DkList<hfieldbatch_t*>& batches, float subdivision = HFIELD_SUBDIVIDE );

	bool						IsEmpty();

	void						DebugRender(bool bDrawTiles, float gridHeight, float gridSize);

	void						GetDecalPolygons( decalPrimitives_t& polys, occludingFrustum_t* frustum);

#ifdef EDITOR
	void						FreeUnusedMaterials();
#endif // EDITOR

public:
	Vector3D					m_position;		// translation of heightfield
	int							m_rotation;		// rotation in degrees

	IVector2D					m_regionPos;	// position in 2D array
	int							m_fieldIdx;		// the hfield layer index in region's array

	int							m_levOffset;

	DkList<hfieldmaterial_t*>	m_materials;

	int							m_sizew;
	int							m_sizeh;

	bool						m_hasTransparentSubsets;

	hfieldPhysicsData_t*		m_physData;

protected:

	hfieldtile_t*		m_points;		// the heightfield
};

void UTIL_GetTileIndexes(const IVector2D& tileXY, const IVector2D& fieldWideTall, IVector2D& outTileXY, IVector2D& outFieldOffset);

//---------------------------------------------------------------

struct hfielddrawbatch_t
{
	PPMEM_MANAGED_OBJECT()

	hfielddrawbatch_t()
	{
		startVertex = 0;
		numVerts = 0;
		startIndex = 0;
		numIndices = 0;

		pMaterial = NULL;
	}

	BoundingBox	bbox;

	int			startVertex;
	int			numVerts;
	int			startIndex;
	int			numIndices;

	IMaterial*	pMaterial;
};

#ifdef EDITOR
class CHeightTileFieldRenderable : public CHeightTileField, public CUndoableObject
#else
class CHeightTileFieldRenderable : public CHeightTileField
#endif // EDITOR
{
	friend class CUI_RoadEditor;
	friend class CEditorLevel;

public:
	CHeightTileFieldRenderable();
	~CHeightTileFieldRenderable();

	void					CleanRenderData(bool deleteVBO = true);

	void					GenerateRenderData(bool debug = false);

	void					SetChanged() {m_isChanged = true;}
	bool					IsChanged() const {return m_isChanged;}

	void					Render(int nDrawFlags, const occludingFrustum_t& occlSet);
	void					RenderDebug(ITexture* debugTexture, int nDrawFlags, const occludingFrustum_t& occlSet);

#ifdef EDITOR
protected:
	UndoableFactoryFunc		Undoable_GetFactoryFunc() { return nullptr; }
	void					Undoable_Remove() {}
	bool					Undoable_WriteObjectData( IVirtualStream* stream );	// writing object
	void					Undoable_ReadObjectData( IVirtualStream* stream );	// reading object
#endif // EDITOR

private:

	hfielddrawbatch_t*		m_batches;
	int						m_numBatches;

	IVertexFormat*			m_format;
	IVertexBuffer*			m_vertexbuffer;
	IIndexBuffer*			m_indexbuffer;

	int						m_numVerts;

	bool					m_isChanged;
};

#endif // HEIGHTFIELD_H
