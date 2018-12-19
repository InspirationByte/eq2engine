//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Basic editor objects
//
// TODO: Undo tables, undoable objects
//////////////////////////////////////////////////////////////////////////////////

#ifndef EDITORRENDERABLE_H
#define EDITORRENDERABLE_H

#include "Platform.h"
#include "Math/DkMath.h"
#include "BaseRenderableObject.h"
#include "IVirtualStream.h"
#include "KeyValues.h"
#include "Utils/eqstring.h"

// object data ident, used for clipboard
#define OBJECTDATA_MAGIC 0x0b7ec1d1

enum EditableType_e
{
	EDITABLE_STATIC = 0,	// static objects
	EDITABLE_TERRAINPATCH,	// terrain patches TODO: remove this type
	EDITABLE_BRUSH,			// brushes
	EDITABLE_ENTITY,		// entities
	EDITABLE_GROUP,			// group container
	EDITABLE_DECAL,			// decal

	EDITABLE_TYPE_COUNT	
};

static char* s_editabletypestrings[EDITABLE_TYPE_COUNT] =
{
	"static",
	"terrainpath",
	"brush",
	"entity",
	"group",
	"decal",
};

inline EditableType_e ResolveEditableTypeFromString(char* pStr)
{
	for(int i = 0; i < EDITABLE_TYPE_COUNT; i++)
	{
		if(!stricmp(s_editabletypestrings[i], pStr))
			return (EditableType_e)i;
	}

	return EDITABLE_STATIC;
}

struct ReadWriteSurfaceTexture_t
{
	char		material[256];	// material and texture
	Plane		UAxis;
	Plane		VAxis;
	Plane		Plane;

	float		rotate;
	Vector2D	scale;

	int			nFlags;
	int			nTesseleation;
};

enum EditableFlags_e
{
	EDFL_HIDDEN		= (1 << 0),		// hidden
	EDFL_SELECTED	= (1 << 1),		// selected
	EDFL_GROUPPED	= (1 << 2),		// groupped object, so it discards selection
	EDFL_CLIP		= (1 << 3),		// any kind of clip
	EDFL_AREAPORTAL	= (1 << 4),		// area portal
	EDFL_ROOM		= (1 << 5),		// room

	EDFL_VISIBILITY	= (1 << 6)		// visocclusion
};

class IMaterial;

// surface texture flag
// must be same with compiler
enum SurftexFlags_e
{
	STFL_SELECTED		= (1 << 0),	// selected?, unused for compiler
	STFL_TESSELATED		= (1 << 1), // tesselation
	STFL_NOSUBDIVISION	= (1 << 2),	// non-subdividable
	STFL_NOCOLLIDE		= (1 << 3),	// not collideable
	STFL_CUSTOMTEXCOORD	= (1 << 4),	// custom texture coordinates, EDITABLE_SURFACE only
};

struct Texture_t
{
	Texture_t()
	{
		nFlags = 0;
	}

	// face material
	IMaterial*						pMaterial;

	// texture matrix
	Plane							UAxis;		// tangent plane
	Plane							VAxis;		// binormal plane
	Plane							Plane;		// normal plane

	Vector2D						vScale;		// texture scale

	float							fRotation;	// texture rotation

	// flags
	int								nFlags;

	// smoothing group index
	ubyte							nSmoothingGroup;
};

inline void MakeDefaultFaceTexture(Texture_t* pTexture)
{
	pTexture->fRotation = 0;
	pTexture->nFlags = 0;
	pTexture->nSmoothingGroup = 0;
	pTexture->pMaterial = NULL;
	pTexture->vScale = Vector2D(0.25);
}


extern int s_objectid_increment;
class CBaseEditableObject : public CBaseRenderableObject // ,public CUndoable // this objects are already undoable
{
public:
	PPMEM_MANAGED_OBJECT();

	friend class CEditorLevel;

