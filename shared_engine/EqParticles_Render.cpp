//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Particle system
//////////////////////////////////////////////////////////////////////////////////

#include "EqParticles.h"
#include "eqGlobalMutex.h"

using namespace Threading;

static CParticleLowLevelRenderer s_pfxRenderer;

CParticleLowLevelRenderer*	g_pPFXRenderer = &s_pfxRenderer;

//----------------------------------------------------------------------------------------------------

ConVar r_particleBufferSize("r_particleBufferSize", "16384", "particle buffer size, change requires restart game", CV_ARCHIVE);

#define PVBO_MAX_SIZE(s)	(s*sizeof(PFXVertex_t)*4)
#define PIBO_MAX_SIZE(s)	(s*(sizeof(uint16)*6))

CParticleRenderGroup::CParticleRenderGroup() : 	
	m_pMaterial(NULL),
	m_pVerts(NULL),
	m_pIndices(NULL),
	m_numVertices(0),
	m_numIndices(0),
	m_bHasOwnVBO(false),
	m_initialized(false),
	m_vertexBuffer(NULL),
	m_indexBuffer(NULL),
	m_vertexFormat(NULL),
	m_useCustomProjMat(false),
	m_maxQuadVerts(0)
{

}

CParticleRenderGroup::~CParticleRenderGroup()
{
	Shutdown();
}

void CParticleRenderGroup::Init( const char* pszMaterialName, bool bCreateOwnVBO, int maxQuads )
{
	m_bHasOwnVBO = bCreateOwnVBO;

	if(m_bHasOwnVBO)
		ASSERT(!"CParticleRenderGroup::Init: bCreateOwnVBO not supported =(");

	m_pMaterial = materials->FindMaterial(pszMaterialName, true);
	m_pMaterial->Ref_Grab();

	m_maxQuadVerts = min(r_particleBufferSize.GetInt(), maxQuads);

	// init buffers
	m_pVerts	= (PFXVertex_t*)malloc(PVBO_MAX_SIZE(m_maxQuadVerts));
	m_pIndices	= (uint16*)malloc(PIBO_MAX_SIZE(m_maxQuadVerts));

	if(!m_pVerts)
		ASSERT(!"FAILED TO ALLOCATE VERTICES!\n");

	if(!m_pIndices)
		ASSERT(!"FAILED TO ALLOCATE INDICES!\n");

	m_numVertices = 0;
	m_numIndices = 0;

	m_initialized = true;
}

void CParticleRenderGroup::Shutdown()
{
	if(!m_initialized)
		return;

	m_initialized = false;

	materials->FreeMaterial(m_pMaterial);
	m_pMaterial = NULL;

	free(m_pVerts);
	free(m_pIndices);

	m_pIndices = NULL;
	m_pVerts = NULL;

	if(m_bHasOwnVBO)
	{
		
	}
}

void CParticleRenderGroup::AddIndex(uint16 idx)
{
	if(!g_pPFXRenderer->IsInitialized())
		return;

	if(m_numVertices*sizeof(PFXVertex_t)*4 > PVBO_MAX_SIZE(m_maxQuadVerts))
	{
		MsgWarning("ParticleRenderGroup overflow\n");

		m_numVertices = 0;
		m_numIndices = 0;

		return;
	}

	m_pIndices[m_numIndices] = idx;
	m_numIndices++;
}

void CParticleRenderGroup::AddVertex(const PFXVertex_t &vert)
{
	m_pVerts[m_numVertices] = vert;
	m_numVertices++;
}

void CParticleRenderGroup::AddVertices(PFXVertex_t* verts, int nVerts)
{
	memcpy(&m_pVerts[m_numVertices], verts, nVerts*sizeof(PFXVertex_t));
	m_numVertices += nVerts;
}

/*
void CParticleRenderGroup::AddVertices2(eqlevelvertex_t *verts, int nVerts)
{
	eqlevelvertex_t* pPartVerts = (eqlevelvertex_t*)m_pVerts;

	memcpy(&pPartVerts[m_numVertices], verts, nVerts*sizeof(eqlevelvertex_t));
	m_numVertices += nVerts;
}
*/
void CParticleRenderGroup::AddIndices(uint16 *indices, int nIndx)
{
	memcpy(&m_pIndices[m_numIndices], indices, nIndx*sizeof(uint16));
	m_numIndices += nIndx;
}

