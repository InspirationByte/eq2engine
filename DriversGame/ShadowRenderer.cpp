//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Driver Syndicate shadow renderer
//////////////////////////////////////////////////////////////////////////////////

#include "ShadowRenderer.h"
#include "ConVar.h"
#include "world.h"
#include "ParticleEffects.h"

#include "Animating.h"
#include "car.h"

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

struct shadowListObject_t
{
	shadowListObject_t()
	{
		next = NULL;
	}

	CGameObject*		object;
	shadowListObject_t*	next;
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

	if(!r_shadows.GetBool())
		return; // don't init shadows

	const ShaderAPICaps_t& caps = g_pShaderAPI->GetCaps();

	ETextureFormat shadowRendertargetFormat = FORMAT_R8;

	if (!caps.renderTargetFormatsSupported[shadowRendertargetFormat])
		shadowRendertargetFormat = FORMAT_RGB8;

	if (!caps.renderTargetFormatsSupported[shadowRendertargetFormat])
		shadowRendertargetFormat = FORMAT_RGBA8;

	if (!caps.renderTargetFormatsSupported[shadowRendertargetFormat])
	{
		MsgWarning("r_shadows: not supported!\n");
		r_shadows.SetBool(false);
		return;
	}

	m_shadowTextureSize = r_shadowAtlasSize.GetFloat();
	m_shadowTexelSize = 1.0f / m_shadowTextureSize;

	// create shadow render target
	m_shadowTexture = g_pShaderAPI->CreateNamedRenderTarget("_dshadow", m_shadowTextureSize.x, m_shadowTextureSize.y, shadowRendertargetFormat);
	m_shadowTexture->Ref_Grab();

	// shadow render target for rendering and blitting
	//m_shadowRt = g_pShaderAPI->CreateNamedRenderTarget("_rt_shadow", 256, 256, FORMAT_R8);
	//m_shadowRt->Ref_Grab();

	kvkeybase_t shadowBuildParams;
	shadowBuildParams.SetKey("nofog", true);
	shadowBuildParams.SetKey("zwrite", false);
	shadowBuildParams.SetKey("ztest", false);

	{
		shadowBuildParams.SetName("ShadowBuildVehicle");

		m_matVehicle = materials->CreateMaterial("_shadowbuild_vehicle", &shadowBuildParams);
		m_matVehicle->Ref_Grab();

		m_matVehicle->LoadShaderAndTextures();
	}

	{
		shadowBuildParams.SetName("ShadowBuildSkinned");

		m_matSkinned = materials->CreateMaterial("_shadowbuild_skinned", &shadowBuildParams);
		m_matSkinned->Ref_Grab();

		m_matSkinned->LoadShaderAndTextures();
	}

	{
		shadowBuildParams.SetName("ShadowBuild");

		m_matSimple = materials->CreateMaterial("_shadowbuild", &shadowBuildParams);
		m_matSimple->Ref_Grab();

		m_matSimple->LoadShaderAndTextures();
	}

	{
		kvkeybase_t shadowParams;
		shadowParams.SetName("Shadow");

		shadowParams.SetKey("basetexture", "_dshadow");
		shadowParams.SetKey("zwrite", false);
		shadowParams.SetKey("translucent", true);
		shadowParams.SetKey("Decal", true);

		m_shadowResult = materials->CreateMaterial("_dshadow", &shadowParams);
		m_shadowResult->Ref_Grab();

		m_shadowResult->LoadShaderAndTextures();
	}

	CSpriteBuilder::Init();

	// sprite builder to triangle mode
	m_triangleListMode = true;

	m_isInit = true;
}

