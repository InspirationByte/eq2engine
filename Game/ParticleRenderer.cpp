//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Particle system
//////////////////////////////////////////////////////////////////////////////////

#include "ParticleRenderer.h"
#include "IDebugOverlay.h"

#ifndef NO_ENGINE
#include "IEngineWorld.h"

#include "GameRenderer.h"

#include "BaseEntity.h"
extern BaseEntity* g_pViewEntity;
#endif // NO_ENGINE

//----------------------------------------------------------------------------------------------------

void Effects_DrawBillboard(PFXBillboard_t* effect, CViewParams* view, Volume* frustum)
{
	/*
	FIXME: May be room visibility?

	// speedup for most cases
	if(!(effect->nFlags & EFFECT_FLAG_ALWAYS_VISIBLE))
	{
		if(!viewrenderer->PointIsVisible(effect->vOrigin))
			return;
	}
	*/
	
	if(!(effect->nFlags & EFFECT_FLAG_NO_FRUSTUM_CHECK))
	{
		float fBillboardSize = (effect->fWide > effect->fTall) ? effect->fWide : effect->fTall;

		if(!frustum->IsSphereInside(effect->vOrigin, fBillboardSize))
			return;
	}

	Vector3D angles, vRight, vUp;

	if(effect->nFlags & EFFECT_FLAG_RADIAL_ALIGNING)
		angles = VectorAngles(normalize(effect->vOrigin - view->GetOrigin()));
	else
		angles = view->GetAngles() + Vector3D(0,0,effect->fZAngle);

	if(effect->nFlags & EFFECT_FLAG_LOCK_X)
		angles.x = 0;

	if(effect->nFlags & EFFECT_FLAG_LOCK_Y)
		angles.y = 0;

	AngleVectors(angles, NULL, &vRight, &vUp);

	ParticleVertex_t verts[4];

	verts[0].point = effect->vOrigin + (vUp * effect->fTall) + (effect->fWide * vRight);
	verts[0].texcoord = Vector2D(1,1);
	verts[0].color = effect->vColor;

	verts[1].point = effect->vOrigin + (vUp * effect->fTall) - (effect->fWide * vRight);
	verts[1].texcoord = Vector2D(1,0);
	verts[1].color = effect->vColor;

	verts[2].point = effect->vOrigin - (vUp * effect->fTall) + (effect->fWide * vRight);
	verts[2].texcoord = Vector2D(0,1);
	verts[2].color = effect->vColor;

	verts[3].point = effect->vOrigin - (vUp * effect->fTall) - (effect->fWide * vRight);
	verts[3].texcoord = Vector2D(0,0);
	verts[3].color = effect->vColor;

	AddParticleQuad(verts, effect->nGroupIndex);
}

//----------------------------------------------------------------------------------------------------

HOOK_TO_CVAR(r_wireframe)

// optimized particle groups
struct particleOptimizedGroup_t
{
	ParticleVertex_t*			s_ParticleVerts;
	uint16*						s_ParticleIndices;

	void AddIndex(uint16 idx)
	{
		s_ParticleIndices[nIndices] = idx;
		nIndices++;
	}

	void AddVertex(ParticleVertex_t &vert)
	{
		s_ParticleVerts[nVertices] = vert;
		nVertices++;
	}

	void AddVertices(ParticleVertex_t *verts, int nVerts)
	{
		memcpy(&s_ParticleVerts[nVertices], verts, nVerts*sizeof(ParticleVertex_t));
		nVertices += nVerts;
	}

#ifndef NO_ENGINE
	void AddVertices2(eqlevelvertex_t *verts, int nVerts)
	{
		eqlevelvertex_t* pPartVerts = (eqlevelvertex_t*)s_ParticleVerts;

		memcpy(&pPartVerts[nVertices], verts, nVerts*sizeof(eqlevelvertex_t));
		nVertices += nVerts;
	}
#endif // NO_ENGINE

	void AddIndices(uint16 *indices, int nIndx)
	{
		memcpy(&s_ParticleIndices[nIndices], indices, nIndx*sizeof(uint16));
		nIndices += nIndx;
	}

