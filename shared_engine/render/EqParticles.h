//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium particles renderer
//				A part of particle system
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "SpriteBuilder.h"


class IVertexBuffer;
class IIndexBuffer;
class IVertexFormat;
class CViewParams;
class Volume;
class IGPURenderPassRecorder;
struct AtlasEntry;
struct VertexLayoutDesc;

class IGPUBuffer;
using IGPUBufferPtr = CRefPtr<IGPUBuffer>;

class IMaterial;
using IMaterialPtr = CRefPtr<IMaterial>;

enum EPartRenderFlags
{
	EPRFLAG_DONT_FLUSHBUFFERS	= (1 << 24),
};

// particle vertex with color
struct PFXVertex
{
	static const VertexLayoutDesc& GetVertexLayoutDesc();

	PFXVertex() = default;
	PFXVertex(const Vector3D &p, const Vector2D &t, const ColorRGBA &c)
	{
		point = p;
		texcoord = t;
		color = MColor(c).pack();
	}

	Vector3D		point;
	TVec2D<half>	texcoord;
	uint			color{ 0xffffffff };
};

//
// Particle batch for creating primitives
//
class CParticleBatch : public CSpriteBuilder<PFXVertex>
{
	friend class CParticleLowLevelRenderer;

public:
	virtual	~CParticleBatch();

	// renders this buffer
	void				Render(int nViewRenderFlags, IGPURenderPassRecorder* rendPassRecorder);

	// allocates a fixed strip for further use.
	// returns vertex start index. Returns -1 if failed
	int					AllocateGeom( int nVertices, int nIndices, PFXVertex** verts, uint16** indices, bool preSetIndices = false );
	void				AddParticleStrip(PFXVertex* verts, int nVertices);

	const IMaterialPtr&	GetMaterial() const				{ return m_material; }

	AtlasEntry*			GetEntry(int idx) const;

	AtlasEntry*			FindEntry(const char* pszName) const;
	int					FindEntryIndex(const char* pszName) const;

	int					GetEntryCount() const;

protected:

	void				Init(const char* pszMaterialName, bool bCreateOwnVBO = false, int maxQuads = 16384);
	void				Shutdown();

	IMaterialPtr		m_material;
	IGPUBufferPtr		m_vertexBuffer;
	IGPUBufferPtr		m_indexBuffer;
};

//------------------------------------------------------------------------------------

class CParticleLowLevelRenderer
{
	friend class CParticleBatch;
	friend class CShadowDecalRenderer;

public:
	void				Init();
	void				Shutdown();

	bool				IsInitialized() const { return m_initialized; }

	CParticleBatch*		CreateBatch(const char* materialName, bool createOwnVBO = false, int maxQuads = 16384, CParticleBatch* insertAfter = nullptr);
	CParticleBatch*		FindBatch(const char* materialName) const;

	// void				AddRenderGroup(CParticleBatch* pRenderGroup, CParticleBatch* after = nullptr);
	// void				RemoveRenderGroup(CParticleBatch* pRenderGroup);

	void				PreloadMaterials();

	// prepares render buffers and sends renderables to ViewRenderer
	void				Render(int nRenderFlags, IGPURenderPassRecorder* rendPassRecorder);
	void				ClearBuffers();

protected:
	bool				InitBuffers();
	bool				ShutdownBuffers();

	Array<CParticleBatch*>	m_batchs{ PP_SL };
	IVertexFormat*		m_vertexFormat{ nullptr };
	bool				m_initialized{ false };
};

//------------------------------------------------------------------------------------

//-----------------------------------
// Effect elementary
//-----------------------------------8

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
	CParticleBatch*	group { nullptr };		// atlas
	AtlasEntry*		tex {nullptr};			// texture name in atlas

	MColor			vColor;
	Vector3D		vOrigin;
	Vector3D		vLockDir;

	float			fWide { 1.0f };
	float			fTall { 1.0f };

	float			fZAngle { 1.0f };

	int				nFlags { 0 };
};

// draws particle
void Effects_DrawBillboard(PFXBillboard_t* effect, CViewParams* view, Volume* frustum);
void Effects_DrawBillboard(PFXBillboard_t* effect, const Matrix4x4& viewMatrix, Volume* frustum);

//------------------------------------------------------------------------------------

extern CStaticAutoPtr<CParticleLowLevelRenderer> g_pfxRender;

