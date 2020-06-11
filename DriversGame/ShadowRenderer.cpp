//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Driver Syndicate shadow renderer
//////////////////////////////////////////////////////////////////////////////////

#include "ShadowRenderer.h"
#include "ConVar.h"
#include "world.h"
#include "ParticleEffects.h"

#include "Animating.h"
#include "car.h"

#include "Shiny.h"

ConVar r_shadowAtlasSize("r_shadowAtlasSize", "1024", 256.0f,  1024.0f, NULL, CV_ARCHIVE);
ConVar r_shadowLod("r_shadowLod", "0", NULL, CV_ARCHIVE);

ConVar r_shadows("r_shadows", "1", NULL, CV_ARCHIVE);

ConVar r_shadowAngleDirtyThresh("r_shadowAngleDirtyThresh", "2.0");

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

void FreeShadowList(shadowListObject_t* object)
{
	// delete all grouped objects
	while (object)
	{
		shadowListObject_t* prev = object;

		object = object->next;
		delete prev;
	};
}

CShadowRenderer::CShadowRenderer() : m_shadowTexture(NULL), m_shadowAngles(90,0,0), m_matVehicle(NULL), m_matSkinned(NULL), m_matSimple(NULL), m_isInit(false)
{
	m_packer.SetPackPadding( 2.0 );
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

	kvkeybase_t shadowBuildParams;
	shadowBuildParams.SetKey("nofog", true);
	shadowBuildParams.SetKey("zwrite", false);
	shadowBuildParams.SetKey("ztest", false);
	//shadowBuildParams.SetKey("translucent", true);

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
		//shadowParams.SetKey("noisetexture", "_noise");

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

	g_pShaderAPI->FreeTexture(m_shadowTexture);
	m_shadowTexture = NULL;

	materials->FreeMaterial(m_matVehicle);
	materials->FreeMaterial(m_matSkinned);
	materials->FreeMaterial(m_matSimple);
	materials->FreeMaterial(m_shadowResult);

	CSpriteBuilder::Shutdown();

	m_isInit = false;
}

bool CShadowRenderer::CanCastShadows(CGameObject* object)
{
	if (!(object->GetDrawFlags() & GO_DRAW_FLAG_SHADOW))
		return false;

	// also check if they are too far from view
	const Vector3D& viewPos = g_pGameWorld->GetView()->GetOrigin();
	if (length(viewPos - object->GetOrigin()) > r_shadowDist.GetFloat())
		return false;

	Vector3D sunDir;
	AngleVectors(m_shadowAngles, &sunDir);

	// FIXME: physics trace to ground??
	BoundingBox bbox = object->GetModel()->GetAABB();
	bbox.minPoint += object->GetOrigin();
	bbox.maxPoint += object->GetOrigin();
	bbox.AddVertex(bbox.GetCenter() - sunDir * 4.0f);

	float bboxSize = length(bbox.GetSize());
	return g_pGameWorld->m_occludingFrustum.IsSphereVisible(bbox.GetCenter(), bboxSize);
}

void CShadowRenderer::AddShadowCaster( CGameObject* object, struct shadowListObject_t* addTo )
{
	if (!m_isInit || !r_shadows.GetBool())
		return;

	if(object->GetModel() == NULL) // egf model required
		return;

	if (!addTo && !CanCastShadows(object))
		return;

	shadowListObject_t* casterObject = new shadowListObject_t;
	casterObject->object = object;

	int numShadowCasterObjects = object->GetChildCasterCount();

	for (int i = 0; i < numShadowCasterObjects; i++)
	{
		AddShadowCaster(object->GetChildShadowCaster(i), casterObject);
	}

	if(addTo)
	{
		casterObject->next = addTo->next;
		addTo->next = casterObject;
	}
	else
	{
		float shadowObjectSize = length(object->GetModel()->GetAABB().GetSize()) * SHADOW_SCALING;

		m_packer.AddRectangle(shadowObjectSize, shadowObjectSize, casterObject);
	}

}

void CShadowRenderer::Clear()
{
	if(!m_isInit)
		return;

	// destroy the unreleased lists as well
	for (int i = 0; i < m_packer.GetRectangleCount(); i++)
	{
		shadowListObject_t* list = (shadowListObject_t*)m_packer.GetRectangleUserData(i);
		FreeShadowList(list);
	}
	m_packer.Cleanup();

	CSpriteBuilder::ClearBuffers();
}

inline int AtlasPackComparison(PackerRectangle *const &elem0, PackerRectangle *const &elem1)
{
	return (elem1->width + elem1->height) - (elem0->width + elem0->height);
}

