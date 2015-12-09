//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Client game renderer
//////////////////////////////////////////////////////////////////////////////////

#include "GameRenderer.h"
#include "BaseEngineHeader.h"

#include "ParticleRenderer.h"

#include "Effects.h"
#include "IEqModel.h"
#include "ParticleRenderer.h"
#include "EngineEntities.h"

#include "FilterPipelineBase.h"

#include "refcounted.h"

#include "Utils/eqjobs.h"

ConVar r_detail_dist("r_detail_dist", "1500");

ConVar r_zfar("r_zfar", "-1", "Z-Far range", CV_CHEAT);
ConVar r_ambientscale("r_ambientscale", "1.0", "Ambient lighting scale", CV_CHEAT);
ConVar r_ambientadd("r_ambientadd", "0", "Ambient lighting add", CV_CHEAT);

ConVar fog_override("fog_override","0","Fog parameters override", CV_CHEAT);
ConVar fog_enable("fog_enable","0","Enables fog",CV_ARCHIVE);

ConVar fog_color_r("fog_color_r","0.5","Fog color",CV_ARCHIVE);
ConVar fog_color_g("fog_color_g","0.5","Fog color",CV_ARCHIVE);
ConVar fog_color_b("fog_color_b","0.5","Fog color",CV_ARCHIVE);
ConVar fog_far("fog_far","400","Fog far",CV_ARCHIVE);
ConVar fog_near("fog_near","20","Fog near",CV_ARCHIVE);
ConVar fog_density("fog_density","0.35f","Fog near",CV_ARCHIVE);

/*
static CDetailSystemTest s_details;
CDetailSystemTest* g_pDetailSystem = &s_details;

DECLARE_CMD(r_details_clear, "clears details", 0)
{
	g_pDetailSystem->ClearNodes();
}*/

void CDetailSystemTest::DrawNodes()
{
	if(!m_bInit || !viewrenderer->GetView() || !m_pDetailVertexBuffer || !m_pDetailIndexBuffer)
		return;

	CViewParams* pView = viewrenderer->GetView();
	Volume frustum = *viewrenderer->GetViewFrustum();
	for(int i = 0; i < 4; i++)
	{
		Plane pl = frustum.GetPlane(i);
		pl.offset += 16.0f;
		frustum.SetupPlane(pl, i);
	}

	for(int i = 0; i < m_pNodes.numElem(); i++)
	{
		if(length(m_pNodes[i].position - pView->GetOrigin()) > r_detail_dist.GetFloat())
			continue;

		if(!frustum.IsPointInside(m_pNodes[i].position))
			continue;

		detail_type_t* detType = m_pDetailTypes[m_pNodes[i].nDetailType];
		detail_model_t* model = detType->models[m_pNodes[i].nDetailSubmodel];
			
		g_pShaderAPI->SetVertexFormat(m_pDetailFormat);
		g_pShaderAPI->SetVertexBuffer(m_pDetailVertexBuffer, 0);
		g_pShaderAPI->SetIndexBuffer(m_pDetailIndexBuffer);

		Matrix4x4 det_translation = translate(m_pNodes[i].position)*scale4(m_pNodes[i].scale, m_pNodes[i].scale, m_pNodes[i].scale);

		materials->SetMatrix(MATRIXMODE_WORLD, det_translation);

		//materials->SetAmbientColor(ColorRGBA(0.5,0.5,0.5, 1.0f));
		materials->BindMaterial( model->material, false );
		materials->Apply();

		g_pShaderAPI->DrawIndexedPrimitives( PRIM_TRIANGLES, model->startIndex, model->indices.numElem(), model->startVertex, model->verts.numElem() );
	}
}

using namespace Threading;

static CBasicRenderList s_modelList;
CBasicRenderList*		g_modelList = &s_modelList;

static CBasicRenderList s_staticPropList;
CBasicRenderList*		g_staticPropList = &s_staticPropList;

static CBasicRenderList s_viewModels;
CBasicRenderList*		g_viewModels = &s_viewModels;

static CBasicRenderList s_transparentsList;
CBasicRenderList*		g_transparentsList = &s_transparentsList;

static CBasicRenderList s_decalList;
CBasicRenderList*		g_decalList = &s_decalList;

// the main view entity
BaseEntity*				g_pViewEntity = NULL;

renderAreaList_t*		g_pViewAreaList = NULL;
renderAreaList_t*		g_pLightViewAreaList = NULL;

// water info
IWaterInfo*				g_pCurrentWaterInfo = NULL;

// rope temp verts
DkList<eqropevertex_t>	g_rope_verts;
DkList<uint16>			g_rope_indices;

IVertexBuffer*			g_pRopeVB = NULL;
IIndexBuffer*			g_pRopeIB = NULL;
IVertexFormat*			g_pRopeFormat = NULL;

ITexture*				g_pFramebufferTexture = NULL;
ITexture*				g_pHDRBlur1Texture = NULL;
ITexture*				g_pHDRBlur2Texture = NULL;

ITexture*				g_pUnderwaterTexture = NULL;
ITexture*				g_pReflectionTexture = NULL;
ITexture*				g_pTransparentLighting = NULL;

IMaterial*				g_pHDRBlur1	= NULL;
IMaterial*				g_pHDRBlur2	= NULL;
IMaterial*				g_pHDRBloom	= NULL;

ConVar	r_cpulight_scale("r_cpulight_scale", "1.0f", "Lightscale for particles", CV_ARCHIVE);
ConVar	r_cpulight_lmscale("r_cpulight_lmscale", "1.0f", "Lightscale for particles", CV_ARCHIVE);

ConVar	r_skipLights("r_skipLights", "0", "Skip light rendering", CV_CHEAT);
ConVar	r_skipShadowMaps("r_skipShadowMaps", "0", "Skip light shadow maps", CV_CHEAT);
ConVar	r_skipPostProcessFilters("r_skipPostProcessFilters", "0", "Skip post-processing filters", CV_ARCHIVE);

ConVar	r_fastNoShadows("r_fastNoShadows", "0", "Skip shadowmaps for omni and spot lights", CV_CHEAT);

DECLARE_CMD(ent_view, "sets view to entity", CV_CHEAT)
{
	if(CMD_ARGC > 0)
	{
		BaseEntity* pEntity = UTIL_EntByIndex(atoi( CMD_ARGV(0).GetData() ));

		if(pEntity)
			GR_SetViewEntity( pEntity );
		else
			MsgError("no such entity with index '%s'", CMD_ARGV(0).GetData());
	}
}