	IMaterial*					pMaterial;

	int							nStartIndex;

	int							nVertices;
	int							nIndices;

	bool						isDecal;
	bool						isPostRender;
	bool						useNormalMaterialMap;
};

static DkList<particleOptimizedGroup_t> s_ParticleOptimizedGroups;

int GetParticleIndexCount(int groupMaterialIndex)
{
	return s_ParticleOptimizedGroups[groupMaterialIndex].nIndices;
}

void ClearParticleCache()
{
	for(int i = 0; i < s_ParticleOptimizedGroups.numElem(); i++)
	{
		free(s_ParticleOptimizedGroups[i].s_ParticleVerts);
		free(s_ParticleOptimizedGroups[i].s_ParticleIndices);

		materials->FreeMaterial(s_ParticleOptimizedGroups[i].pMaterial);
	}

	s_ParticleOptimizedGroups.clear();
}

// precaches new material. Returns index. isPostRender parameter helps with viewmodel effects
int PrecacheParticleMaterial(const char* materialname, bool isPostRender, bool isDecal)
{
	IMaterial* pMat = NULL;

	for(int i = 0; i < s_ParticleOptimizedGroups.numElem(); i++)
	{
		if(!stricmp(s_ParticleOptimizedGroups[i].pMaterial->GetName(), materialname))
		{
			if(s_ParticleOptimizedGroups[i].isPostRender == isPostRender)
			{
				if(!stricmp(s_ParticleOptimizedGroups[i].pMaterial->GetName(), materialname))
					return i;
			}
			else
			{
				pMat = s_ParticleOptimizedGroups[i].pMaterial;
			}
		}
	}

	particleOptimizedGroup_t grp;

#ifndef NO_ENGINE
	if(isDecal)
		grp.s_ParticleVerts		= (ParticleVertex_t*)malloc(DVBO_MAX_SIZE);
	else
#endif // NO_ENGINE
		grp.s_ParticleVerts		= (ParticleVertex_t*)malloc(PVBO_MAX_SIZE);

	grp.s_ParticleIndices	= (uint16*)malloc(PIBO_MAX_SIZE);

	if(!pMat)
		pMat = materials->FindMaterial(materialname);

	pMat->LoadShaderAndTextures();

#ifndef NO_ENGINE
	if(gpGlobals->curtime > 0)
		MsgWarning("WARNING! Late precache of material '%s'\n", materialname);
#endif // NO_ENGINE

	pMat->Ref_Grab();

	grp.nIndices = 0;
	grp.nStartIndex = 0;
	grp.pMaterial = pMat;
	grp.isDecal = isDecal;
	grp.isPostRender = isPostRender;
	grp.useNormalMaterialMap = false;

	IMatVar* pVar = pMat->FindMaterialVar("bumpmap");
	
	if(pVar)
	{
		grp.useNormalMaterialMap = true;
	}

	int idx = s_ParticleOptimizedGroups.numElem();
	
	s_ParticleOptimizedGroups.append(grp);

	return idx;
}

// Precache particle
int PrecacheAddMaterial(IMaterial* pMaterial, bool isPostRender, bool isDecal)
{
	IMaterial* pMat = NULL;

	for(int i = 0; i < s_ParticleOptimizedGroups.numElem(); i++)
	{
		if(!stricmp(s_ParticleOptimizedGroups[i].pMaterial->GetName(), pMaterial->GetName()))
		{
			if(s_ParticleOptimizedGroups[i].isPostRender == isPostRender)
			{
				if(!stricmp(s_ParticleOptimizedGroups[i].pMaterial->GetName(), pMaterial->GetName()))
					return i;
			}
			else
			{
				pMat = s_ParticleOptimizedGroups[i].pMaterial;
			}
		}
	}

	particleOptimizedGroup_t grp;

	grp.s_ParticleVerts =	(ParticleVertex_t*)malloc(PVBO_MAX_SIZE);
	grp.s_ParticleIndices = (uint16*)malloc(PIBO_MAX_SIZE);

	if(!pMat)
	{
		pMat = pMaterial;
	}

	pMat->Ref_Grab();

	grp.nIndices = 0;
	grp.nStartIndex = 0;
	grp.pMaterial = pMat;
	grp.isDecal = isDecal;
	grp.isPostRender = isPostRender;
	grp.useNormalMaterialMap = false;

	IMatVar* pVar = pMat->FindMaterialVar("bumpmap");
	
	if(pVar)
	{
		grp.useNormalMaterialMap = true;
	}

	int idx = s_ParticleOptimizedGroups.numElem();
	
	s_ParticleOptimizedGroups.append(grp);

	return idx;
}

