//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Driver Syndicate shadow renderer
//////////////////////////////////////////////////////////////////////////////////

#include "ShadowRenderer.h"
#include "ConVar.h"
#include "world.h"

ConVar r_shadowAtlasSize("r_shadowAtlasSize", "1024", 256.0f,  1024.0f, NULL, CV_ARCHIVE);
ConVar r_shadowLod("r_shadowLod", "0", NULL, CV_ARCHIVE);

ConVar r_shadows("r_shadows", "1", NULL, CV_ARCHIVE);

ConVar r_shadowDist("r_shadowDist", "50", NULL, CV_ARCHIVE);

const float SHADOW_SCALING = 24.0f;
const float SHADOW_DESCALING = 1.0f / SHADOW_SCALING;
const float SHADOW_CROP = 4.0f;

enum EShadowModelRenderMode
{
	RSHADOW_STANDARD = 0,
	RSHADOW_CAR,
	RSHADOW_SKIN,
};

CShadowRenderer::CShadowRenderer() : m_shadowTexture(NULL), m_shadowAngles(90,0,0), m_matVehicle(NULL), m_matSkinned(NULL), m_matSimple(NULL), m_isInit(false)
{
	m_texAtlasPacker.SetPackPadding( 2.0 );
}

CShadowRenderer::~CShadowRenderer()
{
}

void CShadowRenderer::Init()
{
	if(m_isInit)
		return;

	m_shadowTextureSize = r_shadowAtlasSize.GetFloat();
	m_shadowTexelSize = 1.0f / m_shadowTextureSize;

	// create shadow render target
	m_shadowTexture = g_pShaderAPI->CreateNamedRenderTarget("_dshadow", m_shadowTextureSize.x, m_shadowTextureSize.y, FORMAT_R8);
	m_shadowTexture->Ref_Grab();

	// shadow render target for rendering and blitting
	m_shadowRt = g_pShaderAPI->CreateNamedRenderTarget("_rt_shadow", 256, 256, FORMAT_R8);
	m_shadowRt->Ref_Grab();

	m_matVehicle = materials->FindMaterial("engine/shadowbuild_vehicle");
	m_matVehicle->Ref_Grab();

	m_matSkinned = materials->FindMaterial("engine/shadowbuild_skin");
	m_matSkinned->Ref_Grab();

	m_matSimple = materials->FindMaterial("engine/shadowbuild");
	m_matSimple->Ref_Grab();

	m_shadowResult = materials->FindMaterial("engine/shadow");
	m_shadowResult->Ref_Grab();

	CSpriteBuilder::Init();

	// sprite builder to triangle mode
	m_triangleMode = true;

	m_isInit = true;
}

void CShadowRenderer::Shutdown()
{
	if(!m_isInit)
		return;

	Clear();

	g_pShaderAPI->FreeTexture(m_shadowTexture);
	m_shadowTexture = NULL;

	g_pShaderAPI->FreeTexture(m_shadowRt);
	m_shadowRt = NULL;

	materials->FreeMaterial(m_matVehicle);
	materials->FreeMaterial(m_matSkinned);
	materials->FreeMaterial(m_matSimple);
	materials->FreeMaterial(m_shadowResult);

	CSpriteBuilder::Shutdown();

	m_isInit = false;
}

void CShadowRenderer::AddShadowCaster( CGameObject* object )
{
	if(r_shadows.GetBool() == false)
		return;

	if(object->GetModel() == NULL) // egf model required
		return;

	// only cars, and physics (incl debris)
	if(!(object->ObjType() == GO_CAR || object->ObjType() == GO_CAR_AI || object->ObjType() == GO_PHYSICS || object->ObjType() == GO_DEBRIS))
	//if(!(object->ObjType() == GO_LIGHT_TRAFFIC))
		return;

	Vector3D viewPos = g_pGameWorld->GetView()->GetOrigin();
	if(length(viewPos-object->GetOrigin()) > r_shadowDist.GetFloat())
		return;

	float shadowSize = length(object->GetModel()->GetAABB().GetSize()) * SHADOW_SCALING;

	m_texAtlasPacker.AddRectangle(shadowSize, shadowSize, object);
}