// special software lighting calculation
inline Vector3D ComputeLightingForPoint_PointLight(dlight_t* light, const Vector3D& point)
{
	if(length(point - light->position) > light->radius.x)
		return Vector3D(0);

	if(light->nType == DLT_OMNIDIRECTIONAL)
	{
		Vector3D lInvRad = 1.0f / light->radius.x;

		Vector3D lDir = (light->position - point);
		Vector3D transDir = light->lightRotation*lDir;

		// do it like i'm doing it in the shader
		Vector3D lVec = transDir * lInvRad;
		return light->color * saturate((1.0f - dot(lVec, lVec))) * light->intensity;
	}
	else
	{
		float lInvRad = 1.0f / light->radius.x;

		//if( !(light->nFlags & LFLAG_MATRIXSET) )
		Matrix3x3 tr = transpose(light->lightRotation);
		//Matrix4x4 rot = !rotateXYZ(DEG2RAD(light->angles.x),DEG2RAD(light->angles.y),DEG2RAD(light->angles.z));

		float fCosine = saturate(dot(-tr.rows[2], normalize(light->position - point)) );

		float fBrightness = pow(saturate( fCosine-cos(DEG2RAD(light->fovWH.z)) ), 4.5f);


		//debugoverlay->Line3D(point, point + tr.rows[2]*100, ColorRGBA(1), ColorRGBA(0));
			
		//Vector4D sPos = light->lightWVP*Vector4D(point, 1.0f);

		//Vector4D spotVector = sPos + sPos.w;

		//Vector2D sProject = spotVector.xy() / spotVector.w;

		// do it like i'm doing it in the shader
		Vector3D lVec = (light->position - point) * lInvRad;

		return light->color * saturate((1.0f - dot(lVec, lVec))) * light->intensity * fBrightness;
	}
}

extern CWorldInfo* g_pWorldInfo;

extern ConVar r_ambientscale;
extern ConVar r_ambientadd;

// utility function to compute lighting
Vector3D ComputeLightingForPoint(Vector3D &point, bool doShading)
{
	ColorRGB world_lights_color;
	viewrenderer->GetLightingSampleAtPosition(point, world_lights_color);

	if(!viewrenderer->GetLightCount())
	{
		return g_pWorldInfo->GetAmbient()*r_ambientscale.GetFloat()+world_lights_color*r_cpulight_lmscale.GetFloat() ;
	}

	Vector3D sharedColor = Vector3D(0,0,0);

	for(int i = 0; i < viewrenderer->GetLightCount(); i++)
	{
		dlight_t* pLight = viewrenderer->GetLight(i);

		if(pLight->nType == DLT_OMNIDIRECTIONAL)
		{
			if(length(pLight->position - point) > pLight->radius.x)
				continue;

			if(doShading)
			{
				internaltrace_t tr;
				physics->InternalTraceLine(pLight->position, point, COLLISION_GROUP_WORLD, &tr);

				if(tr.fraction == 1.0f)
					sharedColor += ComputeLightingForPoint_PointLight(pLight, point);
			}
			else
				sharedColor += ComputeLightingForPoint_PointLight(pLight, point);
		}
		else
		{
			if(pLight->nType == DLT_SUN)
			{
				//Matrix4x4 sun_view = rotateZXY(DEG2RAD(-pLight->angles.x),DEG2RAD(-pLight->angles.y),DEG2RAD(-pLight->angles.z));

				//AngleVectors(pLight->angles,&lightDir,NULL,NULL);

				Vector3D lightDir = Vector3D(0,1,0);

				if(pLight->nFlags & LFLAG_MATRIXSET)
				{
					lightDir = pLight->lightRotation[2];
				}
				else
				{
					Matrix4x4 sun_view = !rotateXYZ4(DEG2RAD(pLight->angles.x),DEG2RAD(pLight->angles.y),DEG2RAD(pLight->angles.z));
					lightDir = sun_view.rows[2].xyz();
				}

				Vector3D sun_pos = point - lightDir * MAX_COORD_UNITS;
				/*
				if(doShading)
				{*/
					internaltrace_t tr;
					physics->InternalTraceLine(sun_pos, point, COLLISION_GROUP_WORLD, &tr);

					if(tr.fraction == 1.0f)
						sharedColor += pLight->color * pLight->intensity;
					/*
				}
				else
					sharedColor += pLight->color * pLight->intensity;*/
			}
			else
			{
				if(length(pLight->position - point) > pLight->radius.x)
					continue;

				if(doShading)
				{
					internaltrace_t tr;
					physics->InternalTraceLine(pLight->position, point, COLLISION_GROUP_WORLD, &tr);

					if(tr.fraction == 1.0f)
						sharedColor += ComputeLightingForPoint_PointLight(pLight, point);
				}
				else
					sharedColor += ComputeLightingForPoint_PointLight(pLight, point);
				/*
				// frustum check first
				if(pLight->nFlags & LFLAG_MATRIXSET)
				{
					Volume lFrustum;
					lFrustum.LoadAsFrustum(pLight->lightWVP);

					if( lFrustum.IsPointInside(point) )
					{
						if(doShading)
						{
							internaltrace_t tr;
							physics->InternalTraceLine(pLight->position, point, COLLISION_GROUP_WORLD, &tr);

							if(tr.fraction == 1.0f)
								sharedColor += ComputeLightingForPoint_PointLight(pLight, point);
						}
						else
							sharedColor += ComputeLightingForPoint_PointLight(pLight, point);
					}
				}
				else
				{
					// spot forward
					Vector3D fw;
					AngleVectors(pLight->angles, &fw);

					float angle = acosf(dot(normalize(point - pLight->position),fw));

					// some invalidance here
					//if(IsPointInCone(point, lights[i]->position, fw, cosf(DEG2RAD(lights[i]->fov)), lights[i]->radius))
					if(angle > DEG2RAD(pLight->fovWH.z))
					{
						if(doShading)
						{
							internaltrace_t tr;
							physics->InternalTraceLine(pLight->position, point, COLLISION_GROUP_WORLD, &tr);

							if(tr.fraction == 1.0f)
								sharedColor += ComputeLightingForPoint_PointLight(pLight, point);
						}
						else
							sharedColor += ComputeLightingForPoint_PointLight(pLight, point);
					}
				}
				*/
			}
		}
	}

	return world_lights_color*r_cpulight_lmscale.GetFloat() + sharedColor*r_cpulight_scale.GetFloat() + g_pWorldInfo->GetAmbient()*r_ambientscale.GetFloat() + Vector3D(r_ambientadd.GetFloat());

	//return color3_white;
}


HOOK_TO_CVAR(r_advancedlighting);
ConVar r_viewmodelfov("r_viewModelFov","0.7","View model FOV",CV_CHEAT);
bool g_bShotCubemap = false;

DECLARE_CMD(cubemap, "Shot cubemap", CV_CHEAT)
{
	g_bShotCubemap = true;
}

ConVar r_waterReflection("r_waterReflections","1", "Draw water reflection", CV_ARCHIVE);
ConVar r_waterReflectionShadows("r_waterReflectionShadows","0","Draw water reflection shadows (VEEERY UNOPTIMIZED FEATURE)", CV_ARCHIVE);
ConVar r_fastCheapWater("r_fastCheapWater","0", "Fast switch to cheap water", CV_CHEAT);

ConVar r_sunFrustumClipScale("r_sunFrustumClipScale", "1.0");
ConVar r_omniFrustumClipScale("r_omniFrustumClipScale", "1.0");