void AddParticleGeom(ParticleVertex_t *fourVerts, uint16 *fourIndices, int nVertices, int nIndices, int groupMaterialIndex)
{
	if(s_ParticleOptimizedGroups[groupMaterialIndex].nVertices*sizeof(ParticleVertex_t)*4 > PVBO_MAX_SIZE)
		return;

	int num_ind = s_ParticleOptimizedGroups[groupMaterialIndex].nIndices;

	uint16 nIndicesCurr = 0;

	// if it's a second, first I'll add last index (3 if first, and add first one from fourIndices)
	if(num_ind > 0)
	{
		int lastIdx = s_ParticleOptimizedGroups[groupMaterialIndex].s_ParticleIndices[s_ParticleOptimizedGroups[groupMaterialIndex].nIndices-1];

		nIndicesCurr = lastIdx+1;

		// add two last indices to make degenerates
		uint16 degenerate[2] = {lastIdx, nIndicesCurr};

		s_ParticleOptimizedGroups[groupMaterialIndex].AddIndices(degenerate, 2);
	}

	s_ParticleOptimizedGroups[groupMaterialIndex].AddVertices(fourVerts, nVertices);

	for(register int i = 0; i < nIndices; i++)
		fourIndices[i] += nIndicesCurr;

	s_ParticleOptimizedGroups[groupMaterialIndex].AddIndices(fourIndices, nIndices);
}

void AddParticleQuad(ParticleVertex_t fourVerts[4], int groupMaterialIndex)
{
	uint16 indexArray[4] = {0,1,2,3};

	AddParticleGeom(fourVerts, indexArray, 4, 4, groupMaterialIndex );

	/*
	if(s_ParticleOptimizedGroups[groupMaterialIndex].nVertices*sizeof(ParticleVertex_t)*4 > PVBO_MAX_SIZE)
		return;

	int lastIdx = s_ParticleOptimizedGroups[groupMaterialIndex].s_ParticleIndices[s_ParticleOptimizedGroups[groupMaterialIndex].nIndices-1];

	uint16 nIndicesCurr = (s_ParticleOptimizedGroups[groupMaterialIndex].nIndices ? lastIdx+1 : 0);

	// if it's a second, first I'll add last index (3 if first, and add first one from fourIndices)
	if(nIndicesCurr > 0)
	{
		// add two last indices to make degenerates
		uint16 degenerate[2] = {lastIdx, nIndicesCurr};

		s_ParticleOptimizedGroups[groupMaterialIndex].AddIndices(degenerate, 2);
	}

	s_ParticleOptimizedGroups[groupMaterialIndex].AddVertices(fourVerts, 4);

	uint16 indices[] = {nIndicesCurr, nIndicesCurr+1, nIndicesCurr+2, nIndicesCurr+3};

	s_ParticleOptimizedGroups[groupMaterialIndex].AddIndices(indices, 4);
	*/
}

// 1 kb
static uint16 g_TempIDXList[1024];

#ifndef NO_ENGINE