void CShadowRenderer::Clear()
{
	m_texAtlasPacker.Cleanup();
	CSpriteBuilder::ClearBuffers();
}

inline int AtlasPackComparison(PackerRectangle *const &elem0, PackerRectangle *const &elem1)
{
	return (elem1->width + elem1->height) - (elem0->width + elem0->height);
}

ConVar r_shadows_debugatlas("r_shadows_debugatlas", "0");

void CShadowRenderer::RenderShadowCasters()
{
	if(r_shadows.GetBool() == false)
		return;

	Vector2D neededTexSize = m_shadowTextureSize;

	// process 
	if(!m_texAtlasPacker.AssignCoords(neededTexSize.x,neededTexSize.y)) //, AtlasPackComparison))
	{
		debugoverlay->Text(ColorRGBA(1,0,0,1), "shadows render overflow");
		return; // don't render shadows, size overflow
	}

	Matrix4x4 proj, view, viewProj;
	decalprimitives_t shadowDecal;
	Volume shadowVolume;
	CViewParams orthoView;

	orthoView.SetFOV(0.5f);
	orthoView.SetAngles(Vector3D(m_shadowAngles.x,m_shadowAngles.y,m_shadowAngles.z));

	debugoverlay->Text(ColorRGBA(1), "shadows: %d", m_texAtlasPacker.GetRectangleCount());

	g_pShaderAPI->Reset(STATE_RESET_VBO);
	g_pShaderAPI->ChangeVertexBuffer( NULL, 2 );

	g_pShaderAPI->ChangeRenderTarget( m_shadowTexture );

	// render shadow to the rt
	g_pShaderAPI->Clear( true,false,false, ColorRGBA(1.0f) );

	g_pShaderAPI->ChangeRenderTarget( m_shadowRt );

	Vector2D halfTexSizeNeg = m_shadowTextureSize*-1.0f*SHADOW_DESCALING;

	const Vector3D& viewPos = g_pGameWorld->GetView()->GetOrigin();
	float distFac = 1.0f / r_shadowDist.GetFloat();

	for(int i = 0; i < m_texAtlasPacker.GetRectangleCount(); i++)
	{
		void* userData;
		Rectangle_t shadowRect;
		m_texAtlasPacker.GetRectangle(shadowRect, &userData, i);

		CGameObject* object = (CGameObject*)userData;

		if(!object) // bad
			continue;

		// render shadow to the rt
		if(r_shadows_debugatlas.GetBool())
			g_pShaderAPI->Clear( true,false,false, ColorRGBA((float)i / (float)m_texAtlasPacker.GetRectangleCount()) );
		else
			g_pShaderAPI->Clear( true,false,false, ColorRGBA(1.0f) );

		// calculate view
		Vector2D shadowPos = shadowRect.vleftTop*SHADOW_DESCALING;
		Vector2D shadowSize = shadowRect.GetSize()-SHADOW_CROP;

		IRectangle copyRect(shadowRect.vleftTop, shadowRect.vrightBottom);

		shadowRect.vleftTop *= m_shadowTexelSize;
		shadowRect.vrightBottom *= m_shadowTexelSize;

		// move view to the object origin
		orthoView.SetOrigin(object->GetOrigin());
		orthoView.GetMatrices(proj, view, shadowSize.x*SHADOW_DESCALING, shadowSize.y*SHADOW_DESCALING, -shadowSize.x*0.1f, 100.0f, true );
		
		materials->SetMatrix(MATRIXMODE_PROJECTION, proj);
		materials->SetMatrix(MATRIXMODE_VIEW, view);
		materials->SetMatrix(MATRIXMODE_WORLD, object->m_worldMatrix);

		viewProj = proj * view;

		if(!r_shadows_debugatlas.GetBool())
			RenderShadow( object->GetModel(), object->GetBodyGroups(), RSHADOW_STANDARD);

		g_pShaderAPI->CopyRendertargetToTexture(m_shadowRt, m_shadowTexture, NULL, &copyRect);
		
		// make shadow volume and get our shadow polygons from world
		shadowVolume.LoadAsFrustum( viewProj );
		g_pGameWorld->m_level.GetDecalPolygons(shadowDecal, shadowVolume);

		float shadowAlpha = length(orthoView.GetOrigin()-viewPos) * distFac;
		shadowAlpha = 1.0f - pow(max(0.0f, shadowAlpha), 8.0f);

		// clip decal polygons by volume and apply projection coords
		DecalClipAndTexture(&shadowDecal, shadowVolume, viewProj, shadowRect, shadowAlpha);

		// push geometry
		PFXVertex_t* verts;
		int startIdx = AllocateGeom(shadowDecal.verts.numElem(), 0, &verts, NULL, false);

		if(startIdx != -1)
		{
			memcpy(verts, shadowDecal.verts.ptr(), shadowDecal.verts.numElem()*sizeof(PFXVertex_t));
		}
	}

	g_pShaderAPI->ChangeRenderTargetToBackBuffer();
}

