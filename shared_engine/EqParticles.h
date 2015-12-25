//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium particles renderer
//				A part of particle system
//////////////////////////////////////////////////////////////////////////////////

#ifndef EQPARTICLES_H
#define EQPARTICLES_H

#include "materialsystem/IMaterialSystem.h"
#include "BaseRenderableObject.h"

#include "EffectRender.h"

enum EPartRenderFlags
{
	EPRFLAG_DONT_FLUSHBUFFERS	= (1 << 24),
	EPRFLAG_INVERT_CULL			= (1 << 25),
};

// particle vertex with color
struct PFXVertex_t
{
	PFXVertex_t()
	{
	}

	PFXVertex_t(Vector3D &p, Vector2D &t, ColorRGBA &c)
	{
		point = p;
		texcoord = t;
		color = c;
	}

	Vector3D		point;
	TVec2D<half>	texcoord;
	TVec4D<half>	color;

	//TVec3D<half>	normal;
	//half			unused;
};

//
// Particle renderer
// 
// It's represented as renderable object
//
// You can derive it
#ifndef NO_ENGINE
class CParticleRenderGroup : public CBaseRenderableObject
#else
class CParticleRenderGroup
#endif
{
	friend class CParticleLowLevelRenderer;

public:
	PPMEM_MANAGED_OBJECT();

						CParticleRenderGroup();
			virtual		~CParticleRenderGroup();

	virtual void		Init( const char* pszMaterialName, bool bCreateOwnVBO );
	virtual void		Shutdown();

	// adds geometry to particle buffers
	void				AddParticleGeom(PFXVertex_t* verts, uint16* indices, int nVertices, int nIndices);

	// adds particle quad
	void				AddParticleQuad(PFXVertex_t fourVerts[4]);

	// adds strip to list
	void				AddParticleStrip(PFXVertex_t* verts, int nVertices);

	// allocates a fixed strip for further use.
	// returns vertex start index. Returns -1 if failed
	// terminate with AddStripBreak();
	// this provides less copy operations
	int					AllocateGeom( int nVertices, int nIndices, PFXVertex_t** verts, uint16** indices, bool preSetIndices = false );

	// adds triangle strip break indices
	void				AddStripBreak();

	//-------------------------------------------------------------------

	// min bbox dimensions
	Vector3D			GetBBoxMins() {return Vector3D(MAX_COORD_UNITS);}

	// max bbox dimensions
	Vector3D			GetBBoxMaxs() {return Vector3D(-MAX_COORD_UNITS);}

	// renders this buffer
	void				Render(int nViewRenderFlags);

	void				SetCustomProjectionMatrix(const Matrix4x4& mat);

protected:

	// internal use only
	void				AddIndex(uint16 idx);
	void				AddVertex(const PFXVertex_t& vert);
	void				AddVertices(PFXVertex_t* verts, int nVerts);
	//void				AddVertices2(eqlevelvertex_t* verts, int nVerts);
	void				AddIndices(uint16* indices, int nIndx);

	IMaterial*			m_pMaterial;

	PFXVertex_t*		m_pVerts;
	uint16*				m_pIndices;

	int					m_numVertices;
	int					m_numIndices;

	// uses own VBO? (in case if decals or something rendered)
	bool				m_bHasOwnVBO;

	bool				m_initialized;

	IVertexBuffer*		m_vertexBuffer;
	IIndexBuffer*		m_indexBuffer;
	IVertexFormat*		m_vertexFormat;

	bool				m_useCustomProjMat;
	Matrix4x4			m_customProjMat;
};

//------------------------------------------------------------------------------------

class CPFXAtlasGroup : public CParticleRenderGroup, public CTextureAtlas
{
public:
	CPFXAtlasGroup();

	void				Init( const char* pszMaterialName, bool bCreateOwnVBO );
	void				Shutdown();
};

//------------------------------------------------------------------------------------

class CParticleLowLevelRenderer
{
	friend class CParticleRenderGroup;

public:
	CParticleLowLevelRenderer();

	void							Init();
	void							Shutdown();

	bool							IsInitialized() {return m_initialized;}

	void							PreloadCache();
	void							ClearParticleCache();

	void							AddRenderGroup(CParticleRenderGroup* pRenderGroup);
	void							RemoveRenderGroup(CParticleRenderGroup* pRenderGroup);

	void							PreloadMaterials();

	// prepares render buffers and sends renderables to ViewRenderer
	void							Render(int nRenderFlags);

	bool							MakeVBOFrom(CParticleRenderGroup* pGroup);

	bool							InitBuffers();
	bool							ShutdownBuffers();

protected:

	Threading::CEqMutex&			m_mutex;

	DkList<CParticleRenderGroup*>	m_renderGroups;

	IVertexBuffer*					m_vertexBuffer;
	IIndexBuffer*					m_indexBuffer;
	IVertexFormat*					m_vertexFormat;

	bool							m_initialized;
};

//------------------------------------------------------------------------------------

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
	CPFXAtlasGroup*		group;		// atlas
	TexAtlasEntry_t*	tex;	// texture name in atlas

	Vector4D			vColor;
	Vector3D			vOrigin;
	Vector3D			vLockDir;

	float				fWide;
	float				fTall;

	float				fZAngle;

	int					nFlags;
};

// draws particle
void Effects_DrawBillboard(PFXBillboard_t* effect, CViewParams* view, Volume* frustum);
void Effects_DrawBillboard(PFXBillboard_t* effect, const Matrix4x4& viewMatrix, Volume* frustum);

//------------------------------------------------------------------------------------

extern CParticleLowLevelRenderer*	g_pPFXRenderer;

#endif // EQPARTICLES_H