// adds geometry to particle buffers, but as decal
void AddDecalGeom(eqlevelvertex_t *verts, uint16 *indices, int nVertices, int nIndices, int groupMaterialIndex)
{
	if(s_ParticleOptimizedGroups[groupMaterialIndex].nVertices*sizeof(eqlevelvertex_t)*4 > PVBO_MAX_SIZE)
		return;

	int num_ind = s_ParticleOptimizedGroups[groupMaterialIndex].nIndices;

	int16 nIndicesCurr = 0;

	if(num_ind > 0)
	{
		nIndicesCurr = s_ParticleOptimizedGroups[groupMaterialIndex].nVertices; //s_ParticleOptimizedGroups[groupMaterialIndex].s_ParticleIndices[s_ParticleOptimizedGroups[groupMaterialIndex].nIndices-1]+1;
	}

	for(int i = 0; i < nIndices; i++)
		g_TempIDXList[i] = indices[i] + nIndicesCurr;

	s_ParticleOptimizedGroups[groupMaterialIndex].AddVertices2(verts, nVertices);
	s_ParticleOptimizedGroups[groupMaterialIndex].AddIndices(g_TempIDXList, nIndices);
}

#endif // NO_ENGINE

IVertexBuffer*		g_pDecalVB		= NULL;

IVertexBuffer*		g_pParticleVB	= NULL;
IIndexBuffer*		g_pParticleIB	= NULL;
IVertexFormat*		g_pParticleVF	= NULL;

bool				g_bParticlesInit = false;

bool InitParticleBuffers()
{
	if(g_bParticlesInit)
		return true;

	MsgWarning("Initializing particle buffers...\n");

	bool onCreate = false;

	if(!g_pParticleVB)
	{
		void* tmpVert = malloc(PVBO_MAX_SIZE);
		onCreate = true;
		g_pParticleVB = g_pShaderAPI->CreateVertexBuffer(BUFFER_DYNAMIC, MAX_PARTICLES, sizeof(ParticleVertex_t), tmpVert);
		free(tmpVert);
	}

#ifndef NO_ENGINE
	if(!g_pDecalVB)
	{
		void* tmpVert = malloc(DVBO_MAX_SIZE);
		onCreate = true;
		g_pDecalVB = g_pShaderAPI->CreateVertexBuffer(BUFFER_DYNAMIC, MAX_PARTICLES, sizeof(eqlevelvertex_t), tmpVert);
		free(tmpVert);
	}
#endif // NO_ENGINE
	if(!g_pParticleIB)
	{
		void* tmpIdx = malloc(PIBO_MAX_SIZE);
		onCreate = true;
		g_pParticleIB = g_pShaderAPI->CreateIndexBuffer(MAX_PARTICLES*2, sizeof(int16), BUFFER_DYNAMIC, tmpIdx);
		free(tmpIdx);
	}

	if(!g_pParticleVF)
	{
		VertexFormatDesc_s format[] = {
			{ 0, 3, VERTEXATTRIB_POSITION,		ATTRIBUTEFORMAT_FLOAT },	// vertex position
			{ 0, 2, VERTEXATTRIB_TEXCOORD,	ATTRIBUTEFORMAT_FLOAT },	// vertex texture coord
			{ 0, 4, VERTEXATTRIB_TEXCOORD,	ATTRIBUTEFORMAT_FLOAT },	// vertex color
			{ 0, 3, VERTEXATTRIB_TEXCOORD,	ATTRIBUTEFORMAT_FLOAT },	// vertex normal
		};

		g_pParticleVF = g_pShaderAPI->CreateVertexFormat(format, elementsOf(format));
	}

	if(g_pParticleIB && g_pParticleVB && g_pParticleVF && g_pDecalVB)
		g_bParticlesInit = true;

	return g_bParticlesInit;
}

bool ShutdownParticleBuffers()
{
	if(!g_bParticlesInit)
		return true;

	MsgWarning("Destroying particle buffers...\n");

	g_pShaderAPI->DestroyVertexFormat(g_pParticleVF);
	g_pShaderAPI->DestroyIndexBuffer(g_pParticleIB);
	g_pShaderAPI->DestroyVertexBuffer(g_pParticleVB);
	g_pShaderAPI->DestroyVertexBuffer(g_pDecalVB);

	g_pDecalVB = NULL;
	g_pParticleVB = NULL;
	g_pParticleIB = NULL;
	g_pParticleVF = NULL;

	g_bParticlesInit = false;

	return true;
}