void CShadowRenderer::Draw()
{
	if(r_shadows.GetBool() == false)
		return;

	// upload to vertex & index buffers
	if(!g_pPFXRenderer->MakeVBOFrom(this))
		return;

	g_pShaderAPI->Reset(STATE_RESET_VBO);

	g_pShaderAPI->SetVertexFormat(g_pPFXRenderer->m_vertexFormat);
	g_pShaderAPI->SetVertexBuffer(g_pPFXRenderer->m_vertexBuffer, 0);
	//g_pShaderAPI->SetIndexBuffer(g_pPFXRenderer->m_indexBuffer);

	// render
	materials->SetCullMode( CULL_BACK );

	materials->BindMaterial(m_shadowResult, false);
	materials->Apply();

	// draw
	g_pShaderAPI->DrawNonIndexedPrimitives(PRIM_TRIANGLES, 0, m_numVertices);

	HOOK_TO_CVAR(r_wireframe)

	if(r_wireframe->GetBool())
	{
		materials->SetRasterizerStates(CULL_BACK, FILL_WIREFRAME);

		materials->SetDepthStates(false,false);

		static IShaderProgram* flat = g_pShaderAPI->FindShaderProgram("DefaultFlatColor");

		g_pShaderAPI->Reset(STATE_RESET_SHADER);
		g_pShaderAPI->SetShader(flat);
		g_pShaderAPI->Apply();

		g_pShaderAPI->DrawNonIndexedPrimitives(PRIM_TRIANGLES, 0, m_numVertices);
	}

	g_pShaderAPI->Reset();
}

void CShadowRenderer::SetShadowAngles( const Vector3D& angles )
{
	m_shadowAngles = angles;
}

void CShadowRenderer::RenderShadow(IEqModel* model, ubyte bodyGroups, int mode)
{
	IMaterial* rtMaterial = m_matSimple;

	if(mode == RSHADOW_CAR)
		rtMaterial = m_matVehicle;
	else if(mode == RSHADOW_SKIN)
		rtMaterial = m_matSkinned;

	materials->SetCullMode(CULL_FRONT);

	studiohdr_t* pHdr = model->GetHWData()->pStudioHdr;

	int nLOD = r_shadowLod.GetInt();

	for(int i = 0; i < pHdr->numbodygroups; i++)
	{
		if(!(bodyGroups & (1 << i)))
			continue;

		int bodyGroupLOD = nLOD;

		// TODO: check bodygroups for rendering

		int nLodModelIdx = pHdr->pBodyGroups(i)->lodmodel_index;
		int nModDescId = pHdr->pLodModel(nLodModelIdx)->lodmodels[ bodyGroupLOD ];

		// get the right LOD model number
		while(nModDescId == -1 && bodyGroupLOD > 0)
		{
			bodyGroupLOD--;
			nModDescId = pHdr->pLodModel(nLodModelIdx)->lodmodels[ bodyGroupLOD ];
		}

		if(nModDescId == -1)
			continue;

		// render model groups that in this body group
		for(int j = 0; j < pHdr->pModelDesc(nModDescId)->numgroups; j++)
		{
			//IMaterial* pMaterial = model->GetMaterial(nModDescId, j);

			// FIXME:	pass additional information into that material
			//			like transparency
			materials->BindMaterial(rtMaterial, false);

			model->DrawGroup( nModDescId, j );
		}
	}
}