Volume BuildSunFrustum( const Volume& viewerCameraFrustum, const Vector3D& viewerCameraPos, const Matrix4x4& sunViewProj, float fPlaneOffset )
{
	// keep only near plane of sun, others should be clamped
	Volume sunVolumeFull;
	sunVolumeFull.LoadAsFrustum(sunViewProj);

	Plane sunNear = sunVolumeFull.GetPlane(VOLUME_PLANE_NEAR);

	Vector3D points[5] =
	{
		viewerCameraFrustum.GetFarLeftUp(),
		viewerCameraFrustum.GetFarLeftDown(),
		viewerCameraFrustum.GetFarRightUp(),
		viewerCameraFrustum.GetFarRightDown(),
		viewerCameraPos,
	};

	for(int i = 0; i < 5; i++)
	{
		Plane pl = sunVolumeFull.GetPlane(i);

		// reset plane
		pl.offset = -dot(pl.normal, viewerCameraPos);

		// test this plane
		float min_move_dist = 0;

		for(int j = 0; j < 5; j++)
		{
			if(pl.ClassifyPoint(points[j]) == CP_BACK)
			{
				float dist = pl.Distance( points[j] );

				if(fabs(dist) > fabs(min_move_dist))
					pl.offset += dist * -r_sunFrustumClipScale.GetFloat() + fPlaneOffset;
			}
		}

		sunVolumeFull.SetupPlane(pl, i);
	}

	return sunVolumeFull;
}

extern ConVar	r_sun_znear;
extern ConVar	r_sun_zfar;

ConVar r_sun_recalc_size("r_sun_recalc_size", "0.01");

Matrix4x4 CalcBBoxSunlightViewMatrix(dlight_t* pLight, BoundingBox& bbox)
{
	Matrix4x4 mat;

	Vector3D bbox_size = bbox.GetSize();
	float fSize = length(bbox_size)*r_sun_recalc_size.GetFloat();

	Matrix4x4 sun_proj = orthoMatrixR(-fSize,fSize,-fSize,fSize, r_sun_znear.GetFloat(), r_sun_zfar.GetFloat());

	Matrix4x4 sun_view = !rotateXYZ4(DEG2RAD(pLight->angles.x),DEG2RAD(pLight->angles.y),DEG2RAD(pLight->angles.z));
	pLight->lightRotation = sun_view.getRotationComponent();

	pLight->radius.x = sun_proj.rows[2][3];

	sun_view.translate(-pLight->position);

	return sun_proj*sun_view;
}

Matrix4x4 RecalcSunMatrixByFrustum(dlight_t* pLight, const Volume& sunFrustum)
{
	BoundingBox bbox;

	for(int i = 0; i < 4; i++)
	{
		Plane iPlane = sunFrustum.GetPlane(i);

		for(int j = i; j < 5; j++)
		{
			Plane jPlane = sunFrustum.GetPlane(j);

			for(int k = j; k < 6; k++)
			{
				if ((i != j) && (i != k) && (j != k))
				{

					Plane kPlane = sunFrustum.GetPlane(k);

					Vector3D point;

					if( iPlane.GetIntersectionWithPlanes(jPlane, kPlane, point) && sunFrustum.IsPointInside(point))
						bbox.AddVertex(point);
				}
			}
		}
	}

	return CalcBBoxSunlightViewMatrix(pLight, bbox);
}

Volume BuildOmniFrustum( const Volume& viewerCameraFrustum, const Vector3D& viewerCameraPos, const Vector3D& lightPos, const Matrix4x4& viewMatrix, const Matrix4x4& lightViewProj, float fPlaneOffset )
{
	Volume frustum;
	frustum.LoadAsFrustum(lightViewProj);

	// keep all planes except far
	Plane lightFar = frustum.GetPlane(VOLUME_PLANE_FAR);
	Plane lightNear = frustum.GetPlane(VOLUME_PLANE_NEAR);

	//frustum.IsPointInside(viewerCameraPos);
	if(viewerCameraFrustum.IsPointInside(lightPos))
		return frustum;

	if(lightFar.ClassifyPoint(viewerCameraPos) == CP_FRONT && lightNear.ClassifyPoint(viewerCameraPos) == CP_FRONT)
	{
		float dist = lightFar.Distance( viewerCameraPos );

		lightFar.offset += dist * -r_omniFrustumClipScale.GetFloat() + fPlaneOffset;

		frustum.SetupPlane(lightFar, VOLUME_PLANE_FAR);
	}

	return frustum;
}

bool IsCubeShadowFaceDrawn(const Vector3D& viewerPos, const Matrix4x4& viewMatrix, const sceneinfo_t& sceneinfo, const Vector3D& lightPos, int nCubeFace)
{
	// TODO: realize and implement...

	Matrix4x4 lightViewMatrix = cubeViewMatrix(nCubeFace);

	Vector3D light_dir = lightViewMatrix.rows[2].xyz();

	// make plane
	Plane pl(light_dir, -dot(light_dir, lightPos));

	Vector3D view_dir = viewMatrix.rows[2].xyz();

	if(pl.Distance(viewerPos) < 0.0f && dot(view_dir, light_dir) < 0)
		return false;

	return true;
}

extern ConVar r_sun_osize1;

void GR_LightShadow( dlight_t* pLight, CViewParams* pView, sceneinfo_t& scene, int nRenderFlags, const Volume& viewFrustum, const Matrix4x4& viewMatrix, const Vector3D& viewerPos)
{
	int nPasses = (pLight->nType == DLT_OMNIDIRECTIONAL) ? 6 : 1;

	// set the matrix
	if(pLight->nType == DLT_SUN)
		nPasses = 2;

	int nPassesDrawn = 0;

	int nVRFlags = nRenderFlags | VR_FLAG_NO_TRANSLUCENT;

	bool bLightPositionInFrustum = viewFrustum.IsPointInside(pLight->position);

	for(int i = 0; i < nPasses; i++)
	{
		Matrix4x4 temp = pLight->lightWVP;

		viewrenderer->SetCubemapIndex( i );

		viewrenderer->SetSceneInfo(scene);
		viewrenderer->SetView(pView);

		if(pLight->nType == DLT_SUN)
		{
			if(i > 0)
				pLight->lightWVP = pLight->lightWVP2;

			nVRFlags |= VR_FLAG_CUSTOM_FRUSTUM;

			// setup our custom frustum
			Volume* sunFrustum = viewrenderer->GetViewFrustum();
			(*sunFrustum) = BuildSunFrustum(viewFrustum, viewerPos, pLight->lightWVP, (i > 0) ? r_sun_osize1.GetFloat() : 0.0f);

			// modify sun matrix
			//pLight->lightWVP = RecalcSunMatrixByFrustum(pLight, *sunFrustum);
			//temp = pLight->lightWVP;
		}
		else if(pLight->nType == DLT_OMNIDIRECTIONAL)
		{
			// skip this face if we haven't see it
			if(!bLightPositionInFrustum && !IsCubeShadowFaceDrawn(viewerPos, viewMatrix, scene, pLight->position, i))
				continue;

			//if(!IsCubeShadowFaceDrawn(viewerPos, viewMatrix, scene, pLight->position, i))
			{
				viewrenderer->BuildViewMatrices(1,1,nVRFlags);

				// setup our custom frustum
				Volume* lightFaceFrustum = viewrenderer->GetViewFrustum();
				(*lightFaceFrustum) = BuildOmniFrustum(viewFrustum, viewerPos, pLight->position, viewMatrix, viewrenderer->GetViewProjectionMatrix(), 0.0f);

				nVRFlags |= VR_FLAG_CUSTOM_FRUSTUM;
			}
		}

		viewrenderer->SetDrawMode(VDM_SHADOW);

		// draw world without translucent objects
		viewrenderer->DrawWorld( nVRFlags );

		// draw model list
		viewrenderer->DrawRenderList(g_modelList, nVRFlags | VR_FLAG_MODEL_LIST);

		// restore
		if(pLight->nType == DLT_SUN)
			pLight->lightWVP = temp;

		nPassesDrawn++;
	}

	debugoverlay->Text(ColorRGBA(0,1,0,1), "Shadowmap draws: %d", nPassesDrawn); 
}