// adds geometry to particle buffers
void CParticleRenderGroup::AddParticleGeom(PFXVertex_t* verts, uint16* indices, int nVertices, int nIndices)
{
	if(!g_pPFXRenderer->IsInitialized())
		return;

	if(m_numVertices*sizeof(PFXVertex_t)*4 > PVBO_MAX_SIZE(m_maxQuadVerts))
	{
		MsgWarning("ParticleRenderGroup overflow\n");

		m_numVertices = 0;
		m_numIndices = 0;

		return;
	}

	Threading::CScopedMutex m(g_pPFXRenderer->m_mutex);

	int num_ind = m_numIndices;

	uint16 nIndicesCurr = 0;

	// if it's a second, first I'll add last index (3 if first, and add first one from fourIndices)
	if( num_ind > 0 )
	{
		int lastIdx = m_pIndices[ num_ind-1 ];

		nIndicesCurr = lastIdx+1;

		// add two last indices to make degenerates
		uint16 degenerate[2] = {lastIdx, nIndicesCurr};

		AddIndices(degenerate, 2);
	}

	//Msg("adding %d v %d i (%d verts %d indx in buffer)\n", nVertices, nIndices, m_numVertices, m_numIndices);
	
	AddVertices(verts, nVertices);

	for(int i = 0; i < nIndices; i++)
		indices[i] += nIndicesCurr;

	AddIndices(indices, nIndices);
}

// adds particle quad
void CParticleRenderGroup::AddParticleQuad(PFXVertex_t fourVerts[4])
{
	uint16 indexArray[4] = {0,1,2,3};

	AddParticleGeom(fourVerts, indexArray, 4, 4 );
}

// allocates a fixed strip for further use.
// returns vertex start index. Returns -1 if failed
// this provides less copy operations
int CParticleRenderGroup::AllocateGeom( int nVertices, int nIndices, PFXVertex_t** verts, uint16** indices, bool preSetIndices )
{
	if(!g_pPFXRenderer->IsInitialized())
		return -1;

	if(m_numVertices*sizeof(PFXVertex_t)*4 > PVBO_MAX_SIZE(m_maxQuadVerts))
	{
		// don't warn me about overflow
		m_numVertices = 0;
		m_numIndices = 0;
		return -1;
	}

	Threading::CScopedMutex m(g_pPFXRenderer->m_mutex);

	AddStripBreak();

	int startVertex = m_numVertices;
	int startIndex = m_numIndices;

	// give the pointers
	*verts = &m_pVerts[startVertex];

	// indices are optional
	if(indices)
		*indices = &m_pIndices[startIndex];

	// apply offsets
	m_numVertices += nVertices;
	m_numIndices += nIndices;

	// make it linear and end with strip break
	if(preSetIndices)
	{
		for(int i = 0; i < nIndices; i++)
			m_pIndices[startIndex+i] = startVertex+i;
	}

	return startVertex;
}

// adds strip to list
void CParticleRenderGroup::AddParticleStrip(PFXVertex_t* verts, int nVertices)
{
	if(!g_pPFXRenderer->IsInitialized())
		return;

	if(nVertices == 0)
		return;

	if(m_numVertices*sizeof(PFXVertex_t)*4 > PVBO_MAX_SIZE(m_maxQuadVerts))
	{
		MsgWarning("ParticleRenderGroup overflow\n");

		m_numVertices = 0;
		m_numIndices = 0;

		return;
	}

	int num_ind = m_numIndices;

	uint16 nIndicesCurr = 0;

	// if it's a second, first I'll add last index (3 if first, and add first one from fourIndices)
	if( num_ind > 0 )
	{
		int lastIdx = m_pIndices[ num_ind-1 ];

		nIndicesCurr = lastIdx+1;

		// add two last indices to make degenerates
		uint16 degenerate[2] = {lastIdx, nIndicesCurr};

		AddIndices(degenerate, 2);
	}

	// add indices

	AddVertices(verts, nVertices);

	for(int i = 0; i < nVertices; i++)
		m_pIndices[m_numIndices++] = nIndicesCurr+i;
}

