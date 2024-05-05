//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium particles renderer
//				A part of particle system
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "SpriteBuilder.h"

class IVertexFormat;
class CViewParams;
class Volume;
class IGPURenderPassRecorder;
class IGPUCommandRecorder;
struct AtlasEntry;
struct VertexLayoutDesc;
struct RenderPassContext;

class IGPUBuffer;
using IGPUBufferPtr = CRefPtr<IGPUBuffer>;

class IMaterial;
using IMaterialPtr = CRefPtr<IMaterial>;

using PFXAtlasRef = int;

static constexpr const PFXAtlasRef PFX_ATLAS_REF_INVALID = -1;

// particle vertex with color
struct PFXVertex
{
	static const VertexLayoutDesc& GetVertexLayoutDesc();

	PFXVertex() = default;
	PFXVertex(const Vector3D& p, const Vector2D& t, const ColorRGBA& c);
	
	Vector3D		point;
	TVec2D<half>	texcoord;
	uint			color{ 0xffffffff };
};

//
// Particle batch for creating primitives
//
class CParticleBatch : public CSpriteBuilder<PFXVertex>
{
	friend class CParticleRenderer;

public:
	virtual	~CParticleBatch();

	// renders this buffer
	void				UpdateVBO(IGPUCommandRecorder* bufferUpdateCmds);
	void				Render(const RenderPassContext& passContext, IGPUCommandRecorder* bufferUpdateCmds = nullptr, bool flushBuffers = true);

	// allocates a fixed strip for further use.
	// returns vertex start index. Returns -1 if failed
	int					AllocateGeom( int nVertices, int nIndices, PFXVertex** verts, uint16** indices, bool preSetIndices = false );
	void				AddParticleStrip(PFXVertex* verts, int nVertices);

	const IMaterialPtr&	GetMaterial() const	{ return m_material; }

	const AtlasEntry*	GetEntry(int idx) const;
	const AtlasEntry*	FindEntry(const char* pszName) const;
	int					FindEntryIndex(const char* pszName) const;

	int					GetEntryCount() const;

protected:

	void				Init(const char* pszMaterialName, bool bCreateOwnVBO = false, int maxQuads = 16384);
	void				Shutdown();

	IMaterialPtr		m_material;
	IGPUBufferPtr		m_vertexBuffer;
	IGPUBufferPtr		m_indexBuffer;
	bool				m_bufferDirty{ true };
};

//------------------------------------------------------------------------------------

class CParticleRenderer
{
	friend class CParticleBatch;
	friend class CShadowDecalRenderer;

public:
	void				Init();
	void				Shutdown();
	bool				IsInitialized() const { return m_initialized; }

	CParticleBatch*		CreateBatch(const char* materialName, bool createOwnVBO = false, int maxQuads = 16384, CParticleBatch* insertAfter = nullptr);
	CParticleBatch*		FindBatch(const char* materialName) const;
	bool				GetBatchAndRectangle(PFXAtlasRef atlasRef, CParticleBatch*& batch, AARectangle& rect) const;
	AARectangle			GetRectangle(PFXAtlasRef atlasRef) const;

	void				PreloadMaterials();

	PFXAtlasRef			FindAtlasRef(const char* name) const;
	int					AllocateGeom(PFXAtlasRef atlasRef, int vertCount, int indexCount, PFXVertex** verts, uint16** indices, bool preSetIndices = false);

	// prepares render buffers and sends renderables to ViewRenderer
	void				UpdateBuffers(IGPUCommandRecorder* bufferUpdateCmds);

	void				Render(const RenderPassContext& passContext, IGPUCommandRecorder* bufferUpdateCmds = nullptr, bool flushBuffer = true);
	void				ClearBuffers();

protected:
	bool				InitBuffers();
	bool				ShutdownBuffers();

	Array<CParticleBatch*>	m_batchs{ PP_SL };
	IVertexFormat*		m_vertexFormat{ nullptr };
	bool				m_initialized{ false };
};

//------------------------------------------------------------------------------------

enum EEffectFlags
{
	EFFECT_FLAG_LOCK_X				= (1 << 0),
	EFFECT_FLAG_LOCK_Y				= (1 << 1),
	EFFECT_FLAG_ALWAYS_VISIBLE		= (1 << 2),
	EFFECT_FLAG_NO_DEPTHTEST		= (1 << 3),
	EFFECT_FLAG_RADIAL_ALIGNING		= (1 << 4),
};

struct PFXBillboard
{
	PFXAtlasRef	atlasRef;

	MColor		color;
	Vector3D	origin;
	Vector3D	lockDir;

	Vector2D	size{ 1.0f, 1.0f };

	float		zAngle { 1.0f };
	int			flags { 0 };
};

// draws particle
void Effects_DrawBillboard(const PFXBillboard& effect, const CViewParams& view, const Volume* frustum = nullptr);
void Effects_DrawBillboard(const PFXBillboard& effect, const Matrix4x4& viewMatrix, const Volume* frustum = nullptr);

//------------------------------------------------------------------------------------

extern CStaticAutoPtr<CParticleRenderer> g_pfxRender;