//-------------------------------------------------------------------------------------------------
// Whole scene pipeline
//-------------------------------------------------------------------------------------------------

ConVar r_drawTransparents("r_drawTransparents","1","Draw transparent world", CV_CHEAT);
ConVar r_drawTransparentsForwardLighting("r_drawTransparentsForwardLighting","1","Draw transparent world with forward shading", CV_CHEAT);

ConVar r_lightShadowDisableDistScale("r_lightShadowDisableDistScale", "0.08f", "Shadowmap disables at distance", CV_ARCHIVE);

CEqJobList* g_pJobList = NULL;

void GR_EffectRenderer()
{

	// effects is drawn before viewmodels
	effectrenderer->DrawEffects( gpGlobals->frametime );
}

CViewParams g_viewEntityParams;
CViewParams g_waterReflParams;


void GR_DrawScene( int nRenderFlags )
{
	if(g_pViewEntity == NULL)
		return;

	if(g_bShotCubemap && (nRenderFlags & VR_FLAG_WATERREFLECTION))
		return;

	bool bDoWater = (nRenderFlags & VR_FLAG_WATERREFLECTION);

	sceneinfo_t scinfo = viewrenderer->GetSceneInfo();

	Vector3D origin, angles;
	float fFOV;

	g_pShaderAPI->ChangeRenderTargetToBackBuffer();
	g_pShaderAPI->Reset();

	g_pViewEntity->CalcView( origin, angles, fFOV );

	g_viewEntityParams.SetOrigin(origin);
	g_viewEntityParams.SetAngles(angles);
	g_viewEntityParams.SetFOV(fFOV);

	int nOrigRenderFlags = nRenderFlags;

	// if water is used, reflect by plane
	if( bDoWater )
	{
		if(!g_pCurrentWaterInfo)
			return;

		if(!r_waterReflection.GetBool())
			return;

		// don't render if we're underwater
		if(origin.y < g_pCurrentWaterInfo->GetWaterHeightLevel())
			return;

		Vector3D waterOffs(0, g_pCurrentWaterInfo->GetWaterHeightLevel()*2, 0);

		Vector3D cam_pos = origin * Vector3D(1,-1, 1) + waterOffs;

		g_waterReflParams.SetFOV(g_viewEntityParams.GetFOV());
		g_waterReflParams.SetOrigin(origin * Vector3D(1,-1, 1) + waterOffs);
		g_waterReflParams.SetAngles(angles * Vector3D(-1,1,-1));

		// set the water render targets
		viewrenderer->SetupRenderTargets(1, &g_pReflectionTexture, NULL, 0);

		viewrenderer->SetView( &g_waterReflParams );
	}
	else
		viewrenderer->SetView( &g_viewEntityParams );

	int nCubemapShot = 0;

	int viewportw,viewporth;
	viewportw = g_pEngineHost->GetWindowSize().x;
	viewporth = g_pEngineHost->GetWindowSize().y;
	
	ITexture* pCubemapTarget = NULL;
	ITexture* pCubemapDepthTarget = NULL;

	bool bMustShotCubemap = g_bShotCubemap && !bDoWater;
	
	if( bMustShotCubemap )
	{
		pCubemapTarget = g_pShaderAPI->CreateNamedRenderTarget("_rt_tempcubemap", 512,512,FORMAT_RGBA8,TEXFILTER_LINEAR,ADDRESSMODE_CLAMP,COMP_NEVER,TEXFLAG_CUBEMAP);
		pCubemapDepthTarget = g_pShaderAPI->CreateRenderTarget(512,512, FORMAT_D24, TEXFILTER_LINEAR, ADDRESSMODE_CLAMP);

		pCubemapTarget->Ref_Grab();
		pCubemapDepthTarget->Ref_Grab();
	}

reshot:

	if( bMustShotCubemap )
	{
		nRenderFlags = nOrigRenderFlags;

		viewrenderer->SetSceneInfo(scinfo);

		viewrenderer->SetupRenderTargets(1,&pCubemapTarget, pCubemapDepthTarget, &nCubemapShot);
		g_pShaderAPI->Clear(true,true,false);

		g_viewEntityParams.SetFOV(90);

		switch( nCubemapShot )
		{
			case POSITIVE_X:
				g_viewEntityParams.SetAngles(Vector3D(0,-90,0));
				break;
			case NEGATIVE_X:
				g_viewEntityParams.SetAngles(Vector3D(0,90,0));
				break;
			case POSITIVE_Y:
				g_viewEntityParams.SetAngles(Vector3D(-90,0,0));
				break;
			case NEGATIVE_Y:
				g_viewEntityParams.SetAngles(Vector3D(90,0,0));
				break;
			case POSITIVE_Z:
				g_viewEntityParams.SetAngles(Vector3D(0,0,0));
				break;
			case NEGATIVE_Z:
				g_viewEntityParams.SetAngles(Vector3D(0,-180,0));
				break;
		};

		viewrenderer->SetAreaList( g_pViewAreaList );
		viewrenderer->ResetAreaVisibilityStates();

		viewportw = 512;
		viewporth = 512;
	}

	viewrenderer->SetViewportSize(viewportw,viewporth);

	materials->SetLight(NULL);

	// draw world center axe
	debugoverlay->Line3D(vec3_zero, Vector3D(100,0,0), ColorRGBA(0,0,0,1), ColorRGBA(1,0,0,1));
	debugoverlay->Line3D(vec3_zero, Vector3D(0,100,0), ColorRGBA(0,0,0,1), ColorRGBA(0,1,0,1));
	debugoverlay->Line3D(vec3_zero, Vector3D(0,0,100), ColorRGBA(0,0,0,1), ColorRGBA(0,0,1,1));

	// set deferred lighting model
	if(r_advancedlighting->GetBool())
	{
		materials->SetCurrentLightingModel( MATERIAL_LIGHT_DEFERRED );
	}
	else
		materials->SetCurrentLightingModel( MATERIAL_LIGHT_FORWARD );

	// disable translucents in main cycle
	nRenderFlags |= VR_FLAG_NO_TRANSLUCENT;

	// store original scene info
	sceneinfo_t originalSceneInfo = viewrenderer->GetSceneInfo();

	// draw skybox on maximum distance
	viewrenderer->GetSceneInfo().m_fZFar = MAX_COORD_UNITS*2;

	// update our visibility
	viewrenderer->SetAreaList( g_pViewAreaList );
	viewrenderer->BuildViewMatrices(viewportw,viewporth, nRenderFlags);
	viewrenderer->UpdateAreaVisibility( nRenderFlags );

	nRenderFlags |= VR_FLAG_SKIPVISUPDATE;

	// draw sky and world
	viewrenderer->DrawWorld( nRenderFlags | VR_FLAG_ONLY_SKY );

	viewrenderer->SetSceneInfo(originalSceneInfo);

	// Start drawing VDM_AMBIENT
	viewrenderer->SetDrawMode( VDM_AMBIENT );
	viewrenderer->DrawWorld( nRenderFlags );

	// skip modulated decals!
	viewrenderer->DrawRenderList( g_decalList, VR_FLAG_NO_MODULATEDECALS );

	// draw model list
	viewrenderer->DrawRenderList(g_modelList, nRenderFlags | VR_FLAG_MODEL_LIST);

	//g_pDetailSystem->DrawNodes();

	// sort transparents
	g_transparentsList->SortByDistanceFrom( g_viewEntityParams.GetOrigin() );

	Volume viewFrustum = *viewrenderer->GetViewFrustum();
	Matrix4x4 viewMatrix = viewrenderer->GetMatrix(MATRIXMODE_VIEW);

	// draw view models if not in water mode
	if( !bDoWater )
	{
		//g_pJobList->AddJob((jobFunction_t)GR_EffectRenderer, NULL);
		//g_pJobList->Submit();

		// view model rendering
		CViewParams vm_params = g_viewEntityParams;
		vm_params.SetFOV( g_viewEntityParams.GetFOV() * r_viewmodelfov.GetFloat() );

		// draw viewmodels
		viewrenderer->SetView(&vm_params);

		viewrenderer->DrawRenderList(g_viewModels, nRenderFlags);
	}

	// restore view
	viewrenderer->SetView( &g_viewEntityParams );

	int nLightsRendered = 0;
	int nLightsShadowsRendered = 0;

	// draw modulated decals (VDM_AMBIENTMODULATE because differrent GBuffer setup)
	viewrenderer->SetDrawMode( VDM_AMBIENTMODULATE );
	viewrenderer->DrawRenderList( g_decalList, VR_FLAG_NO_TRANSLUCENT );

	// save the state
	int nOldRenderFlags = nRenderFlags;

	int nTransparentRenderFlags = (VR_FLAG_NO_OPAQUE | VR_FLAG_SKIPVISUPDATE | (bDoWater ? VR_FLAG_WATERREFLECTION : 0));

	// advanced lighting enabled?
	if( r_advancedlighting->GetBool() )
	{
		materials->SetCurrentLightingModel( MATERIAL_LIGHT_FORWARD );

		// Draw main view in VDM_AMBIENT
		viewrenderer->SetDrawMode( VDM_AMBIENT );

		viewrenderer->SetViewportSize(viewportw,viewporth);

		// render ambient, SSAO
		viewrenderer->DrawDeferredAmbient();

		// set light area list
		viewrenderer->SetAreaList( g_pLightViewAreaList );

		// override frustum for lights only
		
		FogInfo_t fogInfo;
		materials->GetFogInfo(fogInfo);

		Volume view_to_light_frustum(viewFrustum);
		/*
		if( fogInfo.enableFog )
		{
			float fDiff = originalSceneInfo.m_fZFar - fogInfo.fogfar;

			// cut plane using some difference between fog and Z Far
			Plane pl = viewFrustum.GetPlane(VOLUME_PLANE_FAR);
			pl.offset -= fDiff;
			view_to_light_frustum.SetupPlane(pl, VOLUME_PLANE_FAR);
		}*/

		// draw all visible lights
		for(int i = 0; i < viewrenderer->GetLightCount() && !r_skipLights.GetBool(); i++)
		{
			dlight_t* pLight = viewrenderer->GetLight(i);
			
			bool bDoShadows = !(pLight->nFlags & LFLAG_NOSHADOWS) && materials->GetConfiguration().enableShadows;

			if(!r_waterReflectionShadows.GetBool() && bDoWater && pLight->nType != DLT_SUN)
				bDoShadows = false;

			if(r_fastNoShadows.GetBool() && pLight->nType != DLT_SUN)
				bDoShadows = false;

			// check light for visibility
			if(pLight->nType != DLT_SUN && !view_to_light_frustum.IsSphereInside(pLight->position, pLight->radius.x))
				continue;

			// setup light view for shadow mapping
			CViewParams lightParams;

			int nShadowFlags = 0;

			if(pLight->nType == DLT_SUN)
			{
				// sun light position is set from main camera view
				pLight->position = g_viewEntityParams.GetOrigin();
			}
			else if(bDoShadows)
			{
				// compute distance and disable shadowmap if needed
				float fDist = length(pLight->position - g_viewEntityParams.GetOrigin());

				if(fDist == 0)
					fDist = 1.0f;

				float fSphereSizeZ = pLight->radius.x / fDist;

				if(fSphereSizeZ < r_lightShadowDisableDistScale.GetFloat())
					bDoShadows = false;
			}

			// compute view, and rendering flags for each light
			if(nCubemapShot == 0 && pLight )
				ComputeLightViewsAndFlags(&lightParams, pLight, nShadowFlags);

			// setup this light
			materials->SetLight(pLight);

			if( bDoShadows && !r_skipShadowMaps.GetBool() )
			{
				sceneinfo_t lightSceneInfo = originalSceneInfo;
				lightSceneInfo.m_fZFar = pLight->radius.x;

				GR_LightShadow(pLight, &lightParams, lightSceneInfo, nShadowFlags, viewFrustum, viewMatrix, origin);

				nLightsShadowsRendered++;
			}

			// now light is dirty, destroy that flag
			if(pLight->nType == DLT_SUN)
				pLight->nFlags &= ~LFLAG_MATRIXSET;

			nLightsRendered++;

			// restore view
			viewrenderer->SetSceneInfo(originalSceneInfo);
			viewrenderer->SetView( &g_viewEntityParams );

			viewrenderer->SetDrawMode( VDM_LIGHTING );

			viewrenderer->SetViewportSize( viewportw, viewporth );
			
			// draw light in deferred
			viewrenderer->DrawDeferredCurrentLighting( bDoShadows );

			if( r_drawTransparentsForwardLighting.GetBool() && !(nRenderFlags & VR_FLAG_WATERREFLECTION) )
			{
				ITexture* pRenderTargets[MAX_MRTS] = { NULL };
				ITexture* pDepth = NULL;
				int nMRTs = 0;
				int nCubeFaces[MAX_MRTS] = { 0 };

				// first need get original render targets
				viewrenderer->GetCurrentRenderTargets(&nMRTs, pRenderTargets, &pDepth, nCubeFaces);

				//
				// now change the lighting model to forward and draw this light in forward shading
				//
				materials->SetCurrentLightingModel( MATERIAL_LIGHT_FORWARD );

				// lighting here comes to special texture to avoid errors and improve performance
				viewrenderer->SetupRenderTargets( 1, &g_pTransparentLighting );

				// it will setup textures
				viewrenderer->SetDrawMode( VDM_LIGHTING );

				// forward shading stage. Draw translucent objects lighting
				viewrenderer->DrawRenderList( g_transparentsList, nTransparentRenderFlags );

				// draw transparent model lighting
				viewrenderer->DrawRenderList( g_modelList, nTransparentRenderFlags | VR_FLAG_MODEL_LIST );

				// now get back to old render targets was set
				viewrenderer->SetupRenderTargets( nMRTs, pRenderTargets, pDepth, nCubeFaces );
			}

			materials->SetLight( NULL );
		}

		materials->SetCurrentLightingModel( MATERIAL_LIGHT_FORWARD );

		viewrenderer->SetSceneInfo(originalSceneInfo);

		viewrenderer->SetAreaList( g_pViewAreaList );

		// Draw main view in VDM_AMBIENT
		viewrenderer->SetDrawMode( VDM_AMBIENT );

		//viewrenderer->SetViewportSize(viewportw,viewporth);

		//viewrenderer->DrawWorld(nRenderFlags);
		if( r_drawTransparents.GetBool() )
			viewrenderer->DrawRenderList(g_transparentsList, nTransparentRenderFlags);

		// draw transparent models
		viewrenderer->DrawRenderList( g_modelList, nTransparentRenderFlags | VR_FLAG_MODEL_LIST );

		// now put here our Transparents lighting texture onto screen.

		if(nLightsRendered && r_drawTransparentsForwardLighting.GetBool() && !(nRenderFlags & VR_FLAG_WATERREFLECTION) )
		{
			IVector2D screen_size(g_pEngineHost->GetWindowSize());

			Vector2D size(screen_size.x, screen_size.y);
			materials->Setup2D(size.x, size.y);

			BlendStateParam_t blend;
			blend.srcFactor = BLENDFACTOR_ONE;
			blend.dstFactor = BLENDFACTOR_ONE;

			int vCount = 0;

			g_pShaderAPI->Reset();
			g_pShaderAPI->SetTexture( g_pTransparentLighting, 0);
			materials->SetBlendingStates( blend );
			materials->SetRasterizerStates(CULL_NONE,FILL_SOLID);
			materials->SetDepthStates(false,false);
			materials->Apply();

			Vertex2D_t* quad_sub = viewrenderer->GetViewSpaceMesh( &vCount );

			IMeshBuilder* pMeshBuilder = g_pShaderAPI->CreateMeshBuilder();

			pMeshBuilder->Begin(PRIM_TRIANGLES);
				for(int i = 0; i < vCount; i++)
				{
					pMeshBuilder->Position3fv(Vector3D(quad_sub[i].m_vPosition * (size-1.0f),0));
					pMeshBuilder->TexCoord2fv(quad_sub[i].m_vTexCoord);
					pMeshBuilder->Color4f(1.0,1.0,1.0,1.0);
					pMeshBuilder->AdvanceVertex();
				}
			pMeshBuilder->End();

			g_pShaderAPI->DestroyMeshBuilder(pMeshBuilder);

			//materials->DrawPrimitives2DFFP(PRIM_TRIANGLES, quad_sub, vCount, g_pTransparentLighting, color4_white, &blend);
		}
	}
	else
	{
		// simply draw transparency objects
		nRenderFlags = VR_FLAG_NO_OPAQUE | VR_FLAG_SKIPVISUPDATE;

		viewrenderer->SetSceneInfo(originalSceneInfo);

		// TEMPORARY
		//viewrenderer->ResetAreaVisibilityStates();
		viewrenderer->SetAreaList( g_pViewAreaList );
		//viewrenderer->UpdateAreaVisibility(nRenderFlags);

		// Draw main view in VDM_AMBIENT
		viewrenderer->SetDrawMode( VDM_AMBIENT );

		viewrenderer->SetViewportSize(viewportw,viewporth);

		//viewrenderer->DrawWorld(nRenderFlags);
		if(r_drawTransparents.GetBool())
			viewrenderer->DrawRenderList(g_transparentsList, nTransparentRenderFlags);

		viewrenderer->DrawRenderList(g_modelList, nTransparentRenderFlags | VR_FLAG_MODEL_LIST);
	}

	viewrenderer->BuildViewMatrices( viewportw, viewporth ,0 );
	viewrenderer->SetViewportSize( viewportw,viewporth );
	
	// don't draw particles in reflection
	if( !bDoWater)
	{
		// set view position for sorting effects
		effectrenderer->SetViewSortPosition(g_viewEntityParams.GetOrigin());

		//g_pJobList->Wait();
		GR_EffectRenderer();

		ColorRGBA amb = materials->GetAmbientColor();

		materials->SetAmbientColor(ColorRGBA(1,1,1,1));

		DrawParticleMaterialGroups();

		materials->SetAmbientColor(amb);
	}
	
	debugoverlay->Text(ColorRGBA(1), "Lights: %d\n", nLightsRendered);
	debugoverlay->Text(ColorRGBA(1), "Lights shadowmaps: %d\n", nLightsShadowsRendered);

	if(bMustShotCubemap && nCubemapShot < 5)
	{
		Msg("Face: %d\n", nCubemapShot+1);
		nCubemapShot++;
		g_pShaderAPI->Reset();
		goto reshot;
	}

	if(bMustShotCubemap && pCubemapTarget)
	{
		g_bShotCubemap = false;

		Msg("Saving cubemap to cubemap.dds\n");

		viewrenderer->SetupRenderTargetToBackbuffer();

		g_pShaderAPI->ChangeRenderTargetToBackBuffer();

		g_pShaderAPI->SaveRenderTarget(pCubemapTarget, "cubemap.dds");
		g_pShaderAPI->Finish();

		g_pShaderAPI->FreeTexture(pCubemapTarget);
		pCubemapTarget = NULL;

		g_pShaderAPI->FreeTexture(pCubemapDepthTarget);
		pCubemapDepthTarget = NULL;

		viewrenderer->SetView( &g_viewEntityParams );
	}
}