void CShadowRenderer::Shutdown()
{
	if(!m_isInit)
		return;

	Clear();

	// destroy the unreleased lists as well
	for(int i = 0; i < m_texAtlasPacker.GetRectangleCount(); i++)
	{
		void* userData = m_texAtlasPacker.GetRectangleUserData(i);
		shadowListObject_t* objectGroup = (shadowListObject_t*)userData;

		while(objectGroup)
		{
			shadowListObject_t* prev = objectGroup;
			objectGroup = objectGroup->next;
			delete prev;
		};
	}

	g_pShaderAPI->FreeTexture(m_shadowTexture);
	m_shadowTexture = NULL;

	//g_pShaderAPI->FreeTexture(m_shadowRt);
	//m_shadowRt = NULL;

	materials->FreeMaterial(m_matVehicle);
	materials->FreeMaterial(m_matSkinned);
	materials->FreeMaterial(m_matSimple);
	materials->FreeMaterial(m_shadowResult);

	CSpriteBuilder::Shutdown();

	m_isInit = false;
}

void CShadowRenderer::AddShadowCaster( CGameObject* object, struct shadowListObject_t* addTo )
{
	if(!m_isInit)
		return;

	if(object->GetModel() == NULL) // egf model required
		return;

	// only cars, and physics (incl debris)
	if(!addTo)
	{
		int objectType = object->ObjType();

		if(!(objectType == GO_CAR || objectType == GO_CAR_AI || objectType == GO_PHYSICS || objectType == GO_DEBRIS || objectType == GO_PEDESTRIAN))
			return;

		Vector3D viewPos = g_pGameWorld->GetView()->GetOrigin();
		if(length(viewPos-object->GetOrigin()) > r_shadowDist.GetFloat())
			return;
	}

	shadowListObject_t* casterObject = new shadowListObject_t;
	casterObject->object = object;

	int numShadowCasterObjects = object->GetChildCasterCount();

	for(int i = 0; i < numShadowCasterObjects; i++)
		AddShadowCaster(object->GetChildShadowCaster(i), casterObject);

	if(addTo)
	{
		casterObject->next = addTo->next;
		addTo->next = casterObject;
	}
	else
	{
		float shadowSize = length(object->GetModel()->GetAABB().GetSize()) * SHADOW_SCALING;
		m_texAtlasPacker.AddRectangle(shadowSize, shadowSize, casterObject);
	}
}

void CShadowRenderer::Clear()
{
	if(!m_isInit)
		return;

	m_texAtlasPacker.Cleanup();
	CSpriteBuilder::ClearBuffers();
}

inline int AtlasPackComparison(PackerRectangle *const &elem0, PackerRectangle *const &elem1)
{
	return (elem1->width + elem1->height) - (elem0->width + elem0->height);
}

bool ShadowDecalClipping(struct decalsettings_t& settings, PFXVertex_t& v1, PFXVertex_t& v2, PFXVertex_t& v3)
{
	CGameObject* object = (CGameObject*)settings.userData;

	Vector3D triNormal = NormalOfTriangle(v1.point,v2.point,v3.point);
	Vector3D facingToObjectNormal = fastNormalize(object->GetOrigin() - v1.point);

	if(dot(triNormal, facingToObjectNormal) < 0.0f || dot(triNormal, settings.facingDir) < 0.0f)
		return false;

	return true;
}

ConVar r_shadows_debugatlas("r_shadows_debugatlas", "0");

EShadowModelRenderMode GetObjectRenderMode(CGameObject* obj)
{
	int objType = obj->ObjType();

	if(objType == GO_CAR || objType == GO_CAR_AI)
		return RSHADOW_CAR;
	else if (objType == GO_PEDESTRIAN)
		return RSHADOW_SKIN;

	return RSHADOW_STANDARD;
}