bool MakeGroupVBOs(int group)
{
	if(!g_bParticlesInit)
		return false;

	int nVerts		= s_ParticleOptimizedGroups[group].nVertices;
	int nIndices	= s_ParticleOptimizedGroups[group].nIndices;

	if(nVerts == 0 || nIndices == 0)
		return false;
	
#ifndef NO_ENGINE
	if(s_ParticleOptimizedGroups[group].isDecal)
	{
		if(nVerts*sizeof(eqlevelvertex_t)*4 > DVBO_MAX_SIZE)
		{
			Msg("Decal overflow\n");
			return false;
		}

		debugoverlay->Text(ColorRGBA(1,1,1,1), "decal draw for %d verts (%d>>>%d)\n", nVerts, nVerts*sizeof(eqlevelvertex_t)*4, DVBO_MAX_SIZE);

		eqlevelvertex_t* copyVerts = NULL;
		if(g_pDecalVB->Lock(0, nVerts, (void**)&copyVerts, false))
		{
			if(!copyVerts)
				return false;

			memcpy(copyVerts, s_ParticleOptimizedGroups[group].s_ParticleVerts ,sizeof(eqlevelvertex_t)*nVerts);
			g_pDecalVB->Unlock();
		}
	}
	else
#endif // NO_ENGINE
	{
		if(nVerts*sizeof(ParticleVertex_t)*4 > PVBO_MAX_SIZE)
			return false;

		ParticleVertex_t* copyVerts = NULL;
		if(g_pParticleVB->Lock(0, nVerts, (void**)&copyVerts, false))
		{
			if(!copyVerts)
				return false;

			memcpy(copyVerts, s_ParticleOptimizedGroups[group].s_ParticleVerts ,sizeof(ParticleVertex_t)*nVerts);
			g_pParticleVB->Unlock();
		}
	}

	int16* copyIndices = NULL;
	if(g_pParticleIB->Lock(0, nIndices, (void**)&copyIndices, false))
	{
		if(!copyIndices)
			return false;

		memcpy(copyIndices, s_ParticleOptimizedGroups[group].s_ParticleIndices, sizeof(int16)*nIndices);
		g_pParticleIB->Unlock();
	}

	return true;
}

IMaterial* GetParticleMaterial(int grpID)
{
	return s_ParticleOptimizedGroups[grpID].pMaterial;
}