ConVar r_debugwaterreflection("r_debugWaterReflection", "0", "Shows water reflection texture", CV_CHEAT);

void GR_PostDraw()
{
	g_modelList->Clear();
	g_viewModels->Clear();

	if(r_debugwaterreflection.GetInt() > 0)
	{
		materials->Setup2D(g_pEngineHost->GetWindowSize().x, g_pEngineHost->GetWindowSize().y);

		Vector2D size(512,512);

		if(r_debugwaterreflection.GetInt() == 2)
		{
			size = Vector2D(g_pEngineHost->GetWindowSize().x, g_pEngineHost->GetWindowSize().y);
		}
	
		Vertex2D_t light_depth[] = { MAKETEXQUAD(0, 0, size.x, size.y, 0) };
		materials->DrawPrimitives2DFFP(PRIM_TRIANGLE_STRIP, light_depth, elementsOf(light_depth),g_pReflectionTexture);
	}

	// lighting here comes to special texture to avoid errors and improve performance
	viewrenderer->SetupRenderTargets( 1, &g_pTransparentLighting );
	g_pShaderAPI->Clear(true,false,false);
	viewrenderer->SetupRenderTargetToBackbuffer();
}

DECLARE_EFFECT_GROUP( MISC_EFFECTS )
{
	PRECACHE_PARTICLE_MATERIAL( "effects/sun");
	PRECACHE_PARTICLE_MATERIAL( "effects/sunflare");
}

