//-=-=-=-=-=-=-=-=-=-= Copyright (C) Damage Byte L.L.C 2009-2012 =-=-=-=-=-=-=-=-=-=-=-
//
// Description: Equilibrium level file format group
//				All main headers of level definitions is here
//
//				NOTE: changing any structure or enums needs recompilation of all worlds
//
// Engine formats:	WORLD	- sectors and entity data
//					WGEOM	- level geometry file
//					WPHYS	- world physics data
//
//-=-=-=-D-=-A-=-M-=-A-=-G-=-E-=-=-B-=-Y-=-T-=-E-=-=-S-=-T-=-U-=-D-=-I-=-O-=-S-=-=-=-=-

#ifndef EQLEVEL_H
#define EQLEVEL_H

#include "Platform.h"
#include "coord.h" // MAX_COORD_UNITS
#include "math/DkMath.h"

#define EQWF_IDENT			MCHAR4('E','Q','W','F') // Equilibrium World File
#define EQWF_VERSION		7

#define EQW_SECTOR_DEFAULT_SIZE		(1024)

// max coord by minimal sector size
#define EQW_MAX_SECTORS				(MAX_COORD_UNITS / 512)

// a value that depends on stack buffer size
#define MAX_LINKED_PORTALS			32

// Sector data format lumps
enum EQWORLD_LUMPS
{
	// geometry file
	EQWLUMP_SURFACES = 0,
	EQWLUMP_VERTICES,
	EQWLUMP_INDICES,
	EQWLUMP_MATERIALS,
	EQWLUMP_ENTITIES,				// default entities for the sector

	// physics file
	EQWLUMP_PHYS_SURFACES,
	EQWLUMP_PHYS_VERTICES,
	EQWLUMP_PHYS_INDICES,

	// occlusion file
	EQWLUMP_OCCLUSION_SURFACES,
	EQWLUMP_OCCLUSION_VERTS,
	EQWLUMP_OCCLUSION_INDICES,

	// world file
	EQWLUMP_ROOMS,
	EQWLUMP_ROOMVOLUMES,
	EQWLUMP_AREAPORTALS,
	EQWLUMP_AREAPORTALVERTS,
	EQWLUMP_PLANES,

	EQWLUMP_LIGHTMAPINFO,			// lightmap information
	EQWLUMP_LIGHTS,					// lightmap light data

	// detail file
	EQWLUMP_DETAIL_SURFACES,		// decals, grass are details
	EQWLUMP_DETAIL_VERTICES,
	EQWLUMP_DETAIL_INDICES,

	// decal file
	EQWLUMP_WATER_PLANES,
	EQWLUMP_WATER_INFOS,
	EQWLUMP_WATER_VOLUMES,

	EQWLUMP_COUNT,
};

// surface flags
enum EQWORLD_SURFACE_FLAGS
{
	EQSURF_FLAG_NOLIGHTMAP		= (1 << 0),	// surface has no lightmap texture
	EQSURF_FLAG_USED_ASMODEL	= (1 << 1),	// this surface used as model
	EQSURF_FLAG_TRANSLUCENT		= (1 << 2),	// surface is translucent and must be sorted in the engine
	EQSURF_FLAG_DETAIL			= (1 << 3),	// surface is a detail surface
	EQSURF_FLAG_SKY				= (1 << 4), // surface is sky
	EQSURF_FLAG_BLOCKLIGHT		= (1 << 5),	// this surface uses as shadow caster, but it's not visible to normal views
	EQSURF_FLAG_WATER			= (1 << 6),	// this surface is a water
};

//-----------------------------------------------------------
// data lump
//-----------------------------------------------------------
struct eqworldlump_s
{
	int		data_type;
	int		data_size;
};

ALIGNED_TYPE(eqworldlump_s, 1) eqworldlump_t;

//-----------------------------------------------------------
// data file format
//-----------------------------------------------------------
struct eqworldhdr_s
{
	int				ident;
	int				version;

	int				num_lumps;
};

ALIGNED_TYPE(eqworldhdr_s, 1) eqworldhdr_t;

//-----------------------------------------------------------
// room and volumes
//-----------------------------------------------------------

