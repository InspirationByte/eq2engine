//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium particles renderer
//				A part of particle system
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "SpriteBuilder.h"

class IMaterial;
class IVertexBuffer;
class IIndexBuffer;
class IVertexFormat;
class CViewParams;
class Volume;
struct TexAtlasEntry_t;

using IMaterialPtr = CRefPtr<IMaterial>;

enum EPartRenderFlags
{
	EPRFLAG_DONT_FLUSHBUFFERS	= (1 << 24),
	EPRFLAG_INVERT_CULL			= (1 << 25),
};

// particle vertex with color
struct PFXVertex_t
{
	PFXVertex_t() = default;
	PFXVertex_t(const Vector3D &p, const Vector2D &t, const ColorRGBA &c)
	{
		point = p;
		texcoord = t;
		color = MColor(c).pack();
	}

	Vector3D		point;
	TVec2D<half>	texcoord;
	uint			color;
};

//
// Particle renderer
//
class CParticleRenderGroup : public CSpriteBuilder<PFXVertex_t>
{
	friend class CParticleLowLevelRenderer;

public:
	CParticleRenderGroup();
	virtual		~CParticleRenderGroup();

	virtual void		Init( const char* pszMaterialName, bool bCreateOwnVBO = false, int maxQuads = 16384 );
	virtual void		Shutdown();

	//-------------------------------------------------------------------

	// renders this buffer
	void				Render(int nViewRenderFlags);

	void				SetCustomProjectionMatrix(const Matrix4x4& mat);

	// allocates a fixed strip for further use.
	// returns vertex start index. Returns -1 if failed
	// terminate with AddStripBreak();
	// this provides less copy operations
	int					AllocateGeom( int nVertices, int nIndices, PFXVertex_t** verts, uint16** indices, bool preSetIndices = false );

	void				AddParticleStrip(PFXVertex_t* verts, int nVertices);

	void				SetCullInverted(bool invert) {m_invertCull = invert;}

	IMaterial*			GetMaterial() const {return m_pMaterial;}
protected:

	IMaterialPtr		m_pMaterial;

	bool				m_useCustomProjMat;
	Matrix4x4			m_customProjMat;

	bool				m_invertCull;
};

//------------------------------------------------------------------------------------

class CPFXAtlasGroup : public CParticleRenderGroup
{
public:
	CPFXAtlasGroup();

	void				Init( const char* pszMaterialName, bool bCreateOwnVBO, int maxQuads = 16384 );
	void				Shutdown();

	TexAtlasEntry_t*	GetEntry(int idx);
	int					GetEntryIndex(TexAtlasEntry_t* entry) const;

	TexAtlasEntry_t*	FindEntry(const char* pszName) const;
	int					FindEntryIndex(const char* pszName) const;

	int					GetEntryCount() const;
};

//------------------------------------------------------------------------------------

class CParticleLowLevelRenderer
{
	friend class CParticleRenderGroup;
	friend class CShadowDecalRenderer;

public:
	CParticleLowLevelRenderer();

	void							Init();
	void							Shutdown();

	bool							IsInitialized() {return m_initialized;}

	void							PreloadCache();
	void							ClearParticleCache();

	void							AddRenderGroup(CParticleRenderGroup* pRenderGroup, CParticleRenderGroup* after = nullptr);
	void							RemoveRenderGroup(CParticleRenderGroup* pRenderGroup);

	void							PreloadMaterials();

	// prepares render buffers and sends renderables to ViewRenderer
	void							Render(int nRenderFlags);
	void							ClearBuffers();

	// returns VBO index
	bool							MakeVBOFrom(CSpriteBuilder<PFXVertex_t>* pGroup);

	bool							InitBuffers();
	bool							ShutdownBuffers();

protected:

	Array<CParticleRenderGroup*>	m_renderGroups{ PP_SL };

	IVertexBuffer*					m_vertexBuffer;
	IIndexBuffer*					m_indexBuffer;
	IVertexFormat*					m_vertexFormat;

	int								m_vbMaxQuads;

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
	CPFXAtlasGroup*		group { nullptr };		// atlas
	TexAtlasEntry_t*	tex {nullptr};			// texture name in atlas

	MColor				vColor;
	Vector3D			vOrigin;
	Vector3D			vLockDir;

	float				fWide { 1.0f };
	float				fTall { 1.0f };

	float				fZAngle { 1.0f };

	int					nFlags { 0 };
};

// draws particle
void Effects_DrawBillboard(PFXBillboard_t* effect, CViewParams* view, Volume* frustum);
void Effects_DrawBillboard(PFXBillboard_t* effect, const Matrix4x4& viewMatrix, Volume* frustum);

//------------------------------------------------------------------------------------

extern CParticleLowLevelRenderer*	g_pPFXRenderer;

