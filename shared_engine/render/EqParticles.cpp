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

CStaticAutoPtr<CParticleLowLevelRenderer> g_pfxRender;

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

	m_material = g_matSystem->GetMaterial(pszMaterialName);
	g_matSystem->QueueLoading(m_material);
}

void CParticleBatch::Shutdown()
{
	if(!m_initialized)
		return;

	CSpriteBuilder::Shutdown();
	m_material = nullptr;
}

void CParticleBatch::SetCustomProjectionMatrix(const Matrix4x4& mat)
{
	m_useCustomProjMat = true;
	m_customProjMat = mat;
}

int CParticleBatch::AllocateGeom( int nVertices, int nIndices, PFXVertex** verts, uint16** indices, bool preSetIndices )
{
	if(!g_pfxRender->IsInitialized())
		return -1;

	Threading::CScopedMutex m(s_particleRenderMutex);

	return _AllocateGeom(nVertices, nIndices, verts, indices, preSetIndices);
}

void CParticleBatch::AddParticleStrip(PFXVertex* verts, int nVertices)
{
	if(!g_pfxRender->IsInitialized())
		return;

	Threading::CScopedMutex m(s_particleRenderMutex);

	_AddParticleStrip(verts, nVertices);
}

// prepares render buffers and sends renderables to ViewRenderer
void CParticleBatch::Render(int nViewRenderFlags)
{
	if(!m_initialized || !r_drawParticles.GetBool())
	{
		m_numIndices = 0;
		m_numVertices = 0;
		return;
	}

	if(m_numVertices == 0 || (!m_triangleListMode && m_numIndices == 0))
		return;

	g_matSystem->SetCullMode(CULL_FRONT);

	if (m_useCustomProjMat)
		g_matSystem->SetMatrix(MATRIXMODE_PROJECTION, m_customProjMat);
	g_matSystem->SetMatrix(MATRIXMODE_WORLD, identity4);

	RenderDrawCmd drawCmd;
	drawCmd.vertexLayout = g_pfxRender->m_vertexFormat;
	drawCmd.vertexBuffers[0] = g_renderAPI->CreateBuffer(BufferInfo(m_pVerts, m_numVertices), BUFFERUSAGE_VERTEX, "PFXVertexBuffer");
	drawCmd.material = m_material;
	drawCmd.primitiveTopology = m_triangleListMode ? PRIM_TRIANGLES : PRIM_TRIANGLE_STRIP;
	if (m_numIndices)
	{
		drawCmd.indexBuffer = g_renderAPI->CreateBuffer(BufferInfo(m_pIndices, m_numIndices), BUFFERUSAGE_INDEX, "PFXIndexBuffer");
		drawCmd.SetDrawIndexed(m_numIndices, 0, m_numVertices);
	}
	else
		drawCmd.SetDrawNonIndexed(m_numVertices);

	g_matSystem->Draw(drawCmd);

	if(!(nViewRenderFlags & EPRFLAG_DONT_FLUSHBUFFERS))
	{
		m_numVertices = 0;
		m_numIndices = 0;
		m_useCustomProjMat = false;
	}
}

AtlasEntry* CParticleBatch::GetEntry(int idx) const
{
	CTextureAtlas* atlas = m_material->GetAtlas();
	if (!atlas)
	{
		ASSERT_FAIL("No atlas loaded for material %s", m_material->GetName());
		return nullptr;
	}

	return atlas->GetEntry(idx);
}

