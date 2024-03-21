//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Particle system
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "core/IConsoleCommands.h"
#include "core/ConVar.h"
#include "utils/TextureAtlas.h"
#include "EqParticles.h"
#include "ViewParams.h"

#include "materialsystem1/IMaterialSystem.h"

const VertexLayoutDesc& PFXVertex::GetVertexLayoutDesc()
{
	static VertexLayoutDesc s_PFXVertexLayoutDesc = Builder<VertexLayoutDesc>()
		.Stride(sizeof(PFXVertex))
		.Attribute(VERTEXATTRIB_POSITION, "position", 0, 0, ATTRIBUTEFORMAT_FLOAT, 3)
		.Attribute(VERTEXATTRIB_TEXCOORD, "texCoord", 1, offsetOf(PFXVertex, texcoord), ATTRIBUTEFORMAT_HALF, 2)
		.Attribute(VERTEXATTRIB_COLOR, "color", 2, offsetOf(PFXVertex, color), ATTRIBUTEFORMAT_UINT8, 4)
		.End();
	return s_PFXVertexLayoutDesc;
}

using namespace Threading;
static CEqMutex s_particleRenderMutex;

CStaticAutoPtr<CParticleRenderer> g_pfxRender;

//----------------------------------------------------------------------------------------------------

DECLARE_CVAR(r_particleBufferSize, "16384", "particle buffer size, change requires restart game", CV_ARCHIVE);
DECLARE_CVAR(r_drawParticles, "1", "Render particles", CV_CHEAT);

CParticleBatch::~CParticleBatch()
{
	Shutdown();
}

void CParticleBatch::Init( const char* pszMaterialName, bool bCreateOwnVBO, int maxQuads )
{
	maxQuads = min(r_particleBufferSize.GetInt(), maxQuads);

	// init sprite stuff
	CSpriteBuilder::Init(maxQuads);

	m_material = g_matSystem->GetMaterial(pszMaterialName, SHADER_VERTEX_ID(PFXVertex));
	g_matSystem->QueueLoading(m_material);
}

void CParticleBatch::Shutdown()
{
	if(!m_initialized)
		return;

	CSpriteBuilder::Shutdown();
	m_material = nullptr;
	m_vertexBuffer = nullptr;
	m_indexBuffer = nullptr;
}

int CParticleBatch::AllocateGeom( int nVertices, int nIndices, PFXVertex** verts, uint16** indices, bool preSetIndices )
{
	if(!g_pfxRender->IsInitialized())
		return -1;

	Threading::CScopedMutex m(s_particleRenderMutex);

	const int result = _AllocateGeom(nVertices, nIndices, verts, indices, preSetIndices);
	if (result != -1) 
		m_bufferDirty = true;
	return result;
}

void CParticleBatch::AddParticleStrip(PFXVertex* verts, int nVertices)
{
	if(!g_pfxRender->IsInitialized())
		return;

	Threading::CScopedMutex m(s_particleRenderMutex);

	_AddParticleStrip(verts, nVertices);
	m_bufferDirty = true;
}

void CParticleBatch::UpdateVBO(IGPUCommandRecorder* bufferUpdateCmds)
{
	if (!m_bufferDirty)
		return;

	if (!m_vertexBuffer)
		m_vertexBuffer = g_renderAPI->CreateBuffer(BufferInfo(1, SVBO_MAX_SIZE(m_maxQuads, PFXVertex)), BUFFERUSAGE_VERTEX | BUFFERUSAGE_COPY_DST, "PFXVertexBuffer");
	if (!m_indexBuffer)
		m_indexBuffer = g_renderAPI->CreateBuffer(BufferInfo(1, SIBO_MAX_SIZE(m_maxQuads)), BUFFERUSAGE_INDEX | BUFFERUSAGE_COPY_DST, "PFXIndexBuffer");

	if (bufferUpdateCmds)
	{
		bufferUpdateCmds->WriteBuffer(m_vertexBuffer, m_pVerts, AlignBufferSize((int)m_numVertices * sizeof(PFXVertex)), 0);
		bufferUpdateCmds->WriteBuffer(m_indexBuffer, m_pIndices, AlignBufferSize((int)m_numIndices * sizeof(uint16)), 0);
	}
	else
	{
		m_vertexBuffer->Update(m_pVerts, AlignBufferSize((int)m_numVertices * sizeof(PFXVertex)), 0);
		m_indexBuffer->Update(m_pIndices, AlignBufferSize((int)m_numIndices * sizeof(uint16)), 0);
	}

	m_bufferDirty = false;
}