void CParticleRenderGroup::AddStripBreak()
{
	int num_ind = m_numIndices;

	uint16 nIndicesCurr = 0;

	// if it's a second, first I'll add last index (3 if first, and add first one from fourIndices)
	if( num_ind > 0 )
	{
		int lastIdx = m_pIndices[ num_ind-1 ];

		nIndicesCurr = lastIdx+1;

		// add two last indices to make degenerates
		uint16 degenerate[2] = {lastIdx, nIndicesCurr};

		AddIndices(degenerate, 2);
	}
}

void CParticleRenderGroup::SetCustomProjectionMatrix(const Matrix4x4& mat)
{
	m_useCustomProjMat = true;
	m_customProjMat = mat;
}

// prepares render buffers and sends renderables to ViewRenderer
void CParticleRenderGroup::Render(int nViewRenderFlags)
{
	if(!m_initialized)
	{
		m_numIndices = 0;
		m_numVertices = 0;
		return;
	}

	if(m_numIndices == 0 || m_numVertices == 0)
		return;

	if(!m_bHasOwnVBO)
	{
		g_pShaderAPI->Reset(STATE_RESET_VBO);
		g_pShaderAPI->ApplyBuffers();

		// copy buffers
		if(!g_pPFXRenderer->MakeVBOFrom(this))
			return;

		g_pShaderAPI->SetVertexFormat(g_pPFXRenderer->m_vertexFormat);
		g_pShaderAPI->SetVertexBuffer(g_pPFXRenderer->m_vertexBuffer, 0);
		g_pShaderAPI->SetIndexBuffer(g_pPFXRenderer->m_indexBuffer);
	}
	else
	{
		// BLAH
	}

	materials->SetCullMode((nViewRenderFlags & EPRFLAG_INVERT_CULL) ? CULL_BACK : CULL_FRONT);

	if(m_useCustomProjMat)
		materials->SetMatrix(MATRIXMODE_PROJECTION, m_customProjMat);

	materials->SetMatrix(MATRIXMODE_WORLD, identity4());

	materials->BindMaterial(m_pMaterial, false);
	materials->Apply();
	
	// draw
	g_pShaderAPI->DrawIndexedPrimitives(PRIM_TRIANGLE_STRIP, 0, m_numIndices, 0, m_numVertices);

	HOOK_TO_CVAR(r_wireframe)

	if(r_wireframe->GetBool())
	{
		materials->SetRasterizerStates(CULL_FRONT, FILL_WIREFRAME);

		materials->SetDepthStates(false,false);

		static IShaderProgram* flat = g_pShaderAPI->FindShaderProgram("DefaultFlatColor");

		g_pShaderAPI->Reset(STATE_RESET_SHADER);
		g_pShaderAPI->SetShader(flat);
		g_pShaderAPI->Apply();

		g_pShaderAPI->DrawIndexedPrimitives(PRIM_TRIANGLE_STRIP, 0, m_numIndices, 0, m_numVertices);
	}

	if(!(nViewRenderFlags & EPRFLAG_DONT_FLUSHBUFFERS))
	{
		m_numVertices = 0;
		m_numIndices = 0;
		m_useCustomProjMat = false;
	}
}

//----------------------------------------------------------------------------------------------------------

CPFXAtlasGroup::CPFXAtlasGroup() : CParticleRenderGroup(), CTextureAtlas()
{

}

void CPFXAtlasGroup::Init( const char* pszMaterialName, bool bCreateOwnVBO, int maxQuads )
{
	if( CTextureAtlas::Load(pszMaterialName, pszMaterialName) )
		CParticleRenderGroup::Init( m_material.GetData(), bCreateOwnVBO, maxQuads);
}

void CPFXAtlasGroup::Shutdown()
{
	CParticleRenderGroup::Shutdown();
	CTextureAtlas::Cleanup();
}

//----------------------------------------------------------------------------------------------------------

