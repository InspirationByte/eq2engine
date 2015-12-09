//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Particle system renderer
//////////////////////////////////////////////////////////////////////////////////

#ifndef PARTICLERENDERER_H
#define PARTICLERENDERER_H

// effects
#include "EffectRender.h"
#include "materialsystem/IMaterialSystem.h"
#include "eqlevel.h"

//-----------------------------------
// Low-level particle rendering
//-----------------------------------

// particle vertex with color
struct ParticleVertex_t
{
	ParticleVertex_t()
	{
	}

	ParticleVertex_t(Vector3D &p, Vector2D &t, ColorRGBA &c)
	{
		point = p;
		texcoord = t;
		color = c;
	}

	Vector3D	point;
	Vector2D	texcoord;
	ColorRGBA	color;

	Vector3D	normal;
};

#define MAX_PARTICLES 8192 // per frame, per group, quad-oriented

#define DVBO_MAX_SIZE (MAX_PARTICLES*sizeof(eqlevelvertex_t)*4)
#define PVBO_MAX_SIZE (MAX_PARTICLES*sizeof(ParticleVertex_t)*4)
#define PIBO_MAX_SIZE (MAX_PARTICLES*(sizeof(uint16)+3))

// Precache particle
int PrecacheParticleMaterial(const char* materialname, bool isPostRender = false, bool isDecal = false);

// Precache particle
int PrecacheAddMaterial(IMaterial* pMaterial, bool isPostRender = false, bool isDecal = false);

//------------------------------------------------------------------------------------
// Effect Group Cache
// Use DECLARE_EFFECT_GROUP macro to declare it
// and inside the {} brackets use PRECACHE_PARTICLE_MATERIAL
//------------------------------------------------------------------------------------
class IEffectCache
{
public:
	virtual void	Precache() = 0;

	int	GetEffectGroupID(int index)
	{
		return effect_list[index];
	}

	int GetCachedCount()
	{
		return effect_list.numElem();
	}

	void Unload()
	{
		effect_list.clear();
	}

protected:
	DkList<int>		effect_list;
};

extern DkList<IEffectCache*> g_pEffectCacheList;

#define DECLARE_EFFECT_GROUP(group_name)					\
class C##group_name##_EffectCache : public IEffectCache		\
{															\
public:														\
	C##group_name##_EffectCache()							\
	{														\
		g_pEffectCacheList.append(this);					\
	}														\
															\
	void Precache();										\
};															\
static C##group_name##_EffectCache s_##group_name##_cache;	\
void C##group_name##_EffectCache::Precache()

#define DECLARE_DYNAMIC_EFFECT_GROUP(group_name) \
	DkList<int> _##group_name##MaterialIndexes

#define UNLOAD_DYNAMIC_EFFECT_GROUP(group_name) \
	_##group_name##MaterialIndexes.clear()

#define EFFECT_DYNAMIC_GROUP_RANDOMMATERIAL_ID(group_name) \
	RandomInt(0, _##group_name##MaterialIndexes.numElem()-1);

#define EFFECT_DYNAMIC_GROUP_ID(group_name, index) \
	_##group_name##MaterialIndexes[index]

#define EFFECT_GROUP_ID(group_name, index) \
	s_##group_name##_cache.GetEffectGroupID(index)

#define EFFECT_GROUP_RANDOMMATERIAL_ID(group_name) \
	RandomInt(0, s_##group_name##_cache.GetCachedCount()-1);

#define EFFECT_GROUP(group_name) \
	s_##group_name##_cache

#define PRECACHE_PARTICLE_MATERIAL(name)	\
{int idx = PrecacheParticleMaterial(name);	\
	effect_list.append(idx);}

#define PRECACHE_PARTICLE_MATERIAL_POST(name)	\
{int idx = PrecacheParticleMaterial(name, true);	\
	effect_list.append(idx);}

#define PRECACHE_DECAL_MATERIAL(name)	\
{int idx = PrecacheParticleMaterial(name, false, true);	\
	effect_list.append(idx);}

IMaterial* GetParticleMaterial(int grpID);

int  GetParticleIndexCount(int groupMaterialIndex);

//------------------------------------------------------------------------------------

bool InitParticleBuffers();
bool ShutdownParticleBuffers();

// clear particle cache
void ClearParticleCache();

// draw particles for G-Buffer
void DrawDecals(ITexture** pDecalGBuffer = NULL);

// draw particles
void DrawParticleMaterialGroups(bool dsCheck = false);

// adds geometry to particle buffers
void AddParticleGeom(ParticleVertex_t *fourVerts, uint16 *fourIndices, int nVertices, int nIndices, int groupMaterialIndex);

// adds particle quad
void AddParticleQuad(ParticleVertex_t fourVerts[4], int groupMaterialIndex);

#ifndef NO_ENGINE
// adds geometry to particle buffers, but as decal
void AddDecalGeom(eqlevelvertex_t *verts, uint16 *indices, int nVertices, int nIndices, int groupMaterialIndex);
#endif // NO_ENGINE

//-----------------------------------
// Effect elementary
//-----------------------------------

enum EffectFlags_e
{
	EFFECT_FLAG_LOCK_X				= (1 << 0),
	EFFECT_FLAG_LOCK_Y				= (1 << 1),
	EFFECT_FLAG_ALWAYS_VISIBLE		= (1 << 2),
	EFFECT_FLAG_NO_FRUSTUM_CHECK	= (1 << 3),
	EFFECT_FLAG_NO_DEPTHTEST		= (1 << 4),
	EFFECT_FLAG_RADIAL_ALIGNING		= (1 << 5),
};

struct PFXBillboard_t
{
	int			nGroupIndex;

	Vector4D	vColor;
	Vector3D	vOrigin;
	Vector3D	vLockDir;

	float		fWide;
	float		fTall;

	float		fZAngle;

	int			nFlags;
};

// draws particle
void Effects_DrawBillboard(PFXBillboard_t* effect, CViewParams* view, Volume* frustum);

#endif // PARTICLERENDERER_H