bool ShadowDecalClipping(struct decalPrimitives_t* decal, struct decalSettings_t& settings, PFXVertex_t& v1, PFXVertex_t& v2, PFXVertex_t& v3)
{
	Vector3D triNormal = NormalOfTriangle(v1.point,v2.point,v3.point);
	Vector3D facingToObjectNormal = fastNormalize(decal->position - v1.point);

	if(dot(triNormal, facingToObjectNormal) < 0.0f || dot(triNormal, settings.facingDir) < 0.0f)
		return false;

	// TODO: calc alpha here

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

	PROFILE_BEGIN(PackRects);

	// pack all rectangles
	if (!m_packer.AssignCoords(neededTexSize.x, neededTexSize.y)) //, AtlasPackComparison))
	{
		// delete all decals since it's sick
		for (int i = 0; i < m_packer.GetRectangleCount(); i++)
		{
			shadowListObject_t* objectGroup = (shadowListObject_t*)m_packer.GetRectangleUserData(i);

			// delete all grouped objects
			FreeShadowList(objectGroup);
		}

		m_packer.Cleanup();

		debugoverlay->Text(ColorRGBA(1, 0, 0, 1), "shadows render overflow");

		return; // skip the render
	}

	PROFILE_END();

	debugoverlay->Text(ColorRGBA(1), "shadows: %d", m_packer.GetRectangleCount());

	g_pShaderAPI->Reset(STATE_RESET_VBO);
	g_pShaderAPI->ChangeVertexBuffer( NULL, 2 );

	// clear the shadow atlas
	g_pShaderAPI->ChangeRenderTarget( m_shadowTexture );
	g_pShaderAPI->Clear( true,false,false, ColorRGBA(1.0f) );

	// now start rendering to temporary shadow buffer
	//g_pShaderAPI->ChangeRenderTarget( m_shadowRt );

	Vector2D halfTexSizeNeg = m_shadowTextureSize*-1.0f*SHADOW_DESCALING;

	Matrix4x4 proj, view, viewProj;

	const Vector3D& viewPos = g_pGameWorld->GetView()->GetOrigin();

	CViewParams orthoView;

	orthoView.SetFOV(0.5f);
	orthoView.SetAngles(m_shadowAngles);

	float distFac = 1.0f / r_shadowDist.GetFloat();

	bool flipY = (g_pShaderAPI->GetShaderAPIClass() == SHADERAPI_OPENGL);

	decalSettings_t decalSettings;
	decalSettings.processFunc = ShadowDecalClipping;
	decalSettings.avoidMaterialFlags = MATERIAL_FLAG_WATER; // only avoid water
	decalSettings.skipTexCoords = true;

	// generate decals before render targets
	for (int i = 0; i < m_packer.GetRectangleCount(); i++)
	{
		// get the object
		shadowListObject_t* objectGroup;

		// get the rectangle on the texture
		Rectangle_t shadowRect;
		m_packer.GetRectangle(shadowRect, (void**)&objectGroup, i);

		CGameObject* firstObject = objectGroup->object;

		// use shadow decal of first object
		decalPrimitives_t& shadowDecal = firstObject->m_shadowDecal;

		float shadowObjectSize = length(firstObject->GetModel()->GetAABB().GetSize()) * SHADOW_SCALING;

		// calculate view
		Vector2D shadowSize = shadowObjectSize - SHADOW_CROP;

		// take shadow height for near plane using first object AABB
		float shadowHeight = length(firstObject->m_bbox.GetSize())*0.5f;

		// move view to the object origin
		orthoView.SetOrigin(firstObject->GetOrigin());
		orthoView.GetMatrices(proj, view, shadowSize.x*SHADOW_DESCALING, shadowSize.y*SHADOW_DESCALING, -shadowHeight, 100.0f, true);

		decalSettings.facingDir = view.rows[2].xyz();
		decalSettings.userData = objectGroup;

		// project our decal to sprite builder
		viewProj = proj * view;

		float shadowAlpha = length(orthoView.GetOrigin() - viewPos) * distFac;
		shadowAlpha = 1.0f - pow(max(0.0f, shadowAlpha), 16.0f);

		if (shadowAlpha <= 0.0f)
		{
			FreeShadowList(objectGroup);
			continue;
		}

		Plane nearPlane(-vec3_up, firstObject->m_bbox.maxPoint.y);// +orthoView.GetOrigin().y);

		decalSettings.clipVolume.LoadAsFrustum(viewProj);
		decalSettings.clipVolume.SetupPlane(nearPlane, VOLUME_PLANE_NEAR);
		decalSettings.customClipVolume = true;

		shadowDecal.settings = decalSettings;

		Rectangle_t texCoordRect = shadowRect;
		texCoordRect.vleftTop *= m_shadowTexelSize;
		texCoordRect.vrightBottom *= m_shadowTexelSize;

		if (m_invalidateAllDecals || shadowDecal.dirty)
			shadowDecal.Clear();

		if (flipY)
			texCoordRect.FlipY();

		//ProjectDecalToSpriteBuilderAddJob(decalSettings, this, texCoordRect, viewProj, ColorRGBA(1.0f, 1.0f, 1.0f, shadowAlpha));

		PROFILE_BEGIN(ProjectDecal);

		decalPrimitivesRef_t ref = ProjectDecalToSpriteBuilder(shadowDecal, this, texCoordRect, viewProj);

		PROFILE_END();
		
		// don't draw decal if it has no polys at all
		if (!ref.numVerts)
		{
			FreeShadowList(objectGroup);
			continue;
		}

		PROFILE_BEGIN(DecalTexture);

		// recalc texture coords and apply shadow alpha
		DecalTexture(ref, viewProj, texCoordRect, ColorRGBA(1.0f, 1.0f, 1.0f, shadowAlpha));

		PROFILE_END();

		materials->SetMatrix(MATRIXMODE_VIEW, view);
		materials->SetMatrix(MATRIXMODE_PROJECTION, proj);

		// now draw the decal
		PROFILE_BEGIN(RenderShadow);
		{
			// prepare viewport
			IRectangle viewportRect(shadowRect.vleftTop, shadowRect.vrightBottom);
			IVector2D viewportRectSize = viewportRect.GetSize();

			// prepare to draw
			g_pShaderAPI->SetViewport(viewportRect.vleftTop.x, viewportRect.vleftTop.y, viewportRectSize.x, viewportRectSize.y);

			// draw all grouped objects as single shadow
			while (objectGroup)
			{
				CGameObject* curObject = objectGroup->object;

				EShadowModelRenderMode renderMode = GetObjectRenderMode(curObject);

				materials->SetMatrix(MATRIXMODE_WORLD, curObject->m_worldMatrix);

				if (!r_shadows_debugatlas.GetBool())
					RenderShadow(curObject, curObject->GetBodyGroups(), renderMode);

				// free all stuff as we don't need it
				shadowListObject_t* prev = objectGroup;

				objectGroup = objectGroup->next;
				delete prev;
			};
		}
		PROFILE_END();
	}

	m_packer.Cleanup();

	g_pShaderAPI->ChangeRenderTargetToBackBuffer();

	m_invalidateAllDecals = false;
}