void DrawDecals(ITexture** pDecalGBuffer)
{
	materials->SetMatrix(MATRIXMODE_WORLD, identity4());

	/*
	for(register int i = 0; i < s_ParticleOptimizedGroups.numElem(); i++)
	{
		if(!s_ParticleOptimizedGroups[i].isDecal)
			continue;

		if(MakeGroupVBOs(i))
		{
			materials->BindMaterial(s_ParticleOptimizedGroups[i].pMaterial, false);

			if(pDecalGBuffer)
			{
				// change to backbuffer again
				g_pShaderAPI->ChangeRenderTargetToBackBuffer();

				int slices[] = {0,0,0,0,0,0,0};

				g_pShaderAPI->ChangeRenderTargets(pDecalGBuffer, (s_ParticleOptimizedGroups[i].useNormalMaterialMap ? 3 : 1), slices);
			}

			g_pShaderAPI->SetVertexFormat(g_pParticleVF);
			g_pShaderAPI->SetVertexBuffer(g_pParticleVB ,0);
			g_pShaderAPI->SetIndexBuffer(g_pParticleIB);

			//g_pShaderAPI->DrawSetColor(ColorRGBA(1));

			if(s_ParticleOptimizedGroups[i].isDecal)
				g_pShaderAPI->ChangeRasterStateEx(CULL_BACK, FILL_SOLID);
			else
				g_pShaderAPI->ChangeRasterStateEx(CULL_FRONT, FILL_SOLID);

			g_pShaderAPI->SetVertexShaderConstantMatrix4(90, viewrenderer->GetViewProjectionMatrix());
			g_pShaderAPI->SetVertexShaderConstantMatrix4(94, identity4());

			//g_pShaderAPI->ChangeDepthStateEx(true, s_ParticleOptimizedGroups[i].isDecal);

			if(s_ParticleOptimizedGroups[i].isDecal)
				g_pShaderAPI->SetDepthRange(0.0f,0.99999f);
			else
				g_pShaderAPI->SetDepthRange(0.0f,1.0f);

			g_pShaderAPI->Apply();

			int nVerts = s_ParticleOptimizedGroups[i].nVertices;
			int nIndices = s_ParticleOptimizedGroups[i].nIndices;

			if(s_ParticleOptimizedGroups[i].isDecal)
				g_pShaderAPI->DrawIndexedPrimitives(PRIM_TRIANGLES, 0, nIndices, 0, nVerts);
			else
				g_pShaderAPI->DrawIndexedPrimitives(PRIM_TRIANGLE_STRIP, 0, nIndices, 0, nVerts);

			if(r_wireframe->GetBool())
			{
				if(s_ParticleOptimizedGroups[i].isDecal)
					g_pShaderAPI->ChangeRasterStateEx(CULL_BACK, FILL_WIREFRAME);
				else
					g_pShaderAPI->ChangeRasterStateEx(CULL_FRONT, FILL_WIREFRAME);

				g_pShaderAPI->ChangeDepthStateEx(false,false);

				static IShaderProgram* flat = g_pShaderAPI->FindShaderProgram("DefaultFlatColor");

				g_pShaderAPI->Reset(STATE_RESET_SHADER);
				g_pShaderAPI->SetShader(flat);
				g_pShaderAPI->Apply();

				g_pShaderAPI->DrawSetColor(Vector4D(0.0f,1.0f,1.0f,1.0f));

				if(s_ParticleOptimizedGroups[i].isDecal)
					g_pShaderAPI->DrawIndexedPrimitives(PRIM_TRIANGLES, 0, nIndices, 0, nVerts);
				else
					g_pShaderAPI->DrawIndexedPrimitives(PRIM_TRIANGLE_STRIP, 0, nIndices, 0, nVerts);
			}

		}
		

		s_ParticleOptimizedGroups[i].nIndices = 0;
		s_ParticleOptimizedGroups[i].nVertices = 0;
	}
	*/
}

static ConVar r_drawParticles("r_drawParticles", "1", "Draw all particles", CV_CHEAT);