// room (EQWLUMP_ROOMS)
struct eqroom_s
{
	Vector3D mins;
	Vector3D maxs;

	int portals[MAX_LINKED_PORTALS];
	int numPortals;

	int firstVolume;
	int numVolumes;
};

ALIGNED_TYPE(eqroom_s, 1) eqroom_t;

// room volume (EQWLUMP_ROOMVOLUMES)
struct eqroomvolume_s
{
	Vector3D mins;
	Vector3D maxs;

	int firstPlane;	// pointed to EQWLUMP_PLANES
	int numPlanes;

	int firstsurface;
	int numSurfaces;
};

ALIGNED_TYPE(eqroomvolume_s, 1) eqroomvolume_t;

//-----------------------------------------------------------
// portal
//-----------------------------------------------------------
struct eqareaportal_s
{
	Vector3D mins;
	Vector3D maxs;

	int rooms[2];			// rooms linked to this portal
	int portalPlanes[2];	// portal plane ids (pointed to EQWLUMP_PLANES)

	int firstVertex[2];		// vertex starts (pointed to EQWLUMP_AREAPORTALVERTS)
	int numVerts[2];		// vertex count for two planes
};

ALIGNED_TYPE(eqareaportal_s, 1) eqareaportal_t;

struct eqlevelvertexlm_s;

//-----------------------------------------------------------
// vertex for VBO
// NOTE: used only by editor
//-----------------------------------------------------------
struct eqlevelvertex_s
{
	eqlevelvertex_s() {}
	eqlevelvertex_s(Vector3D &pos, Vector2D &tc, Vector3D &nrm)
	{
		position = pos;
		texcoord = tc;

		tangent = binormal = normal = nrm;

		vertexPaint = ColorRGBA(1,0,0,0);
	}

	eqlevelvertex_s(eqlevelvertexlm_s &vtx);

	Vector3D	position;
	Vector2D	texcoord; // TODO: lightmap coords in Vector4D

	Vector3D	tangent;
	Vector3D	binormal;
	Vector3D	normal;

	ColorRGBA	vertexPaint;
};

typedef eqlevelvertex_s eqlevelvertex_t;

// ready for rendering - level vertex structure
struct eqlevelvertexlm_s
{
	eqlevelvertexlm_s() {}
	eqlevelvertexlm_s(Vector3D &pos, Vector2D &tc, Vector3D &nrm)
	{
		position = pos;
		texcoord = Vector4D(tc,0,0);

		tangent = binormal = normal = nrm;

		vertexPaint = ColorRGBA(1,0,0,0);
	}

	eqlevelvertexlm_s(eqlevelvertex_t &vtx)
	{
		position = vtx.position;
		texcoord = Vector4D(vtx.texcoord,0,0);

		tangent = vtx.tangent;
		binormal = vtx.binormal;
		normal = vtx.normal;

		vertexPaint = vtx.vertexPaint;
	}

	Vector3D	position;
	Vector4D	texcoord;

	Vector3D	tangent;
	Vector3D	binormal;
	Vector3D	normal;

	ColorRGBA	vertexPaint;
};

typedef eqlevelvertexlm_s eqlevelvertexlm_t;

// converts normalized vector3D to ubyte
inline void NormalizedVectorToUbyteVector(Vector3D& n, ubyte* dst)
{
	dst[0] = (n.x + 1.0) * 255;
	dst[1] = (n.y + 1.0) * 255;
	dst[2] = (n.z + 1.0) * 255;
}

// ready for rendering - level vertex structure (lightmap + color)
struct eqlevelvertexlit_s
{
	eqlevelvertexlit_s() {}
	eqlevelvertexlit_s(Vector3D &pos, Vector2D &tx, Vector3D &nrm)
	{
		position = pos;
		texcoord = Vector4D(tx,0,0);

		NormalizedVectorToUbyteVector(nrm, tc);
		NormalizedVectorToUbyteVector(nrm, bc);
		NormalizedVectorToUbyteVector(nrm, nc);

		tc[3] = 0;
		bc[3] = 0;
		nc[3] = 0;

		vertexPaint = ColorRGBA(1,0,0,0);
	}