void CShadowRenderer::Draw()
{
	if(!m_isInit || !r_shadows.GetBool())
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

	materials->BindMaterial(m_shadowResult, 0);

	g_pShaderAPI->SetTexture(g_pGameWorld->m_noiseTex, "NoiseTexture", 8);

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
	// if sun angle changed significantly, update decals
	if (!fsimilar(m_shadowAngles.x, angles.x, r_shadowAngleDirtyThresh.GetFloat()) ||
		!fsimilar(m_shadowAngles.y, angles.y, r_shadowAngleDirtyThresh.GetFloat()))
	{
		m_shadowAngles = angles;
		m_invalidateAllDecals = true;
	}
}

void CShadowRenderer::RenderShadow(CGameObject* object, ubyte bodyGroups, int mode)
{
	IEqModel* model = object->GetModel();
	object->UpdateTransform();
	
	materials->SetInstancingEnabled(false);
	// force disable vertex buffer
	g_pShaderAPI->SetVertexBuffer( NULL, 2 );

	materials->SetCullMode(CULL_FRONT);

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
		float bodyDamages[16] = { 0.0f };
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
				g_pShaderAPI->SetVertexFormat(g_worldGlobals.vehicleVF);
				cleanModel->SetupVBOStream(0);
				damagedModel->SetupVBOStream(1);

				// instead of prepare skinning, we send BodyDamage
				g_pShaderAPI->SetShaderConstantArrayFloat("BodyDamage", bodyDamages, elementsOf(bodyDamages));

				model->DrawGroup(nModDescId, j, false);
			}
			else if (mode == RSHADOW_SKIN)
			{
#ifndef PLAT_ANDROID
				CAnimatingEGF* animating = dynamic_cast<CAnimatingEGF*>(object);

				Matrix4x4* bones = animating->GetBoneMatrices();

				materials->SetSkinningEnabled(true);

				materials->BindMaterial(m_matSkinned, 0);

				model->PrepareForSkinning(bones);
				model->DrawGroup(nModDescId, j);

				materials->SetSkinningEnabled(false);
#else
				// NOT SUPPORTED for SKINNED
#endif // PLAT_ANDROID
			}
			else
			{
				materials->BindMaterial(m_matSimple, 0);

				model->DrawGroup(nModDescId, j);
			}
			
		}
	}
}