ConVar r_waterTexturesSize("r_waterTexturesSize","512","Water reflection texture size", CV_ARCHIVE);

bool ViewRenderInitResources()
{
	// init game renderer
	viewrenderer->InitializeResources();

	// do it here
	materials->BeginPreloadMarker();

	g_pShaderAPI->ChangeRenderTargetToBackBuffer();
	g_pShaderAPI->Reset();

	if(!g_pFramebufferTexture)
	{
		g_pFramebufferTexture = g_pShaderAPI->CreateNamedRenderTarget("_rt_framebuffer", g_pEngineHost->GetWindowSize().x, g_pEngineHost->GetWindowSize().y, FORMAT_RGB8, TEXFILTER_LINEAR, ADDRESSMODE_CLAMP);
		g_pFramebufferTexture->Ref_Grab();
	}

	if(!g_pTransparentLighting)
	{
		g_pTransparentLighting = g_pShaderAPI->CreateNamedRenderTarget("_rt_transbuffer", g_pEngineHost->GetWindowSize().x, g_pEngineHost->GetWindowSize().y, FORMAT_RGB8, TEXFILTER_LINEAR, ADDRESSMODE_CLAMP);
		g_pTransparentLighting->Ref_Grab();
	}

	if(!g_pHDRBlur1Texture)
	{
		g_pHDRBlur1Texture = g_pShaderAPI->CreateNamedRenderTarget("_rt_hdrblur1", 512, 512, FORMAT_RGBA16F, TEXFILTER_LINEAR, ADDRESSMODE_CLAMP);
		g_pHDRBlur1Texture->Ref_Grab();
	}

	if(!g_pHDRBlur2Texture)
	{
		g_pHDRBlur2Texture = g_pShaderAPI->CreateNamedRenderTarget("_rt_hdrblur2", 512, 512, FORMAT_RGBA16F, TEXFILTER_LINEAR, ADDRESSMODE_CLAMP);
		g_pHDRBlur2Texture->Ref_Grab();
	}

	if(!g_pUnderwaterTexture)
	{
		g_pUnderwaterTexture = g_pShaderAPI->CreateNamedRenderTarget("_rt_underwater", r_waterTexturesSize.GetInt(), r_waterTexturesSize.GetInt(), FORMAT_RGB8, TEXFILTER_LINEAR, ADDRESSMODE_CLAMP);
		g_pUnderwaterTexture->Ref_Grab();
	}

	if(!g_pReflectionTexture)
	{
		g_pReflectionTexture = g_pShaderAPI->CreateNamedRenderTarget("_rt_reflection", r_waterTexturesSize.GetInt(), r_waterTexturesSize.GetInt(), FORMAT_RGB8, TEXFILTER_LINEAR, ADDRESSMODE_CLAMP);
		g_pReflectionTexture->Ref_Grab();
	}

	g_pHDRBlur1	= materials->FindMaterial("engine/postprocess/hdr_blurfilter1", false);
	g_pHDRBlur2	= materials->FindMaterial("engine/postprocess/hdr_blurfilter2", false);
	g_pHDRBloom	= materials->FindMaterial("engine/postprocess/hdr_bloom", true);

	materials->EndPreloadMarker();
	materials->Wait();

	return true;
}

