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

static VertexFormatDesc_t g_PFXVertexFormatDesc[] = {
	{ 0, 3, VERTEXATTRIB_POSITION, ATTRIBUTEFORMAT_FLOAT, "position" },		// position
	{ 0, 2, VERTEXATTRIB_TEXCOORD, ATTRIBUTEFORMAT_HALF, "texcoord" },		// texture coord
	{ 0, 4, VERTEXATTRIB_COLOR, ATTRIBUTEFORMAT_UBYTE, "color" },			// color
	//{ 0, 4, VERTEXATTRIB_TEXCOORD, ATTRIBUTEFORMAT_HALF, "normal" },		// normal; unused
};

using namespace Threading;
static CEqMutex s_particleRenderMutex;

CAutoPtr<CParticleLowLevelRenderer> g_pPFXRenderer;

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

	m_material = materials->GetMaterial(pszMaterialName);
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

int CParticleBatch::AllocateGeom( int nVertices, int nIndices, PFXVertex_t** verts, uint16** indices, bool preSetIndices )
{
	if(!g_pPFXRenderer->IsInitialized())
		return -1;

	Threading::CScopedMutex m(s_particleRenderMutex);

	return _AllocateGeom(nVertices, nIndices, verts, indices, preSetIndices);
}

void CParticleBatch::AddParticleStrip(PFXVertex_t* verts, int nVertices)
{
	if(!g_pPFXRenderer->IsInitialized())
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

	g_pShaderAPI->Reset(STATE_RESET_VBO);

	// copy buffers
	if(!g_pPFXRenderer->MakeVBOFrom(this))
		return;

	g_pShaderAPI->SetVertexFormat(g_pPFXRenderer->m_vertexFormat);
	g_pShaderAPI->SetVertexBuffer(g_pPFXRenderer->m_vertexBuffer, 0);

	if(m_numIndices)
		g_pShaderAPI->SetIndexBuffer(g_pPFXRenderer->m_indexBuffer);

	const bool invertCull = m_invertCull || (nViewRenderFlags & EPRFLAG_INVERT_CULL);
	materials->SetCullMode(invertCull ? CULL_BACK : CULL_FRONT);

	if(m_useCustomProjMat)
		materials->SetMatrix(MATRIXMODE_PROJECTION, m_customProjMat);
	materials->SetMatrix(MATRIXMODE_WORLD, identity4);

	materials->BindMaterial(m_material);

	const int primMode = m_triangleListMode ? PRIM_TRIANGLES : PRIM_TRIANGLE_STRIP;

	// draw
	if(m_numIndices)
		g_pShaderAPI->DrawIndexedPrimitives((ER_PrimitiveType)primMode, 0, m_numIndices, 0, m_numVertices);
	else
		g_pShaderAPI->DrawNonIndexedPrimitives((ER_PrimitiveType)primMode, 0, m_numVertices);

#if 0
	HOOK_TO_CVAR(r_wireframe)
	if(r_wireframe->GetBool())
	{
		materials->SetRasterizerStates(CULL_FRONT, FILL_WIREFRAME);

		materials->SetDepthStates(false,false);

		static IShaderProgram* flat = g_pShaderAPI->FindShaderProgram("DefaultFlatColor");

		g_pShaderAPI->Reset(STATE_RESET_SHADER);
		g_pShaderAPI->SetShader(flat);
		g_pShaderAPI->Apply();

		if(m_numIndices)
			g_pShaderAPI->DrawIndexedPrimitives((ER_PrimitiveType)primMode, 0, m_numIndices, 0, m_numVertices);
		else
			g_pShaderAPI->DrawNonIndexedPrimitives((ER_PrimitiveType)primMode, 0, m_numVertices);
	}
#endif
	if(!(nViewRenderFlags & EPRFLAG_DONT_FLUSHBUFFERS))
	{
		m_numVertices = 0;
		m_numIndices = 0;
		m_useCustomProjMat = false;
	}
}

TexAtlasEntry_t* CParticleBatch::GetEntry(int idx) const
{
	CTextureAtlas* atlas = m_material->GetAtlas();
	if (!atlas)
	{
		ASSERT_FAIL("No atlas loaded for material %s", m_material->GetName());
		return nullptr;
	}

	return atlas->GetEntry(idx);
}

