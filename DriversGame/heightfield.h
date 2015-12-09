//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Heightfield
//////////////////////////////////////////////////////////////////////////////////

#ifndef HEIGHTFIELD_H
#define HEIGHTFIELD_H

#include "materialsystem/IMaterialSystem.h"
#include "IVirtualStream.h"
#include "occluderSet.h"
#include "TextureAtlas.h"

#include "math/DkMath.h"

#ifdef EDITOR
#include "../DriversEditor/EditorActionHistory.h"
#endif // EDITOR

#define HFIELD_HEIGHT_STEP		(0.1f)
#define HFIELD_POINT_SIZE		(4.0f)
#define HFIELD_MIN_POINTS		64			// means default is 32x32 meters

#define NEIGHBOR_OFFS_XDX(x, f)	{x-f, x, x+f, x, x-f, x+f, x+f, x-f}		// neighbours
#define NEIGHBOR_OFFS_YDY(y, f)	{y, y-f, y, y+f, y-f, y-f, y+f, y+f}

#define NEIGHBOR_OFFS_SXDX(x, f)	{x, x-f, x, x+f, x, x-f, x+f, x+f, x-f}	// self and neighbours
#define NEIGHBOR_OFFS_SYDY(y, f)	{y, y, y-f, y, y+f, y-f, y-f, y+f, y+f}

#define NEIGHBOR_OFFS_X(x)		{x-1, x, x+1, x}		// non-diagonal
#define NEIGHBOR_OFFS_Y(y)		{y, y-1, y, y+1}

#define NEIGHBOR_OFFS_DX(x, f)	{x-f, x+f, x+f, x-f}	// diagonal
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
	PPMEM_MANAGED_OBJECT()

	hfieldtile_s()
	{
		height = 0;
		texture = -1;
		flags = 0;
		rotatetex = 0;
		atlasIdx = 0;
		transition = 0;
	}

	int8	flags;				// EFieldPointFlag
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
		border = 1.0f;
	}

	hfielddrawvertex_s(const Vector3D& p, const Vector3D& n, const Vector2D& t )
	{
		position = p;
		normal = n;
		texcoord = t;
		border = 1.0f;
	}

	Vector3D		position;
	TVec2D<half>	texcoord;

	TVec3D<half>	normal;
	half			border;
	//Matrix3x3	tbn;
};

ALIGNED_TYPE(hfielddrawvertex_s,4) hfielddrawvertex_t;

struct hfieldmaterial_t
{
	hfieldmaterial_t()
	{
		atlas = NULL;
	}

	~hfieldmaterial_t()
	{
		delete atlas;
		atlas = NULL;
	}

	CTextureAtlas*	atlas;
	IMaterial*		material;
};

struct hfieldbatch_t
{
	PPMEM_MANAGED_OBJECT()

	hfieldbatch_t()
	{
		materialBundle = NULL;
		flags = 0;
	}

	DkList<hfielddrawvertex_t>	verts;
	DkList<uint32>				indices;

	int							sx;
	int							sy;

	int							flags;

	hfieldmaterial_t*			materialBundle;
};

class CHeightTileField
{
public:
	PPMEM_MANAGED_OBJECT()

						CHeightTileField();
				virtual ~CHeightTileField();

	void						Init(int size = HFIELD_MIN_POINTS, int pos_x = -1, int pos_y = -1);
	void						Destroy();


	void						ReadFromStream( IVirtualStream* stream );
	int							WriteToStream( IVirtualStream* stream );

	void						Optimize();

	// get/set
	bool						SetPointMaterial(int x, int y, IMaterial* pMaterial, int atlIdx = 0);

	hfieldtile_t*				GetTile(int x, int y) const;										// returns tile for modifying
	hfieldtile_t*				GetTile_CheckFlag(int x, int y, int flag, bool enabled) const;	// tile with flag checking, for modify

	hfieldtile_t*				GetTileAndNeighbourField(int x, int y, CHeightTileField** field) const;	// returns tile for modifying

	// returns face at position
	bool						PointAtPos(const Vector3D& pos, int& x, int& y) const;

	void						Generate( bool render, DkList<hfieldbatch_t*>& batches );

	bool						IsEmpty();

	void						DebugRender(bool bDrawTiles);

public:
	Vector3D					m_position;		// translation of heightfield
	int							m_rotation;		// rotation in degrees

	int							m_posidx_x;		// position x in 2D array
	int							m_posidx_y;		// position x in 2D array

	int							m_fieldIdx;

	void*						m_userData;

	DkList<hfieldmaterial_t*>	m_materials;

	int							m_sizew;
	int							m_sizeh;

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

public:
	CHeightTileFieldRenderable();
	~CHeightTileFieldRenderable();

	void					CleanRenderData(bool deleteVBO = true);

	void					GenereateRenderData();

	void					SetChanged() {m_isChanged = true;}
	bool					IsChanged() const {return m_isChanged;}

	void					Render(int nDrawFlags, const occludingFrustum_t& occlSet);

#ifdef EDITOR
protected:
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