// prepares render buffers and sends renderables to ViewRenderer
void CParticleBatch::Render(const RenderPassContext& passContext, IGPUCommandRecorder* bufferUpdateCmds, bool flushBuffer)
{
	if (!m_initialized || !r_drawParticles.GetBool())
	{
		m_numIndices = 0;
		m_numVertices = 0;
		return;
	}

	if (m_numVertices == 0 || (!m_triangleListMode && m_numIndices == 0))
		return;

	if (bufferUpdateCmds)
		UpdateVBO(bufferUpdateCmds);

	RenderDrawCmd drawCmd;
	drawCmd
		.SetMaterial(m_material)
		.SetInstanceFormat(g_pfxRender->m_vertexFormat)
		.SetVertexBuffer(0, m_vertexBuffer);
		
	if (m_numIndices)
	{
		drawCmd
			.SetIndexBuffer(m_indexBuffer, INDEXFMT_UINT16)
			.SetDrawIndexed(m_triangleListMode ? PRIM_TRIANGLES : PRIM_TRIANGLE_STRIP, m_numIndices, 0, m_numVertices);
	}
	else
		drawCmd.SetDrawNonIndexed(m_triangleListMode ? PRIM_TRIANGLES : PRIM_TRIANGLE_STRIP, m_numVertices);

	g_matSystem->SetupDrawCommand(drawCmd, passContext);

	if(flushBuffer)
	{
		m_numVertices = 0;
		m_numIndices = 0;
	}
}

const AtlasEntry* CParticleBatch::GetEntry(int idx) const
{
	const CTextureAtlas* atlas = m_material->GetAtlas();
	if (!atlas)
	{
		ASSERT_FAIL("No atlas loaded for material %s", m_material->GetName());
		return nullptr;
	}

	return atlas->GetEntry(idx);
}

const AtlasEntry* CParticleBatch::FindEntry(const char* pszName) const
{
	const CTextureAtlas* atlas = m_material->GetAtlas();
	if (!atlas)
	{
		ASSERT_FAIL("No atlas loaded for material %s", m_material->GetName());
		return nullptr;
	}

	const AtlasEntry* atlEntry = atlas->FindEntry(pszName);
	ASSERT_MSG(atlEntry, "Atlas entry '%s' not found in %s", pszName, m_material->GetName());
	return atlEntry;
}

int CParticleBatch::FindEntryIndex(const char* pszName) const
{
	CTextureAtlas* atlas = m_material->GetAtlas();
	if (!atlas)
	{
		ASSERT_FAIL("No atlas loaded for material %s", m_material->GetName());
		return -1;
	}

	const int atlEntryIdx = atlas->FindEntryIndex(pszName);
	ASSERT_MSG(atlEntryIdx != -1, "Atlas entry '%s' not found in %s", pszName, m_material->GetName());
	return atlEntryIdx;
}

int CParticleBatch::GetEntryCount() const
{
	CTextureAtlas* atlas = m_material->GetAtlas();
	if (!atlas)
		return 0;

	return atlas->GetEntryCount();
}

//----------------------------------------------------------------------------------------------------------

void CParticleRenderer::Init()
{
	InitBuffers();
}

void CParticleRenderer::Shutdown()
{
	ShutdownBuffers();

	for (CParticleBatch* batch : m_batchs)
		delete batch;
	m_batchs.clear(true);
}

bool CParticleRenderer::InitBuffers()
{
	if(m_initialized)
		return true;

	if(!m_vertexFormat)
	{
		const VertexLayoutDesc& fmtDesc = PFXVertex::GetVertexLayoutDesc();
		m_vertexFormat = g_renderAPI->CreateVertexFormat("PFXVertex", ArrayCRef(&fmtDesc, 1));
	}

	if(m_vertexFormat)
		m_initialized = true;

	return false;
}

bool CParticleRenderer::ShutdownBuffers()
{
	if(!m_initialized)
		return true;

	g_renderAPI->DestroyVertexFormat(m_vertexFormat);
	m_vertexFormat = nullptr;

	m_initialized = false;
	return true;
}

CParticleBatch*	CParticleRenderer::CreateBatch(const char* materialName, bool createOwnVBO, int maxQuads, CParticleBatch* insertAfter)
{
	ASSERT(FindBatch(materialName) == nullptr);

	CParticleBatch* batch = PPNew CParticleBatch();
	batch->Init(materialName, createOwnVBO, maxQuads);
	for (int i = 0; i < batch->GetEntryCount(); ++i)
	{
		const AtlasEntry* atlEntry = batch->GetEntry(i);
		PFXAtlasRef ref = FindAtlasRef(atlEntry->name);
		ASSERT_MSG(!ref, "Particle atlas' %s' has '%s' which is already found in '%s'", atlEntry->name, batch->GetMaterial()->GetName(), ref.batch->GetMaterial()->GetName());
	}

	if (insertAfter)
	{
		const int idx = arrayFindIndex(m_batchs, insertAfter);
		m_batchs.insert(batch, idx + 1);
	}
	else
		m_batchs.append(batch);

	return batch;
}

CParticleBatch* CParticleRenderer::FindBatch(const char* materialName) const
{
	for (CParticleBatch* batch : m_batchs)
	{
		if (!stricmp(batch->m_material->GetName(), materialName))
			return batch;
	}
	return nullptr;
}