	eqlevelvertexlit_s(eqlevelvertex_t &vtx)
	{
		position = vtx.position;
		texcoord = Vector4D(vtx.texcoord,0,0);

		NormalizedVectorToUbyteVector(vtx.tangent, tc);
		NormalizedVectorToUbyteVector(vtx.binormal, bc);
		NormalizedVectorToUbyteVector(vtx.normal, nc);

		tc[3] = 0;
		bc[3] = 0;
		nc[3] = 0;

		vertexPaint = vtx.vertexPaint;
	}

	Vector3D	position;
	Vector4D	texcoord;

	ubyte		tc[4];
	ubyte		bc[4];
	ubyte		nc[4];

	ColorRGBA	vertexPaint;
};

typedef eqlevelvertexlit_s eqlevelvertexlit_t;

// specially
inline eqlevelvertex_s::eqlevelvertex_s(eqlevelvertexlm_t &vtx)
{
	position = vtx.position;
	texcoord = vtx.texcoord.xy();

	tangent = vtx.tangent;
	binormal = vtx.binormal;
	normal = vtx.normal;

	vertexPaint = vtx.vertexPaint;
}

// ready for rendering - detail vertex structure
struct eqdetailvertex_s
{
	eqdetailvertex_s() {}
	eqdetailvertex_s(Vector3D &pos, Vector2D &tc, Vector3D &nrm)
	{
		position = Vector4D(pos,0);
		texcoord = Vector4D(tc,0,0);

		normal = nrm;

		vertexPaint = ColorRGBA(1,0,0,0);
	}

	eqdetailvertex_s(eqlevelvertex_t &vtx)
	{
		position = Vector4D(vtx.position,0);
		texcoord = Vector4D(vtx.texcoord,0,0);

		//tangent = vtx.tangent;
		//binormal = vtx.binormal;
		normal = vtx.normal;

		vertexPaint = vtx.vertexPaint;
	}

	Vector4D	position;
	Vector4D	texcoord; // TODO: lightmap coords in Vector4D

	Vector3D	normal;

	ColorRGBA	vertexPaint;
};

ALIGNED_TYPE(eqdetailvertex_s, 1) eqdetailvertex_t;

inline eqlevelvertex_t InterpolateLevelVertex(eqlevelvertex_t &u, eqlevelvertex_t &v, float fac)
{
	eqlevelvertex_t out;
	out.position = lerp(u.position, v.position, fac);
	out.texcoord = lerp(u.texcoord, v.texcoord, fac);
	out.tangent = lerp(u.tangent, v.tangent, fac);
	out.binormal = lerp(u.binormal, v.binormal, fac);
	out.normal = lerp(u.normal, v.normal, fac);
	out.vertexPaint = lerp(u.vertexPaint, v.vertexPaint, fac);

	return out;
}

inline eqlevelvertexlm_t InterpolateLevelVertex(eqlevelvertexlm_t &u, eqlevelvertexlm_t &v, float fac)
{
	eqlevelvertexlm_t out;
	out.position = lerp(u.position, v.position, fac);
	out.texcoord = lerp(u.texcoord, v.texcoord, fac);
	out.tangent = lerp(u.tangent, v.tangent, fac);
	out.binormal = lerp(u.binormal, v.binormal, fac);
	out.normal = lerp(u.normal, v.normal, fac);
	out.vertexPaint = lerp(u.vertexPaint, v.vertexPaint, fac);

	return out;
}

// comparsion operator
inline bool operator == (const eqlevelvertex_t &u, const eqlevelvertex_t &v)
{
	if(u.position == v.position && u.texcoord == v.texcoord && u.normal == v.normal)
		return true;

	return false;
}

// comparison operator
inline bool operator == (const eqlevelvertexlm_t &u, const eqlevelvertexlm_t &v)
{
	if(u.position == v.position && u.texcoord == v.texcoord && u.normal == v.normal)
		return true;

	return false;
}

// ready for physics - level vertex structure
struct eqphysicsvertex_s
{
	eqphysicsvertex_s() {}
	eqphysicsvertex_s(eqlevelvertex_s &vert)
	{
		position = vert.position;
	}