void CShadowRenderer::RenderShadowCasters()
{
	if(!m_isInit || !r_shadows.GetBool())
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
	shadowDecal.settings.avoidMaterialFlags = MATERIAL_FLAG_WATER; // only avoid water
	shadowDecal.processFunc = ShadowDecalClipping;

	CViewParams orthoView;

	orthoView.SetFOV(0.5f);
	orthoView.SetAngles(Vector3D(m_shadowAngles.x,m_shadowAngles.y,m_shadowAngles.z));

	debugoverlay->Text(ColorRGBA(1), "shadows: %d", m_texAtlasPacker.GetRectangleCount());

	g_pShaderAPI->Reset(STATE_RESET_VBO);
	g_pShaderAPI->ChangeVertexBuffer( NULL, 2 );

	// clear the shadow atlas
	g_pShaderAPI->ChangeRenderTarget( m_shadowTexture );
	g_pShaderAPI->Clear( true,false,false, ColorRGBA(1.0f) );

	// now start rendering to temporary shadow buffer
	//g_pShaderAPI->ChangeRenderTarget( m_shadowRt );

	Vector2D halfTexSizeNeg = m_shadowTextureSize*-1.0f*SHADOW_DESCALING;

	const Vector3D& viewPos = g_pGameWorld->GetView()->GetOrigin();
	float distFac = 1.0f / r_shadowDist.GetFloat();

	bool flipY = (g_pShaderAPI->GetShaderAPIClass() == SHADERAPI_OPENGL);

	for(int i = 0; i < m_texAtlasPacker.GetRectangleCount(); i++)
	{
		void* userData;
		Rectangle_t shadowRect;
		m_texAtlasPacker.GetRectangle(shadowRect, &userData, i);

		shadowListObject_t* objectGroup = (shadowListObject_t*)userData;
		CGameObject* firstObject = objectGroup->object;

		// calculate view
		Vector2D shadowPos = shadowRect.vleftTop*SHADOW_DESCALING;
		Vector2D shadowSize = shadowRect.GetSize()-SHADOW_CROP;

		IRectangle copyRect(shadowRect.vleftTop, shadowRect.vrightBottom);

		shadowRect.vleftTop *= m_shadowTexelSize;
		shadowRect.vrightBottom *= m_shadowTexelSize;

		IVector2D copyRectSize = copyRect.GetSize();
		g_pShaderAPI->SetViewport(copyRect.vleftTop.x,copyRect.vleftTop.y, copyRectSize.x, copyRectSize.y);

		// take shadow height for near plane using first object AABB
		float shadowHeight = length(firstObject->m_bbox.GetSize())*0.5f;

		// move view to the object origin
		orthoView.SetOrigin(firstObject->GetOrigin());
		orthoView.GetMatrices(proj, view, shadowSize.x*SHADOW_DESCALING, shadowSize.y*SHADOW_DESCALING, -shadowHeight, 100.0f, true );
		
		shadowDecal.settings.facingDir = view.rows[2].xyz();
		
		materials->SetMatrix(MATRIXMODE_PROJECTION, proj);
		materials->SetMatrix(MATRIXMODE_VIEW, view);
		viewProj = proj * view;

		// draw all grouped objects as single shadow
		while(objectGroup)
		{
			CGameObject* curObject = objectGroup->object;

			EShadowModelRenderMode renderMode = GetObjectRenderMode(curObject);

			materials->SetMatrix(MATRIXMODE_WORLD, curObject->m_worldMatrix);

			if(!r_shadows_debugatlas.GetBool())
				RenderShadow( curObject, curObject->GetBodyGroups(), renderMode);

			shadowListObject_t* prev = objectGroup;

			objectGroup = objectGroup->next;
			delete prev;
		};

		// copy temporary shadow buffer result to shadow atlas
		//g_pShaderAPI->CopyRendertargetToTexture(m_shadowRt, m_shadowTexture, NULL, &copyRect);

		// project our decal to sprite builder
		float shadowAlpha = length(orthoView.GetOrigin()-viewPos) * distFac;
		shadowAlpha = 1.0f - pow(max(0.0f, shadowAlpha), 8.0f);

		Plane nearPlane(-vec3_up, firstObject->m_bbox.maxPoint.y);

		shadowDecal.settings.clipVolume.LoadAsFrustum(viewProj);
		shadowDecal.settings.clipVolume.SetupPlane(nearPlane, VOLUME_PLANE_NEAR);
		shadowDecal.settings.userData = firstObject;

		if(flipY)
			shadowRect.FlipY();

		ProjectDecalToSpriteBuilder( shadowDecal, this, shadowRect, viewProj, ColorRGBA(1,1,1,shadowAlpha) );
	}

	g_pShaderAPI->ChangeRenderTargetToBackBuffer();
}