PFXAtlasRef CParticleRenderer::FindAtlasRef(const char* name) const
{
	for (CParticleBatch* batch : m_batchs)
	{
		const AtlasEntry* atlEntry = batch->GetMaterial()->GetAtlas()->FindEntry(name);

		if(atlEntry)
			return PFXAtlasRef{ atlEntry, batch };
	}
	return PFXAtlasRef{};
}

void CParticleRenderer::PreloadMaterials()
{
	for(CParticleBatch* batch : m_batchs)
		g_matSystem->QueueLoading(batch->m_material);
}

void CParticleRenderer::UpdateBuffers(IGPUCommandRecorder* bufferUpdateCmds)
{
	for (CParticleBatch* batch : m_batchs)
		batch->UpdateVBO(bufferUpdateCmds);
}

// prepares render buffers and sends renderables to ViewRenderer
void CParticleRenderer::Render(const RenderPassContext& passContext, IGPUCommandRecorder* bufferUpdateCmds, bool flushBuffer)
{
	for(CParticleBatch* batch : m_batchs)
		batch->Render(passContext, bufferUpdateCmds, flushBuffer);
}

void CParticleRenderer::ClearBuffers()
{
	for(CParticleBatch* batch : m_batchs)
		batch->ClearBuffers();
}

//----------------------------------------------------------------------------------------------------

void Effects_DrawBillboard(const PFXBillboard& effect, const CViewParams& view, const Volume* frustum)
{
	ASSERT(effect.atlasRef);
	if (!effect.atlasRef)
		return;

	if(frustum)
	{
		const float size = max(effect.size.x, effect.size.y);
		if(!frustum->IsSphereInside(effect.origin, size))
			return;
	}

	PFXVertex* verts;
	if(effect.atlasRef.batch->AllocateGeom(4,4,&verts, nullptr, true) < 0)
		return;

	Vector3D angles;
	if(effect.flags & EFFECT_FLAG_RADIAL_ALIGNING)
		angles = VectorAngles(fastNormalize(effect.origin - view.GetOrigin()));
	else
		angles = view.GetAngles() + Vector3D(0,0,effect.zAngle);

	if(effect.flags & EFFECT_FLAG_LOCK_X)
		angles.x = 0;

	if(effect.flags & EFFECT_FLAG_LOCK_Y)
		angles.y = 0;

	Vector3D vRight, vUp;
	AngleVectors(angles, nullptr, &vRight, &vUp);

	const AARectangle texCoords = effect.atlasRef.entry->rect;

	const uint color = effect.color.pack();

	verts[0].point = effect.origin + (vUp * effect.size.y) + (effect.size.x * vRight);
	verts[0].texcoord = Vector2D(texCoords.rightBottom.x, texCoords.rightBottom.y);
	verts[0].color = color;

	verts[1].point = effect.origin + (vUp * effect.size.y) - (effect.size.x * vRight);
	verts[1].texcoord = Vector2D(texCoords.rightBottom.x, texCoords.leftTop.y);
	verts[1].color = color;

	verts[2].point = effect.origin - (vUp * effect.size.y) + (effect.size.x * vRight);
	verts[2].texcoord = Vector2D(texCoords.leftTop.x, texCoords.rightBottom.y);
	verts[2].color = color;

	verts[3].point = effect.origin - (vUp * effect.size.y) - (effect.size.x * vRight);
	verts[3].texcoord = Vector2D(texCoords.leftTop.x, texCoords.leftTop.y);
	verts[3].color = color;
}

void Effects_DrawBillboard(const PFXBillboard& effect, const Matrix4x4& viewMatrix, const Volume* frustum)
{
	ASSERT(effect.atlasRef);
	if (!effect.atlasRef)
		return;

	if (frustum)
	{
		const float size = max(effect.size.x, effect.size.y);
		if (!frustum->IsSphereInside(effect.origin, size))
			return;
	}

	PFXVertex* verts;
	if(effect.atlasRef.batch->AllocateGeom(4,4,&verts, nullptr, true) < 0)
		return;

	const Vector3D vRight = viewMatrix.rows[0].xyz();
	const Vector3D vUp = viewMatrix.rows[1].xyz();

	const AARectangle texCoords = effect.atlasRef.entry->rect;
	const uint color = effect.color.pack();

	verts[0].point = effect.origin + (vUp * effect.size.y) + (effect.size.x * vRight);
	verts[0].texcoord = Vector2D(texCoords.rightBottom.x, texCoords.rightBottom.y);
	verts[0].color = color;

	verts[1].point = effect.origin + (vUp * effect.size.y) - (effect.size.x * vRight);
	verts[1].texcoord = Vector2D(texCoords.rightBottom.x, texCoords.leftTop.y);
	verts[1].color = color;

	verts[2].point = effect.origin - (vUp * effect.size.y) + (effect.size.x * vRight);
	verts[2].texcoord = Vector2D(texCoords.leftTop.x, texCoords.rightBottom.y);
	verts[2].color = color;

	verts[3].point = effect.origin - (vUp * effect.size.y) - (effect.size.x * vRight);
	verts[3].texcoord = Vector2D(texCoords.leftTop.x, texCoords.leftTop.y);
	verts[3].color = color;
}