TexAtlasEntry_t* CParticleBatch::FindEntry(const char* pszName) const
{
	CTextureAtlas* atlas = m_material->GetAtlas();
	if (!atlas)
	{
		ASSERT_FAIL("No atlas loaded for material %s", m_material->GetName());
		return nullptr;
	}

	TexAtlasEntry_t* atlEntry = atlas->FindEntry(pszName);
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

	MsgWarning("Initializing particle buffers...\n");

	m_vbMaxQuads = r_particleBufferSize.GetInt();

	m_vertexBuffer = g_pShaderAPI->CreateVertexBuffer(BUFFER_DYNAMIC, m_vbMaxQuads * 4, sizeof(PFXVertex_t), nullptr);
	m_indexBuffer = g_pShaderAPI->CreateIndexBuffer(m_vbMaxQuads * 6, sizeof(int16), BUFFER_DYNAMIC, nullptr);

	if(!m_vertexFormat)
		m_vertexFormat = g_pShaderAPI->CreateVertexFormat("PFXVertex", g_PFXVertexFormatDesc, elementsOf(g_PFXVertexFormatDesc));

	if(m_vertexBuffer && m_indexBuffer && m_vertexFormat)
		m_initialized = true;

	return false;
}

bool CParticleLowLevelRenderer::ShutdownBuffers()
{
	if(!m_initialized)
		return true;

	MsgWarning("Destroying particle buffers...\n");

	g_pShaderAPI->DestroyVertexFormat(m_vertexFormat);
	g_pShaderAPI->DestroyIndexBuffer(m_indexBuffer);
	g_pShaderAPI->DestroyVertexBuffer(m_vertexBuffer);

	m_vertexBuffer = nullptr;
	m_indexBuffer = nullptr;
	m_vertexFormat = nullptr;

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
		materials->PutMaterialToLoadingQueue(m_batchs[i]->m_material);
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

bool CParticleLowLevelRenderer::MakeVBOFrom(const CSpriteBuilder<PFXVertex_t>* pGroup)
{
	if(!m_initialized)
		return false;

	const uint16 nVerts	= pGroup->m_numVertices;
	const uint16 nIndices = pGroup->m_numIndices;

	if(nVerts == 0)
		return false;

	if(nVerts > SVBO_MAX_SIZE(m_vbMaxQuads, PFXVertex_t))
		return false;

	m_vertexBuffer->Update((void*)pGroup->m_pVerts, nVerts, 0, true);

	if(nIndices)
		m_indexBuffer->Update((void*)pGroup->m_pIndices, nIndices, 0, true);

	return true;
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

	PFXVertex_t* verts;
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
	verts[0].texcoord = Vector2D(texCoords.vrightBottom.x, texCoords.vrightBottom.y);
	verts[0].color = color;

	verts[1].point = effect->vOrigin + (vUp * effect->fTall) - (effect->fWide * vRight);
	verts[1].texcoord = Vector2D(texCoords.vrightBottom.x, texCoords.vleftTop.y);
	verts[1].color = color;

	verts[2].point = effect->vOrigin - (vUp * effect->fTall) + (effect->fWide * vRight);
	verts[2].texcoord = Vector2D(texCoords.vleftTop.x, texCoords.vrightBottom.y);
	verts[2].color = color;

	verts[3].point = effect->vOrigin - (vUp * effect->fTall) - (effect->fWide * vRight);
	verts[3].texcoord = Vector2D(texCoords.vleftTop.x, texCoords.vleftTop.y);
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


	PFXVertex_t* verts;
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
	verts[0].texcoord = Vector2D(texCoords.vrightBottom.x, texCoords.vrightBottom.y);
	verts[0].color = color;

	verts[1].point = effect->vOrigin + (vUp * effect->fTall) - (effect->fWide * vRight);
	verts[1].texcoord = Vector2D(texCoords.vrightBottom.x, texCoords.vleftTop.y);
	verts[1].color = color;

	verts[2].point = effect->vOrigin - (vUp * effect->fTall) + (effect->fWide * vRight);
	verts[2].texcoord = Vector2D(texCoords.vleftTop.x, texCoords.vrightBottom.y);
	verts[2].color = color;

	verts[3].point = effect->vOrigin - (vUp * effect->fTall) - (effect->fWide * vRight);
	verts[3].texcoord = Vector2D(texCoords.vleftTop.x, texCoords.vleftTop.y);
	verts[3].color = color;
}