void CShadowRenderer::Draw()
{
	if(!m_isInit || !r_shadows.GetBool())
		return;

	// upload to vertex & index buffers
	int vboIdx = g_pPFXRenderer->MakeVBOFrom(this);
	if(vboIdx == -1)
		return;

	g_pShaderAPI->Reset(STATE_RESET_VBO);

	g_pShaderAPI->SetVertexFormat(g_pPFXRenderer->m_vertexFormat);
	g_pShaderAPI->SetVertexBuffer(g_pPFXRenderer->m_vertexBuffer[vboIdx], 0);
	//g_pShaderAPI->SetIndexBuffer(g_pPFXRenderer->m_indexBuffer[vboIdx]);

	// render
	materials->SetCullMode( CULL_BACK );

	materials->BindMaterial(m_shadowResult, 0);
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

void CShadowRenderer::RenderShadow(CGameObject* object, ubyte bodyGroups, int mode)
{
	IEqModel* model = object->GetModel();
	
	/*
	IMaterial* rtMaterial = m_matSimple;

	if(mode == RSHADOW_CAR)
		rtMaterial = m_matVehicle;
	else if(mode == RSHADOW_SKIN)
		rtMaterial = m_matSkinned;
	*/
	//materials->SetCullMode(CULL_FRONT);

	materials->SetInstancingEnabled(false);
	// force disable vertex buffer
	g_pShaderAPI->SetVertexBuffer( NULL, 2 );

	//object->Draw(RFLAG_SHADOW | RFLAG_NOINSTANCE);

	studiohdr_t* pHdr = model->GetHWData()->studio;

	int nLOD = r_shadowLod.GetInt();

	for (int i = 0; i < pHdr->numBodyGroups; i++)
	{
		if (!(bodyGroups & (1 << i)))
			continue;

		int bodyGroupLOD = nLOD;

		// TODO: check bodygroups for rendering

		int nLodModelIdx = pHdr->pBodyGroups(i)->lodModelIndex;
		int nModDescId = pHdr->pLodModel(nLodModelIdx)->modelsIndexes[bodyGroupLOD];

		// get the right LOD model number
		while (nModDescId == -1 && bodyGroupLOD > 0)
		{
			bodyGroupLOD--;
			nModDescId = pHdr->pLodModel(nLodModelIdx)->modelsIndexes[bodyGroupLOD];
		}

		if (nModDescId == -1)
			continue;

		// shared; for car damage
		float bodyDamages[16];
		bool canApplyDamage = false;

		if (mode == RSHADOW_CAR)
		{
			CCar* car = (CCar*)object;
			canApplyDamage = car->GetBodyDamageValuesMappedToBones(bodyDamages);
		}

		// render model groups that in this body group
		for (int j = 0; j < pHdr->pModelDesc(nModDescId)->numGroups; j++)
		{
			// FIXME:	pass additional information into that material
			//			like transparency

			if (mode == RSHADOW_CAR)
			{
				materials->BindMaterial(m_matVehicle, 0);

				CCar* car = (CCar*)object;

				IEqModel* cleanModel = car->GetModel();
				IEqModel* damagedModel = (car->GetDamagedModel() && canApplyDamage) ? car->GetDamagedModel() : cleanModel;

				// setup our brand new vertex format
				// and bind required VBO streams by hand
				g_pShaderAPI->SetVertexFormat(g_worldGlobals.vehicleVertexFormat);
				cleanModel->SetupVBOStream(0);
				damagedModel->SetupVBOStream(1);

				// instead of prepare skinning, we send BodyDamage
				g_pShaderAPI->SetShaderConstantArrayFloat("BodyDamage", bodyDamages, 16);

				model->DrawGroup(nModDescId, j, false);
			}
			else if (mode == RSHADOW_SKIN)
			{
				CAnimatingEGF* animating = (CAnimatingEGF*)object;

				materials->SetSkinningEnabled(true);

				materials->BindMaterial(m_matSkinned, 0);

				model->PrepareForSkinning(animating->GetBoneMatrices());
				model->DrawGroup(nModDescId, j);

				materials->SetSkinningEnabled(false);
			}
			else
			{
				materials->BindMaterial(m_matSimple, 0);

				model->DrawGroup(nModDescId, j);
			}
			
		}
	}
}