AtlasEntry* CParticleBatch::FindEntry(const char* pszName) const
{
	CTextureAtlas* atlas = m_material->GetAtlas();
	if (!atlas)
	{
		ASSERT_FAIL("No atlas loaded for material %s", m_material->GetName());
		return nullptr;
	}

	AtlasEntry* atlEntry = atlas->FindEntry(pszName);
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

void CParticleLowLevelRenderer::Init()
{
	InitBuffers();
}

void CParticleLowLevelRenderer::Shutdown()
{
	ShutdownBuffers();

	for (int i = 0; i < m_batchs.numElem(); i++)
		delete m_batchs[i];
	m_batchs.clear(true);
}

bool CParticleLowLevelRenderer::InitBuffers()
{
	if(m_initialized)
		return true;

	m_vbMaxQuads = r_particleBufferSize.GetInt();

	if(!m_vertexFormat)
	{
		const VertexLayoutDesc& fmtDesc = PFXVertex::GetVertexLayoutDesc();
		m_vertexFormat = g_renderAPI->CreateVertexFormat("PFXVertex", ArrayCRef(&fmtDesc, 1));
	}

	if(m_vertexFormat)
		m_initialized = true;

	return false;
}

bool CParticleLowLevelRenderer::ShutdownBuffers()
{
	if(!m_initialized)
		return true;
	m_initialized = false;
	return true;
}

CParticleBatch*	CParticleLowLevelRenderer::CreateBatch(const char* materialName, bool createOwnVBO, int maxQuads, CParticleBatch* insertAfter)
{
	ASSERT(FindBatch(materialName) == nullptr);

	CParticleBatch* batch = PPNew CParticleBatch();
	batch->Init(materialName, createOwnVBO, maxQuads);

	if (insertAfter)
	{
		const int idx = arrayFindIndex(m_batchs, insertAfter);
		m_batchs.insert(batch, idx + 1);
	}
	else
		m_batchs.append(batch);

	return batch;
}

CParticleBatch* CParticleLowLevelRenderer::FindBatch(const char* materialName) const
{
	for (int i = 0; i < m_batchs.numElem(); ++i)
	{
		if (!stricmp(m_batchs[i]->m_material->GetName(), materialName))
			return m_batchs[i];
	}
	return nullptr;
}

void CParticleLowLevelRenderer::PreloadMaterials()
{
	for(int i = 0; i < m_batchs.numElem(); i++)
		g_matSystem->QueueLoading(m_batchs[i]->m_material);
}

// prepares render buffers and sends renderables to ViewRenderer
void CParticleLowLevelRenderer::Render(int nRenderFlags)
{
	for(int i = 0; i < m_batchs.numElem(); i++)
		m_batchs[i]->Render( nRenderFlags );
}

void CParticleLowLevelRenderer::ClearBuffers()
{
	for(int i = 0; i < m_batchs.numElem(); i++)
		m_batchs[i]->ClearBuffers();
}

//----------------------------------------------------------------------------------------------------

void Effects_DrawBillboard(PFXBillboard_t* effect, CViewParams* view, Volume* frustum)
{
	if(!(effect->nFlags & EFFECT_FLAG_NO_FRUSTUM_CHECK))
	{
		float fBillboardSize = (effect->fWide > effect->fTall) ? effect->fWide : effect->fTall;

		if(!frustum->IsSphereInside(effect->vOrigin, fBillboardSize))
			return;
	}

	PFXVertex* verts;
	if(effect->group->AllocateGeom(4,4,&verts, nullptr, true) < 0)
		return;

	Vector3D angles, vRight, vUp;

	if(effect->nFlags & EFFECT_FLAG_RADIAL_ALIGNING)
		angles = VectorAngles(fastNormalize(effect->vOrigin - view->GetOrigin()));
	else
		angles = view->GetAngles() + Vector3D(0,0,effect->fZAngle);

	if(effect->nFlags & EFFECT_FLAG_LOCK_X)
		angles.x = 0;

	if(effect->nFlags & EFFECT_FLAG_LOCK_Y)
		angles.y = 0;

	AngleVectors(angles, nullptr, &vRight, &vUp);

	AARectangle texCoords(0,0,1,1);

	if(effect->tex)
		texCoords = effect->tex->rect;

	const uint color = effect->vColor.pack();

	verts[0].point = effect->vOrigin + (vUp * effect->fTall) + (effect->fWide * vRight);
	verts[0].texcoord = Vector2D(texCoords.rightBottom.x, texCoords.rightBottom.y);
	verts[0].color = color;

	verts[1].point = effect->vOrigin + (vUp * effect->fTall) - (effect->fWide * vRight);
	verts[1].texcoord = Vector2D(texCoords.rightBottom.x, texCoords.leftTop.y);
	verts[1].color = color;

	verts[2].point = effect->vOrigin - (vUp * effect->fTall) + (effect->fWide * vRight);
	verts[2].texcoord = Vector2D(texCoords.leftTop.x, texCoords.rightBottom.y);
	verts[2].color = color;

	verts[3].point = effect->vOrigin - (vUp * effect->fTall) - (effect->fWide * vRight);
	verts[3].texcoord = Vector2D(texCoords.leftTop.x, texCoords.leftTop.y);
	verts[3].color = color;
}

void Effects_DrawBillboard(PFXBillboard_t* effect, const Matrix4x4& viewMatrix, Volume* frustum)
{
	if(!(effect->nFlags & EFFECT_FLAG_NO_FRUSTUM_CHECK))
	{
		float fBillboardSize = (effect->fWide > effect->fTall) ? effect->fWide : effect->fTall;

		if(!frustum->IsSphereInside(effect->vOrigin, fBillboardSize))
			return;
	}


	PFXVertex* verts;
	if(effect->group->AllocateGeom(4,4,&verts, nullptr, true) < 0)
		return;

	Vector3D vRight, vUp;

	vRight = viewMatrix.rows[0].xyz();
	vUp = viewMatrix.rows[1].xyz();

	AARectangle texCoords(0,0,1,1);

	if(effect->tex)
		texCoords = effect->tex->rect;

	const uint color = effect->vColor.pack();

	verts[0].point = effect->vOrigin + (vUp * effect->fTall) + (effect->fWide * vRight);
	verts[0].texcoord = Vector2D(texCoords.rightBottom.x, texCoords.rightBottom.y);
	verts[0].color = color;

	verts[1].point = effect->vOrigin + (vUp * effect->fTall) - (effect->fWide * vRight);
	verts[1].texcoord = Vector2D(texCoords.rightBottom.x, texCoords.leftTop.y);
	verts[1].color = color;

	verts[2].point = effect->vOrigin - (vUp * effect->fTall) + (effect->fWide * vRight);
	verts[2].texcoord = Vector2D(texCoords.leftTop.x, texCoords.rightBottom.y);
	verts[2].color = color;

	verts[3].point = effect->vOrigin - (vUp * effect->fTall) - (effect->fWide * vRight);
	verts[3].texcoord = Vector2D(texCoords.leftTop.x, texCoords.leftTop.y);
	verts[3].color = color;
}