	CBaseEditableObject();
	CBaseEditableObject(Vector3D &position, Vector3D &angles, Vector3D &scale);

	// renders this object as ghost
	virtual void					RenderGhost(Vector3D &addpos, Vector3D &addscale, Vector3D &addrot, bool use_center = false, Vector3D &center = vec3_zero) = 0;

	// called when object is removes from level
	virtual void					OnRemove(bool bOnLevelCleanup = false);

	// returns object position, entity-only
	virtual Vector3D				GetPosition();

	// returns object angles, entity-only
	virtual Vector3D				GetAngles();

	// returns object scale, entity-only
	virtual Vector3D				GetScale();

	// returns object origin, entity-only
	virtual void					SetPosition(Vector3D &position);

	// returns object angles, entity-only
	virtual void					SetAngles(Vector3D &angles);

	// returns object scale, entity-only
	virtual void					SetScale(Vector3D &scale);

	// translates object
	virtual void					Translate(Vector3D &position) = 0;

	// scales object
	virtual void					Scale(Vector3D &scale, bool use_center = false, Vector3D &scale_center = vec3_zero) = 0;

	// rotates object
	virtual void					Rotate(Vector3D &rotation_angles, bool use_center = false, Vector3D &rotation_center = vec3_zero) = 0;

	// snaps object by grid. You can override it!
	virtual void					SnapObjectByGrid(int grid_size);

	// returns type of editable
	virtual EditableType_e			GetType() = 0;

	// sets group ID
	void							SetGroupID(int id);
	int								GetGroupID();

	// group colors
	void							SetGroupColor(ColorRGB &col);
	ColorRGB						GetGroupColor();

	// returns unique id of object
	int								GetObjectID() {return m_id;}

	// returns layer id
	int								GetLayerID() {return m_layer_id;}

	// min bbox dimensions
	virtual Vector3D				GetBoundingBoxMins() = 0;

	// max bbox dimensions
	virtual Vector3D				GetBoundingBoxMaxs() = 0;

	// adds flags
	void							AddFlags(int flags);

	// removes flags
	void							RemoveFlags(int flags);

	// returns current object's flags
	virtual int						GetFlags();

	// checks object for intersection. useful for Selection
	virtual float					CheckLineIntersection(const Vector3D &start, const Vector3D &end, Vector3D &intersectionPos) = 0;

	// returns surface textures count
	virtual int						GetSurfaceTextureCount() { return 0; }

	// returns texture
	virtual Texture_t*				GetSurfaceTexture(int id) {return NULL;}

	// updates texturing
	virtual void					UpdateSurfaceTextures() {}

	// saves this object
	virtual bool					WriteObject(IVirtualStream*	pStream) = 0;

	// read this object
	virtual bool					ReadObject(IVirtualStream*	pStream) = 0;

	// stores object in keyvalues
	virtual void					SaveToKeyValues(kvkeybase_t* pSection) = 0;

	// stores object in keyvalues
	virtual bool					LoadFromKeyValues(kvkeybase_t* pSection) = 0;

	// called when whole level builds
	//virtual void					BuildObject(level_build_data_t* pLevelBuildData) = 0;

	// copies this object
	virtual CBaseEditableObject*	CloneObject() = 0;

	// begin/end modification
	virtual void					BeginModify() {}		// modification notification
	virtual void					EndModify() {}			// ends modification

	virtual void					SetName(const char* pszName);
	virtual const char*				GetName();

	// retruns 1 or 2 rooms. 0 is outside
	int								GetRoomIndex(int *rooms) {return 0;}

	int								m_id;		// unique object id

	void							SetLayer(int id);

	int								m_layer_id;	// layer id

protected:
	Vector3D						m_position;
	Vector3D						m_angles;
	Vector3D						m_scale;

	EqString						m_szName;

	int								m_groupid;

	int								m_nFlags;

	ColorRGB						m_groupColor;
};

#endif // EDITORRENDERABLE_H