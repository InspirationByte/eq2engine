//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Base compiler object
//////////////////////////////////////////////////////////////////////////////////

#ifndef COMPILERBASEOBJECT_H
#define COMPILERBASEOBJECT_H

#include "Utils/eqstring.h"
#include "KeyValues.h"
#include "ppmem.h"

enum CompilerObjectType_e
{
	COMPOBJ_STATIC = 0,		// static objects
	COMPOBJ_BRUSH,			// brushes
	COMPOBJ_ENTITY,			// entities
	COMPOBJ_GROUP,			// group container
	COMPOBJ_DECAL,			// decal

	COMPOBJ_TYPE_COUNT,
};

static char* s_objecttypestrings[COMPOBJ_TYPE_COUNT] =
{
	"static",
	"brush",
	"entity",
	"group",
	"decal",
};

inline CompilerObjectType_e ResolveObjectTypeFromString(char* pStr)
{
	for(int i = 0; i < COMPOBJ_TYPE_COUNT; i++)
	{
		if(!stricmp(s_objecttypestrings[i], pStr))
			return (CompilerObjectType_e)i;
	}

	return COMPOBJ_STATIC;
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

// surface texture flag
// must be same with editor
enum SurftexFlags_e
{
	STFL_SELECTED		= (1 << 0),	// selected?, unused for compiler
	STFL_TESSELATED		= (1 << 1), // tesselation
	STFL_NOSUBDIVISION	= (1 << 2),	// non-subdividable
	STFL_NOCOLLIDE		= (1 << 3),	// not collideable
	STFL_CUSTOMTEXCOORD	= (1 << 4),	// custom texture coordinates, EDITABLE_SURFACE only
};

// surface flags
enum CompilerSurfaceFlags_e
{
	CS_FLAG_NODRAW			= (1 << 0),
	CS_FLAG_NOCOLLIDE		= (1 << 1),

	CS_FLAG_OCCLUDER		= (1 << 2),		// occlusion surface
	CS_FLAG_NOSUBDIVISION	= (1 << 3),		// non-subdividable

	//CS_FLAG_SKIPNAVIGATION	= (1 << 4),		// physics surface flag for skipping navigation generation.

	CS_FLAG_NOFACE			= (CS_FLAG_NOCOLLIDE | CS_FLAG_NODRAW),	// removes face

	CS_FLAG_USEDINSECTOR	= (1 << 5),								// surface put to the sector
	CS_FLAG_SUBDIVIDE		= (1 << 6),	// surface must be subdivided
	CS_FLAG_DONTSUBDIVIDE	= (1 << 7),

	CS_FLAG_MULTIMATERIAL	= (1 << 8),	// texture transition
	CS_FLAG_NOLIGHTMAP		= (1 << 9),
};

// base compiler object
PPCLASS( CCompilerBaseObject )
{
public:
	CCompilerBaseObject();

	virtual bool					LoadFromKeyValues(kvkeybase_t* pBase) = 0;

	virtual void					SetName(const char* pszName);
	virtual const char*				GetName();

	// returns type of editable
	virtual CompilerObjectType_e	GetType() = 0;

	// returns unique id of object
	int								GetObjectID() {return m_nId;}

	// returns layer id
	int								GetLayerID() {return m_nLayerID;}

	// min bbox dimensions
	virtual Vector3D				GetBoundingBoxMins() = 0;

	// max bbox dimensions
	virtual Vector3D				GetBoundingBoxMaxs() = 0;

protected:
	int			m_nFlags;

	EqString	m_szName;
	int			m_nLayerID;
	int			m_nId;
};

#endif // COMPILERBASEOBJECT_H