CParticleLowLevelRenderer::CParticleLowLevelRenderer() : m_mutex(GetGlobalMutex(MUTEXPURPOSE_RENDERER))
{
	m_vertexBuffer = NULL;
	m_indexBuffer = NULL;
	m_vertexFormat = NULL;

	m_initialized = false;
}

void CParticleLowLevelRenderer::PreloadCache()
{

}

void CParticleLowLevelRenderer::ClearParticleCache()
{
	for(int i = 0; i < m_renderGroups.numElem(); i++)
	{
		m_renderGroups[i]->Shutdown();

		delete m_renderGroups[i];
	}

	m_renderGroups.clear();
}

bool MatSysFn_InitParticleBuffers();
bool MatSysFn_ShutdownParticleBuffers();

void CParticleLowLevelRenderer::Init()
{
	materials->AddDestroyLostCallbacks(MatSysFn_ShutdownParticleBuffers, MatSysFn_InitParticleBuffers);

	InitBuffers();
}

void CParticleLowLevelRenderer::Shutdown()
{
	materials->RemoveLostRestoreCallbacks(MatSysFn_ShutdownParticleBuffers, MatSysFn_InitParticleBuffers);

	ShutdownBuffers();
}

bool CParticleLowLevelRenderer::InitBuffers()
{
	if(m_initialized)
		return true;

	MsgWarning("Initializing particle buffers...\n");

	m_vbMaxQuads = r_particleBufferSize.GetInt();

	if(!m_vertexBuffer)
	{
		void* tmpVert = malloc(PVBO_MAX_SIZE(m_vbMaxQuads));

		m_vertexBuffer = g_pShaderAPI->CreateVertexBuffer(BUFFER_DYNAMIC, m_vbMaxQuads*4, sizeof(PFXVertex_t), tmpVert);
		free(tmpVert);
	}

	if(!m_indexBuffer)
	{
		void* tmpIdx = malloc(PIBO_MAX_SIZE(m_vbMaxQuads));

		m_indexBuffer = g_pShaderAPI->CreateIndexBuffer(m_vbMaxQuads*6, sizeof(int16), BUFFER_DYNAMIC, tmpIdx);
		free(tmpIdx);
	}

	if(!m_vertexFormat)
	{
		VertexFormatDesc_s format[] = {
			{ 0, 3, VERTEXTYPE_VERTEX,		ATTRIBUTEFORMAT_FLOAT },	// vertex position
			{ 0, 2, VERTEXTYPE_TEXCOORD,	ATTRIBUTEFORMAT_HALF },	// vertex texture coord
			{ 0, 4, VERTEXTYPE_TEXCOORD,	ATTRIBUTEFORMAT_HALF },	// vertex color
			//{ 0, 4, VERTEXTYPE_TEXCOORD,	ATTRIBUTEFORMAT_HALF },	// vertex normal
		};

		m_vertexFormat = g_pShaderAPI->CreateVertexFormat(format, elementsOf(format));
	}

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

	m_vertexBuffer = NULL;
	m_indexBuffer = NULL;
	m_vertexFormat = NULL;

	m_initialized = false;

	return true;
}

void CParticleLowLevelRenderer::AddRenderGroup(CParticleRenderGroup* pRenderGroup)
{
	m_renderGroups.append(pRenderGroup);
}

void CParticleLowLevelRenderer::RemoveRenderGroup(CParticleRenderGroup* pRenderGroup)
{
	m_renderGroups.remove(pRenderGroup);
}

void CParticleLowLevelRenderer::PreloadMaterials()
{
	for(int i = 0; i < m_renderGroups.numElem(); i++)
	{
		materials->PutMaterialToLoadingQueue(m_renderGroups[i]->m_pMaterial);
	}
}

// prepares render buffers and sends renderables to ViewRenderer
void CParticleLowLevelRenderer::Render(int nRenderFlags)
{
#ifndef NO_ENGINE
	for(int i = 0; i < m_renderGroups.numElem(); i++)
	{
		// add to view renderer
	}
#else
	for(int i = 0; i < m_renderGroups.numElem(); i++)
	{
		m_renderGroups[i]->Render( nRenderFlags );
	}
#endif
}