bool ViewShutdownResources()
{
	// shutdown resources
	viewrenderer->ShutdownResources();

	g_pShaderAPI->ChangeRenderTargetToBackBuffer();
	g_pShaderAPI->Reset();

	// remove render target
	g_pShaderAPI->FreeTexture(g_pFramebufferTexture);
	g_pFramebufferTexture = NULL;

	g_pShaderAPI->FreeTexture(g_pTransparentLighting);
	g_pTransparentLighting = NULL;

	g_pShaderAPI->FreeTexture(g_pHDRBlur1Texture);
	g_pHDRBlur1Texture = NULL;

	g_pShaderAPI->FreeTexture(g_pHDRBlur2Texture);
	g_pHDRBlur2Texture = NULL;

	// remove render target
	g_pShaderAPI->FreeTexture(g_pUnderwaterTexture);
	g_pUnderwaterTexture = NULL;

	// remove render target
	g_pShaderAPI->FreeTexture(g_pReflectionTexture);
	g_pReflectionTexture = NULL;

	materials->FreeMaterial(g_pHDRBlur1);
	materials->FreeMaterial(g_pHDRBlur2);
	materials->FreeMaterial(g_pHDRBloom);

	g_pHDRBlur1 = NULL;
	g_pHDRBlur2 = NULL;
	g_pHDRBloom = NULL;

	return true;
}

void GR_Init()
{
	g_rope_verts.clear();
	g_rope_indices.clear();

	//g_pDetailSystem->Init();

	materials->AddDestroyLostCallbacks(ViewShutdownResources,ViewRenderInitResources);

	ViewRenderInitResources();

	InitParticleBuffers();

	PrecacheEffects();

	FogInfo_t nofog;
	nofog.enableFog	= false;

	materials->SetFogInfo(nofog);

	eqlevel->UpdateStaticDecals();

	// make area list
	g_pViewAreaList = eqlevel->CreateRenderAreaList();
	g_pLightViewAreaList = eqlevel->CreateRenderAreaList();

	int nTransparents = eqlevel->GetTranslucentObjectCount();
	CBaseRenderableObject** pObjectList = eqlevel->GetTranslucentObjectRenderables();

	for(int i = 0; i < nTransparents; i++)
		g_transparentsList->AddRenderable( pObjectList[i] );

	g_pJobList = new CEqJobList();
}

void GR_SetupDynamicsIBO()
{
	g_pShaderAPI->Reset(STATE_RESET_VBO);

	g_pShaderAPI->SetVertexFormat( g_pRopeFormat );
	g_pShaderAPI->SetVertexBuffer( g_pRopeVB, 0 );
	g_pShaderAPI->SetIndexBuffer( g_pRopeIB );
}

void GR_InitDynamics()
{
	int nStaticDecals = eqlevel->GetStaticDecalCount();
	CBaseRenderableObject** pObjectList = eqlevel->GetStaticDecalRenderables();

	for(int i = 0; i < nStaticDecals; i++)
		g_decalList->AddRenderable( pObjectList[i] );

	// init ropes
	if(g_rope_verts.numElem() > 0 && g_rope_indices.numElem() > 0)
	{
		if(!g_pRopeFormat)
		{
			VertexFormatDesc_t pRopeFormat[] = {
				{ 0, 3, VERTEXTYPE_VERTEX, ATTRIBUTEFORMAT_FLOAT },	  // position
				{ 0, 3, VERTEXTYPE_TEXCOORD, ATTRIBUTEFORMAT_FLOAT }, // TexCoord with bone index
				{ 0, 3, VERTEXTYPE_TEXCOORD, ATTRIBUTEFORMAT_FLOAT }, // Normal
			};

			g_pRopeFormat = g_pShaderAPI->CreateVertexFormat(pRopeFormat, elementsOf(pRopeFormat));
		}

		g_pRopeVB = g_pShaderAPI->CreateVertexBuffer(BUFFER_STATIC, g_rope_verts.numElem(), sizeof(eqropevertex_t), g_rope_verts.ptr());
		g_pRopeIB = g_pShaderAPI->CreateIndexBuffer(g_rope_indices.numElem(), sizeof(uint16), BUFFER_STATIC, g_rope_indices.ptr());
	}
}