void DrawParticleMaterialGroups(bool dsCheck)
{
	for(register int i = 0; i < s_ParticleOptimizedGroups.numElem(); i++)
	{
		if(s_ParticleOptimizedGroups[i].isPostRender)
			continue;
		/*
		if(s_ParticleOptimizedGroups[i].isDecal)
			continue;
			*/

		// if no rendering, reset the index and vertex count
		if(!r_drawParticles.GetBool())
		{
			s_ParticleOptimizedGroups[i].nIndices = 0;
			s_ParticleOptimizedGroups[i].nVertices = 0;
			continue;
		}

		if( MakeGroupVBOs(i) )
		{
			materials->SetMatrix(MATRIXMODE_WORLD, identity4());

			materials->BindMaterial(s_ParticleOptimizedGroups[i].pMaterial, true);

#ifndef NO_ENGINE
			if(s_ParticleOptimizedGroups[i].isDecal)
			{
				g_pShaderAPI->SetVertexFormat(eqlevel->GetDecalVertexFormat());
				g_pShaderAPI->SetVertexBuffer(g_pDecalVB ,0);
			}
			else
#endif // NO_ENGINE
			{
				g_pShaderAPI->SetVertexFormat(g_pParticleVF);
				g_pShaderAPI->SetVertexBuffer(g_pParticleVB ,0);
			}

			g_pShaderAPI->SetIndexBuffer(g_pParticleIB);


			materials->SetRasterizerStates(CULL_FRONT, FILL_SOLID);

			
			if(s_ParticleOptimizedGroups[i].isDecal)
				materials->SetRasterizerStates(CULL_BACK, FILL_SOLID);
			else
				materials->SetRasterizerStates(CULL_FRONT, FILL_SOLID);

			g_pShaderAPI->Apply();

			int nVerts = s_ParticleOptimizedGroups[i].nVertices;
			int nIndices = s_ParticleOptimizedGroups[i].nIndices;

			if(s_ParticleOptimizedGroups[i].isDecal)
				g_pShaderAPI->DrawIndexedPrimitives(PRIM_TRIANGLES, 0, nIndices, 0, nVerts);
			else
				g_pShaderAPI->DrawIndexedPrimitives(PRIM_TRIANGLE_STRIP, 0, nIndices, 0, nVerts);

			if(r_wireframe->GetBool())
			{
				
				if(s_ParticleOptimizedGroups[i].isDecal)
					materials->SetRasterizerStates(CULL_BACK, FILL_WIREFRAME);
				else
					materials->SetRasterizerStates(CULL_FRONT, FILL_WIREFRAME);

				materials->SetDepthStates(false,false);
				

				static IShaderProgram* flat = g_pShaderAPI->FindShaderProgram("DefaultFlatColor");

				g_pShaderAPI->Reset(STATE_RESET_SHADER);
				g_pShaderAPI->SetShader(flat);
				g_pShaderAPI->Apply();

				//g_pShaderAPI->DrawSetColor(Vector4D(0.0f,1.0f,1.0f,1.0f));

				if(s_ParticleOptimizedGroups[i].isDecal)
					g_pShaderAPI->DrawIndexedPrimitives(PRIM_TRIANGLES, 0, nIndices, 0, nVerts);
				else
					g_pShaderAPI->DrawIndexedPrimitives(PRIM_TRIANGLE_STRIP, 0, nIndices, 0, nVerts);
			}

		}

		s_ParticleOptimizedGroups[i].nIndices = 0;
		s_ParticleOptimizedGroups[i].nVertices = 0;
	}

	for(register int i = 0; i < s_ParticleOptimizedGroups.numElem(); i++)
	{
		if(!s_ParticleOptimizedGroups[i].isPostRender)
			continue;
		/*
		if(s_ParticleOptimizedGroups[i].isDecal)
			continue;
			*/

		if(MakeGroupVBOs(i))
		{
			materials->SetMatrix( MATRIXMODE_WORLD, identity4() );

			materials->BindMaterial(s_ParticleOptimizedGroups[i].pMaterial, true);

#ifndef NO_ENGINE
			if(s_ParticleOptimizedGroups[i].isDecal)
			{
				g_pShaderAPI->SetVertexFormat(eqlevel->GetDecalVertexFormat());
				g_pShaderAPI->SetVertexBuffer(g_pDecalVB ,0);
			}
			else
#endif // NO_ENGINE
			{
				g_pShaderAPI->SetVertexFormat(g_pParticleVF);
				g_pShaderAPI->SetVertexBuffer(g_pParticleVB ,0);
			}

			g_pShaderAPI->SetIndexBuffer(g_pParticleIB);

			materials->SetRasterizerStates(CULL_FRONT, FILL_SOLID);

			if(s_ParticleOptimizedGroups[i].isDecal)
				materials->SetRasterizerStates(CULL_BACK, FILL_SOLID);
			else
				materials->SetRasterizerStates(CULL_FRONT, FILL_SOLID);

			g_pShaderAPI->Apply();

			int nVerts = s_ParticleOptimizedGroups[i].nVertices;
			int nIndices = s_ParticleOptimizedGroups[i].nIndices;

			if(s_ParticleOptimizedGroups[i].isDecal)
				g_pShaderAPI->DrawIndexedPrimitives(PRIM_TRIANGLES, 0, nIndices, 0, nVerts);
			else
				g_pShaderAPI->DrawIndexedPrimitives(PRIM_TRIANGLE_STRIP, 0, nIndices, 0, nVerts);
		}

		s_ParticleOptimizedGroups[i].nIndices = 0;
		s_ParticleOptimizedGroups[i].nVertices = 0;
	}
}