	Vector3D	position;
};

ALIGNED_TYPE(eqphysicsvertex_s, 1) eqphysicsvertex_t;

// comparsion operator
inline bool operator == (const eqphysicsvertex_s &u, const eqphysicsvertex_s &v)
{
	if(u.position == v.position)
		return true;

	return false;
}

//-----------------------------------------------------------
// geometry.build file surface format
//-----------------------------------------------------------
struct eqlevelsurf_s
{
	int			firstvertex;
	int			numvertices;

	int			firstindex;
	int			numindices;

	int			material_id;
	int			lightmap_id;

	Vector3D	mins;
	Vector3D	maxs;

	int			flags;				// game surface flags
};

ALIGNED_TYPE(eqlevelsurf_s, 1) eqlevelsurf_t;

//-----------------------------------------------------------
// physics surface format
//-----------------------------------------------------------
struct eqlevelphyssurf_s
{
	int			firstvertex;
	int			numvertices;

	int			firstindex;
	int			numindices;

	int			material_id;

	char		surfaceprops[64];
};

ALIGNED_TYPE(eqlevelphyssurf_s, 1) eqlevelphyssurf_t;

//-----------------------------------------------------------
// material info
//-----------------------------------------------------------
struct eqlevelmaterial_s
{
	char material_path[256];		// material path
};

ALIGNED_TYPE(eqlevelmaterial_s, 1) eqlevelmaterial_t;

//-----------------------------------------------------------
// lightmap info
//-----------------------------------------------------------
struct eqlevellightmapinfo_s
{
	int		numlightmaps;
	int		lightmapsize;
};

ALIGNED_TYPE(eqlevellightmapinfo_s, 1) eqlevellightmapinfo_t;

#define EQLEVEL_LIGHT_LMLIGHT	0x1
#define EQLEVEL_LIGHT_DLIGHT	0x2
#define EQLEVEL_LIGHT_NOSHADOWS	0x4	// shadows disabled
#define EQLEVEL_LIGHT_NOTRACE	0x8	// tracing in engine disabled

//-----------------------------------------------------------
// lightmap light format
//-----------------------------------------------------------
struct eqlevellight_s
{
	uint		type;				// DLT_* type

	ubyte		flags;				// 0x1 - lightmap light, 0x2 - dynamic, 0x4 - no shadows

	Vector3D	origin;				// position or direction for DLT_SUN type
	float		radius;				// light radius
	ColorRGB	color;				// color

	Matrix4x4	worldmatrix;		// spot matrix

	float		fov;				// light field of view
};

ALIGNED_TYPE(eqlevellight_s, 1) eqlevellight_t;

//-----------------------------------------------------------
// occlusion surface format
//-----------------------------------------------------------
struct eqocclusionsurf_s
{
	int			firstvertex;
	int			numvertices;

	int			firstindex;
	int			numindices;

	Vector3D	mins;
	Vector3D	maxs;
};

ALIGNED_TYPE(eqocclusionsurf_s, 1) eqocclusionsurf_t;

//-----------------------------------------------------------
// layer data
//-----------------------------------------------------------
struct eqworldlayer_s
{
	char	layer_name[128];

	bool	initial_active;

	int		numLayerSurfs;
	int		numLayerEnts;
	int		numLayerPhysSurfs;
};

ALIGNED_TYPE(eqworldlayer_s, 1) eqworldlayer_t;

//-----------------------------------------------------------
// warer info
//-----------------------------------------------------------

enum EqWaterFlags_e
{
	EQWATER_FLAG_CHEAP_REFLECTIONS = (1 << 0),
};

struct eqwaterinfo_s
{
	Vector3D	mins;
	Vector3D	maxs;

	// the volumes exactly same as eqroomvolume_t
	int			firstVolume;
	int			numVolumes;

	// the water height stored here for reflections
	float		waterHeight;
	int			waterMaterialId;
	int			nFlags;		//EqWaterFlags_e
};

ALIGNED_TYPE(eqwaterinfo_s, 1) eqwaterinfo_t;

#endif // EQLEVEL_H