void GR_Cleanup()
{
	g_decalList->Clear();

	if(g_pRopeFormat)
		g_pShaderAPI->DestroyVertexFormat(g_pRopeFormat);

	if(g_pRopeIB)
		g_pShaderAPI->DestroyIndexBuffer(g_pRopeIB);

	if(g_pRopeVB)
		g_pShaderAPI->DestroyVertexBuffer(g_pRopeVB);

	g_pRopeVB = NULL;
	g_pRopeIB = NULL;
	g_pRopeFormat = NULL;

	g_rope_verts.clear();
	g_rope_indices.clear();
}

void GR_Shutdown()
{
	//g_pDetailSystem->ClearNodes();

	materials->RemoveLostRestoreCallbacks(ViewShutdownResources,ViewRenderInitResources);

	UnloadEffects();

	ClearParticleCache();
	ShutdownParticleBuffers();
	ViewShutdownResources();

	eqlevel->DestroyRenderAreaList( g_pViewAreaList );
	eqlevel->DestroyRenderAreaList( g_pLightViewAreaList );

	g_pViewAreaList = NULL;
	g_pLightViewAreaList = NULL;
	g_transparentsList->Clear();
	GR_Cleanup();

	delete g_pJobList;
	g_pJobList = NULL;

	g_pCurrentWaterInfo = NULL;
}

// post-rendering
CFilter*	g_pCurrFilter = NULL;
CFilter*	g_pPrevFilter = NULL;
float		g_fFilterTransition = 0.0f;	// this is veeery slow thing

void GR_Draw2DMaterial(IMaterial* pMaterial)
{
	g_pShaderAPI->Reset( STATE_RESET_VBO );

	materials->Setup2D(g_pEngineHost->GetWindowSize().x, g_pEngineHost->GetWindowSize().y);

	materials->SetCullMode(CULL_FRONT);

	materials->BindMaterial( pMaterial, false );

	materials->Apply();

	int vCount = 0;

	Vertex2D_t* quad_sub = viewrenderer->GetViewSpaceMesh( &vCount );

	Vector2D screen_size(g_pEngineHost->GetWindowSize().x, g_pEngineHost->GetWindowSize().y);

	IMeshBuilder* pMeshBuilder = g_pShaderAPI->CreateMeshBuilder();

	pMeshBuilder->Begin(PRIM_TRIANGLES);
		for(int i = 0; i < vCount; i++)
		{
			pMeshBuilder->Position3fv(Vector3D(quad_sub[i].m_vPosition * (screen_size-1.0f),0));
			pMeshBuilder->TexCoord2fv(quad_sub[i].m_vTexCoord);
			pMeshBuilder->AdvanceVertex();
		}
	pMeshBuilder->End();

	g_pShaderAPI->DestroyMeshBuilder(pMeshBuilder);
}

void GR_DrawFilters()
{
	// image post-processing
	if(!r_skipPostProcessFilters.GetBool())
	{
		if(g_pCurrFilter)
		{
			// transition the selected filters
			g_pCurrFilter->Render( 1.0f );
		}

		// HDR
		if(g_pHDRBlur1)
		{
			GR_CopyFramebuffer();
			g_pShaderAPI->ChangeRenderTarget( g_pHDRBlur1Texture, 0, NULL );
			g_pShaderAPI->Clear(true,false,false);
			GR_Draw2DMaterial( g_pHDRBlur1 );
			g_pShaderAPI->ChangeRenderTarget( g_pHDRBlur2Texture, 0, NULL );
			g_pShaderAPI->Clear(true,false,false);
			GR_Draw2DMaterial( g_pHDRBlur2 );
			g_pShaderAPI->ChangeRenderTargetToBackBuffer();

			GR_Draw2DMaterial( g_pHDRBloom );
		}
	}
}

void GR_SetupScene()
{
	sceneinfo_t scene_info;

	scene_info.m_fZNear = 1.0f;

	if(r_zfar.GetInt() == -1)
		scene_info.m_fZFar = g_pWorldInfo->GetLevelRenderableDist();
	else
		scene_info.m_fZFar = r_zfar.GetFloat();
	
	scene_info.m_cAmbientColor = r_ambientadd.GetFloat() + g_pWorldInfo->GetAmbient()*r_ambientscale.GetFloat();

	viewrenderer->SetSceneInfo(scene_info);

	if(fog_override.GetBool())
	{
		FogInfo_t overrridefog;

		overrridefog.enableFog	= fog_enable.GetBool();
		overrridefog.fogColor	= Vector3D(fog_color_r.GetFloat(),fog_color_g.GetFloat(),fog_color_b.GetFloat());
		overrridefog.fogfar		= fog_far.GetFloat();
		overrridefog.fognear	= fog_near.GetFloat();

		materials->SetFogInfo(overrridefog);
	}
}

void GR_Postrender()
{
	viewrenderer->SetView( &g_viewEntityParams );

	// render post-process
	GR_DrawFilters();

	// TODO: HUD

	if(!g_bShotCubemap)
	{
		viewrenderer->UpdateLights(gpGlobals->frametime);
	}
}

void GR_CopyFramebuffer()
{
	// copy.
	g_pShaderAPI->CopyFramebufferToTexture( g_pFramebufferTexture );
}

void GR_PreloadEntityMaterials(BaseEntity* pEnt, void* userData)
{
	if(pEnt->GetModel() == NULL)
		return;

	pEnt->UpdateRenderStuff();

	int* rooms = (int*)userData;

	for(int i = 0; i < pEnt->m_nRooms; i++)
	{
		if(pEnt->m_pRooms[i] == rooms[0] || pEnt->m_pRooms[i] == rooms[1])
			pEnt->GetModel()->LoadMaterials();
	}
}

void GR_SetViewEntity(BaseEntity* pEnt)
{
	int rooms[2] = {-1,-1};
	eqlevel->GetRoomsForPoint(pEnt->GetAbsOrigin(), rooms);

	eqlevel->LoadRoomMaterials( rooms[0] );
	eqlevel->LoadRoomMaterials( rooms[1] );

	// go through entities and preload their materials
	UTIL_DoForEachEntity( GR_PreloadEntityMaterials, rooms);

	g_pViewEntity = pEnt;
}

void GR_DrawBillboard(PFXBillboard_t* effect)
{
	Effects_DrawBillboard(effect, viewrenderer->GetView(), viewrenderer->GetViewFrustum());
}