bool CParticleLowLevelRenderer::MakeVBOFrom(CParticleRenderGroup* pGroup)
{
	if(!m_initialized)
		return false;

	int nVerts		= pGroup->m_numVertices;
	int nIndices	= pGroup->m_numIndices;

	if(nVerts == 0 || nIndices == 0)
		return false;
	
	if(nVerts*sizeof(PFXVertex_t)*4 > PVBO_MAX_SIZE(m_vbMaxQuads))
		return false;

	PFXVertex_t* copyVerts = NULL;
	if(m_vertexBuffer->Lock(0, nVerts, (void**)&copyVerts, false))
	{
		if(!copyVerts)
			return false;

		memcpy(copyVerts, pGroup->m_pVerts ,sizeof(PFXVertex_t)*nVerts);
		m_vertexBuffer->Unlock();
	}

	int16* copyIndices = NULL;
	if(m_indexBuffer->Lock(0, nIndices, (void**)&copyIndices, false))
	{
		if(!copyIndices)
			return false;

		memcpy(copyIndices, pGroup->m_pIndices, sizeof(int16)*nIndices);
		m_indexBuffer->Unlock();
	}

	return true;
}

//----------------------------------------------------------------------------------------------------

bool MatSysFn_InitParticleBuffers()
{
	return g_pPFXRenderer->InitBuffers();
}

bool MatSysFn_ShutdownParticleBuffers()
{
	return g_pPFXRenderer->ShutdownBuffers();
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
	if(effect->group->AllocateGeom(4,4,&verts, NULL, true) < 0)
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

	AngleVectors(angles, NULL, &vRight, &vUp);

	Rectangle_t texCoords(0,0,1,1);

	if(effect->tex)
		texCoords = effect->tex->rect;
	
	verts[0].point = effect->vOrigin + (vUp * effect->fTall) + (effect->fWide * vRight);
	verts[0].texcoord = Vector2D(texCoords.vrightBottom.x, texCoords.vrightBottom.y);
	verts[0].color = effect->vColor;

	verts[1].point = effect->vOrigin + (vUp * effect->fTall) - (effect->fWide * vRight);
	verts[1].texcoord = Vector2D(texCoords.vrightBottom.x, texCoords.vleftTop.y);
	verts[1].color = effect->vColor;

	verts[2].point = effect->vOrigin - (vUp * effect->fTall) + (effect->fWide * vRight);
	verts[2].texcoord = Vector2D(texCoords.vleftTop.x, texCoords.vrightBottom.y);
	verts[2].color = effect->vColor;

	verts[3].point = effect->vOrigin - (vUp * effect->fTall) - (effect->fWide * vRight);
	verts[3].texcoord = Vector2D(texCoords.vleftTop.x, texCoords.vleftTop.y);
	verts[3].color = effect->vColor;
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
	if(effect->group->AllocateGeom(4,4,&verts, NULL, true) < 0)
		return;

	Vector3D vRight, vUp;

	vRight = viewMatrix.rows[0].xyz();
	vUp = viewMatrix.rows[1].xyz();

	Rectangle_t texCoords(0,0,1,1);

	if(effect->tex)
		texCoords = effect->tex->rect;
	
	verts[0].point = effect->vOrigin + (vUp * effect->fTall) + (effect->fWide * vRight);
	verts[0].texcoord = Vector2D(texCoords.vrightBottom.x, texCoords.vrightBottom.y);
	verts[0].color = effect->vColor;

	verts[1].point = effect->vOrigin + (vUp * effect->fTall) - (effect->fWide * vRight);
	verts[1].texcoord = Vector2D(texCoords.vrightBottom.x, texCoords.vleftTop.y);
	verts[1].color = effect->vColor;

	verts[2].point = effect->vOrigin - (vUp * effect->fTall) + (effect->fWide * vRight);
	verts[2].texcoord = Vector2D(texCoords.vleftTop.x, texCoords.vrightBottom.y);
	verts[2].color = effect->vColor;

	verts[3].point = effect->vOrigin - (vUp * effect->fTall) - (effect->fWide * vRight);
	verts[3].texcoord = Vector2D(texCoords.vleftTop.x, texCoords.vleftTop.y);
	verts[3].color = effect->vColor;
}
