//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: UI elements
//////////////////////////////////////////////////////////////////////////////////

#include "DrvSynHUD.h"
#include "AIManager.h"
#include "heightfield.h"
#include "replay.h"

#include "input.h"

#include "director.h"

#include "world.h"

#define DEFAULT_HUD_SCHEME "resources/hud/defaulthud.res"

static CDrvSynHUDManager s_drvSynHUDManager;
CDrvSynHUDManager* g_pGameHUD = &s_drvSynHUDManager;

ConVar r_drawHUD("hud_draw", "1", NULL, CV_ARCHIVE);

ConVar hud_mapZoom("hud_mapZoom", "1.0", NULL, CV_ARCHIVE);

ConVar hud_debug_car("hud_debug_car", "0", NULL, CV_CHEAT);

ConVar hud_debug_roadmap("hud_debug_roadmap", "0", NULL, CV_CHEAT);

ConVar hud_show_controls("hud_show_controls", "0", NULL, CV_ARCHIVE);

ConVar hud_map_pos("hud_map_pos", "0", "Map position (0 - bottom, 1 - top)", CV_ARCHIVE);

ConVar g_showCameraPosition("g_showCameraPosition", "0", NULL, CV_CHEAT);
ConVar g_showCarPosition("g_showCarPosition", "0", NULL, CV_CHEAT);

DECLARE_CMD(hud_showLastMessage, NULL, 0)
{
	g_pGameHUD->ShowLastMessage();
}

//----------------------------------------------------------------------------------

CDrvSynHUDManager::CDrvSynHUDManager()
	: m_handleCounter(0), m_mainVehicle(nullptr), m_lightsTime(0.0f), m_mapTexture(nullptr), m_showMap(true), m_screenAlertTime(0.0f), m_screenMessageTime(0.0f), 
		m_hudLayout(nullptr), m_hudDamageBar(nullptr), 	m_hudFelonyBar(nullptr), m_hudMap(nullptr), m_hudTimer(nullptr), 
		m_fadeValue(0.0f), m_fadeCurtains(false), m_fadeTarget(false),
		m_blurAccumMat(nullptr), m_blurMat(nullptr), m_framebufferTex(nullptr), m_blurAccumTex(nullptr)
{
}

void CDrvSynHUDManager::Init()
{
	m_enable = true;
	m_enableInReplay = false;

	EqString mapTexName("levelmap/");
	mapTexName.Append( g_pGameWorld->GetLevelName() );

	m_mapTexture = g_pShaderAPI->LoadTexture( mapTexName.c_str() , TEXFILTER_LINEAR, TEXADDRESS_CLAMP, TEXFLAG_NOQUALITYLOD );

	if( m_mapTexture )
	{
		if(!stricmp(m_mapTexture->GetName(), "error"))
			m_mapTexture = NULL;
		else
			m_mapTexture->Ref_Grab();

		m_mapRenderTarget = g_pShaderAPI->CreateNamedRenderTarget("_mapRender",256, 256, FORMAT_RGBA8, TEXFILTER_LINEAR, TEXADDRESS_CLAMP);
		m_mapRenderTarget->Ref_Grab();
	}

	m_showMap = (m_mapTexture != NULL);

	m_handleCounter = 0;
	m_lightsTime = 0.0f;

	m_screenMessageTime = 0.0f;
	m_screenMessageText.Clear();

	m_screenAlertType = HUD_ALERT_NORMAL;
	m_screenAlertTime = 0.0f;
	m_screenAlertInTime = 0.0f;
	m_screenAlertText.Clear();

	m_fadeTarget = 0.0f;
	m_fadeCurtains = false;
	m_fadeValue = 0.0f;
	m_showMotionBlur = false;

	if (!m_framebufferTex)
	{
		m_framebufferTex = g_pShaderAPI->CreateNamedRenderTarget("_hud_framebuffer", 512, 512, FORMAT_RGB8, TEXFILTER_LINEAR, TEXADDRESS_CLAMP);
		m_framebufferTex->Ref_Grab();
	}

	if (!m_blurAccumTex)
	{
		m_blurAccumTex = g_pShaderAPI->CreateNamedRenderTarget("_rt_blur", 512, 512, FORMAT_RGB8, TEXFILTER_LINEAR, TEXADDRESS_CLAMP);
		m_blurAccumTex->Ref_Grab();
	}

	if (!m_blurAccumMat)
	{
		kvkeybase_t blurAccumulatorMat;
		blurAccumulatorMat.SetName("BaseUnlit");
		blurAccumulatorMat.SetKey("basetexture", "_hud_framebuffer");
		blurAccumulatorMat.SetKey("translucent", true);
		blurAccumulatorMat.SetKey("color", "[1 1 1 0.1]");
		blurAccumulatorMat.SetKey("ztest", false);
		blurAccumulatorMat.SetKey("zwrite", false);

		m_blurAccumMat = materials->CreateMaterial("_hudBlurAccum", &blurAccumulatorMat);
		m_blurAccumMat->Ref_Grab();
	}

	if (!m_blurMat)
	{
		kvkeybase_t blurMat;
		blurMat.SetName("BaseUnlit");
		blurMat.SetKey("basetexture", "_rt_blur");
		blurMat.SetKey("translucent", true);
		blurMat.SetKey("color", "[1.25 1.25 1.25 0.6]");
		blurMat.SetKey("ztest", false);
		blurMat.SetKey("zwrite", false);

		m_blurMat = materials->CreateMaterial("_hudMotionBlur", &blurMat);
		m_blurMat->Ref_Grab();
	}

	m_radarBlank = 0.0f;

	m_felonyTok = g_localizer->GetToken("HUD_FELONY_TITLE");

	// init hud layout
	m_hudLayout = equi::Manager->CreateElement("HudElement");

	int hudModelCacheId = PrecacheStudioModel("/models/misc/hud_models.egf");
	m_hudObjectDummy.SetModel("/models/misc/hud_models.egf");
}

void CDrvSynHUDManager::RenderBlur(const IVector2D& screenSize)
{
	materials->SetCullMode(CULL_FRONT);

	g_pShaderAPI->CopyFramebufferToTexture(m_framebufferTex);
	g_pShaderAPI->ChangeRenderTarget(m_blurAccumTex);

	materials->BindMaterial(m_blurAccumMat);

	CMeshBuilder meshBuilder(materials->GetDynamicMesh());

	Rectangle_t screenRect(0,0, screenSize.x, screenSize.y);

	Rectangle_t texCoords(0, 0, 1, 1);
	if (g_pShaderAPI->GetShaderAPIClass() == SHADERAPI_OPENGL)
		texCoords.FlipY();

	// render blur to accumulator texture
	meshBuilder.Begin(PRIM_TRIANGLE_STRIP);
		meshBuilder.TexturedQuad2(screenRect.GetRightTop(), screenRect.GetLeftTop(), screenRect.GetRightBottom(), screenRect.GetLeftBottom(),
								  texCoords.GetRightTop(), texCoords.GetLeftTop(), texCoords.GetRightBottom(), texCoords.GetLeftBottom());
	meshBuilder.End();

	g_pShaderAPI->ChangeRenderTargetToBackBuffer();

	// render accumulated texture on screen
	materials->BindMaterial(m_blurMat);

	meshBuilder.Begin(PRIM_TRIANGLE_STRIP);
		meshBuilder.TexturedQuad2(screenRect.GetRightTop(), screenRect.GetLeftTop(), screenRect.GetRightBottom(), screenRect.GetLeftBottom(),
			texCoords.GetRightTop(), texCoords.GetLeftTop(), texCoords.GetRightBottom(), texCoords.GetLeftBottom());
	meshBuilder.End();
}

void InitHUDBaseKeyValues_r(kvkeybase_t* hudKVs, kvkeybase_t* baseKVs)
{
	for(int i = 0; i < baseKVs->keys.numElem(); i++)
	{
		kvkeybase_t* src = baseKVs->keys[i];

		kvkeybase_t* dst = hudKVs->FindKeyBase(src->name);

		if(dst == NULL)
			dst = hudKVs->AddKeyBase(src->name);

		src->CopyTo(dst);

		// go to next in hierarchy
		InitHUDBaseKeyValues_r(src, dst);
	}
}

void CDrvSynHUDManager::SetHudScheme(const char* name)
{
	if(!m_schemeName.CompareCaseIns(name))
		return;

	MsgInfo("Loading HUD scheme '%s'...\n", name);

	m_schemeName = name;
	EqString baseHudPath(m_schemeName.Path_Strip_Name());

	m_hudDamageBar = nullptr;
	m_hudFelonyBar = nullptr;
	m_hudMap = nullptr;
	m_hudTimer = nullptr;

	kvkeybase_t hudKvs;
	kvkeybase_t* baseHud = nullptr;

	if( KV_LoadFromFile(m_schemeName.c_str(), SP_MOD, &hudKvs) )
	{
		kvkeybase_t* hudBasePath = hudKvs.FindKeyBase("base");
		if(hudBasePath)
		{
			EqString hudPath(CombinePath(2, baseHudPath.c_str(), KV_GetValueString(hudBasePath)));
			baseHud = KV_LoadFromFile(hudPath.c_str(), SP_MOD);

			if(!baseHud)
			{
				MsgError("Can't open HUD scheme '%s'\n", hudPath.c_str());
			}
		}

		if(baseHud)
		{
			m_hudLayout->InitFromKeyValues( baseHud );
			m_hudLayout->InitFromKeyValues( &hudKvs, true );
		}
		else
			m_hudLayout->InitFromKeyValues( &hudKvs );

		delete baseHud;
	}
	else
		MsgError("Failed to load HUD scheme\n");

	m_hudDamageBar = m_hudLayout->FindChild("damageBar");
	m_hudFelonyBar = m_hudLayout->FindChild("felonyLabel");
	m_hudMap = m_hudLayout->FindChild("map");

	equi::IUIControl* timerControl = m_hudLayout->FindChild("timer");

	// validate it
	m_hudTimer = equi::DynamicCast<equi::DrvSynTimerElement>(timerControl);
}

void CDrvSynHUDManager::InvalidateObjects()
{
	/*
	for(hudDisplayObjIterator_t iterator = m_displayObjects.begin(); iterator != m_displayObjects.end(); iterator++)
	{
		if(iterator->second.object)
			iterator->second.object->Ref_Drop();
	}
	*/

	m_displayObjects.clear();
	SetDisplayMainVehicle(nullptr);
}

void CDrvSynHUDManager::Cleanup()
{
	g_pShaderAPI->FreeTexture( m_mapTexture );
	m_mapTexture = nullptr;

	g_pShaderAPI->FreeTexture(m_mapRenderTarget);
	m_mapRenderTarget = nullptr;

	g_pShaderAPI->FreeTexture(m_framebufferTex);
	m_framebufferTex = nullptr;

	g_pShaderAPI->FreeTexture(m_blurAccumTex);
	m_blurAccumTex = nullptr;

	materials->FreeMaterial(m_blurAccumMat);
	m_blurAccumMat = nullptr;

	materials->FreeMaterial(m_blurMat);
	m_blurMat = nullptr;

	InvalidateObjects();

	delete m_hudLayout;
	m_hudLayout = nullptr;

	m_schemeName.Empty();

	m_hudObjectDummy.SetModelPtr(nullptr);
}

void CDrvSynHUDManager::ShowMessage( const char* token, float time )
{
	m_screenMessageText = LocalizedString(token);

	m_screenMessageTime = time;

	if(time != 0.0f && time <= 1.0f)
		m_screenMessageTime += 0.5f;
}

void CDrvSynHUDManager::ShowLastMessage()
{
	m_screenMessageTime = 2.0f;
}

void CDrvSynHUDManager::ShowAlert( const char* token, float time, int type )
{
	m_screenAlertText = LocalizedString(token);
	m_screenAlertText = m_screenAlertText.UpperCase();

	m_screenAlertTime = time;
	m_screenAlertInTime = 1.0f;

	m_screenAlertType = (EScreenAlertType)type;
}

void CDrvSynHUDManager::SetTimeDisplay(bool enabled, double time)
{
	if (m_hudTimer)
	{
		m_hudTimer->SetTimeValue(time);
		m_hudTimer->SetVisible(enabled);
	}
}

void CDrvSynHUDManager::FadeIn( bool useCurtains, float time)
{
	m_fadeValue = time;
	m_fadeTarget = -time;
	m_fadeCurtains = useCurtains;
}

void CDrvSynHUDManager::FadeOut(bool useCurtains, float time)
{
	m_fadeValue = 0.0f;
	m_fadeTarget = time;
	m_fadeCurtains = useCurtains;
}

equi::IUIControl* CDrvSynHUDManager::FindChildElement(const char* name) const
{
	return m_hudLayout->FindChild(name);
}

equi::IUIControl* CDrvSynHUDManager::GetRootElement() const
{
	return m_hudLayout;
}

void CDrvSynHUDManager::DrawDamageBar(CMeshBuilder& meshBuilder, Rectangle_t& rect, float percentage, float alpha)
{
	meshBuilder.Color4f(0.435, 0.435, 0.435, 0.35f*alpha);
	meshBuilder.Quad2(rect.GetRightTop(), rect.GetLeftTop(), rect.GetRightBottom(), rect.GetLeftBottom());

	if(percentage > 0)
	{
		Rectangle_t fillRect(rect.vleftTop, Vector2D(lerp(rect.vleftTop.x, rect.vrightBottom.x, percentage), rect.vrightBottom.y));

		ColorRGBA damageColor = lerp(ColorRGBA(0,0.6f,0,alpha), ColorRGBA(0.6f,0,0,alpha), percentage) * 1.5f;

		if (percentage > 0.85f)
		{
			float flashValue = sin(m_lightsTime*16.0f) * 5.0f;
			flashValue = clamp(flashValue, 0.0f, 1.0f);

			damageColor = lerp(ColorRGBA(0.5f, 0.0f, 0.0f, alpha), ColorRGBA(0.8f, 0.2f, 0.2f, alpha), flashValue) * 1.5f;
		}

		// draw damage bar foreground
		meshBuilder.Color4fv(damageColor);
		meshBuilder.Quad2(fillRect.GetRightTop(), fillRect.GetLeftTop(), fillRect.GetRightBottom(), fillRect.GetLeftBottom());
	}
}

void CDrvSynHUDManager::DrawWorldIntoMap(const CViewParams& params, float fDt)
{
	CMeshBuilder meshBuilder(materials->GetDynamicMesh());

	IRectangle mapRectangle = m_hudMap->GetClientRectangle();
	Vector2D mapRectSize = mapRectangle.GetSize();

	Matrix4x4 proj,view;
	params.GetMatrices(proj, view, mapRectSize.x, mapRectSize.y, 0.1f, 900.0f, false);

	materials->SetMatrix(MATRIXMODE_PROJECTION, proj);
	materials->SetMatrix(MATRIXMODE_VIEW, view);
	materials->SetMatrix(MATRIXMODE_WORLD, identity4());

	Vector2D imgSize(1.0f);

	if (m_mapTexture)
		imgSize = Vector2D(m_mapTexture->GetWidth(), m_mapTexture->GetHeight());

	Vector2D imgToWorld = imgSize * HFIELD_POINT_SIZE;
	Vector2D imgHalf = imgToWorld * 0.5f;

	BlendStateParam_t blending;
	blending.srcFactor = BLENDFACTOR_SRC_ALPHA;
	blending.dstFactor = BLENDFACTOR_ONE_MINUS_SRC_ALPHA;

	BlendStateParam_t additiveBlend;
	additiveBlend.srcFactor = BLENDFACTOR_ONE;
	additiveBlend.dstFactor = BLENDFACTOR_ONE;

	RasterizerStateParams_t raster;
	raster.scissor = false;
	raster.cullMode = CULL_NONE;

	g_pShaderAPI->SetTexture(m_mapTexture, nullptr, 0);
	materials->SetBlendingStates(blending);
	materials->SetRasterizerStates(raster);
	materials->SetDepthStates(false, false);
	materials->BindMaterial(materials->GetDefaultMaterial());

	Vertex2D_t mapVerts[] = { MAKETEXQUAD(imgHalf.x, -imgHalf.y, -imgHalf.x, imgHalf.y, 0) };

#define MAKE_3D(v) Vector3D(v.x,0.0f,v.y)

	// draw the map rectangle
	meshBuilder.Begin(PRIM_TRIANGLES);
		meshBuilder.Color4f(1, 1, 1, 0.75f);
		meshBuilder.TexturedQuad3(MAKE_3D(mapVerts[0].position), MAKE_3D(mapVerts[1].position), MAKE_3D(mapVerts[2].position), MAKE_3D(mapVerts[3].position),
			mapVerts[0].texCoord, mapVerts[1].texCoord, mapVerts[2].texCoord, mapVerts[3].texCoord);
	meshBuilder.End();

	Vector3D mainVehiclePos = m_mainVehicle ? m_mainVehicle->GetOrigin() * Vector3D(1.0f, 0.0f, 1.0f) : params.GetOrigin();

	bool mainVehicleInPursuit = (m_mainVehicle ? m_mainVehicle->GetPursuedCount() > 0 : false);
	bool mainVehicleHasFelony = (m_mainVehicle ? m_mainVehicle->GetFelony() >= 0.1f : false);

	g_pShaderAPI->SetTexture(nullptr, nullptr, 0);
	materials->BindMaterial(materials->GetDefaultMaterial());

	meshBuilder.Begin(PRIM_TRIANGLE_STRIP);

		// display cop cars on map
		for(int i = 0; i < g_pAIManager->m_trafficCars.numElem(); i++)
		{
			CAIPursuerCar* pursuer = UTIL_CastToPursuer(g_pAIManager->m_trafficCars[i]);

			if(!pursuer)
				continue;

			if(!pursuer->IsAlive() || !pursuer->IsEnabled())
				continue;

			// don't reveal cop position
			if(mainVehicleInPursuit && !pursuer->InPursuit())
				continue;

			Vector3D cop_forward = pursuer->GetForwardVector();

			float Yangle = -RAD2DEG(atan2f(cop_forward.z, cop_forward.x));

			Vector3D copPos = (pursuer->GetOrigin() + pursuer->GetForwardVector()) * Vector3D(1.0f,0.0f,1.0f);

			meshBuilder.Color4fv(1.0f);
			meshBuilder.Position3fv(copPos);

			int vtxIdx = meshBuilder.AdvanceVertexIndex();

			bool inPursuit = pursuer->InPursuit();

			bool hasAttention = (mainVehicleHasFelony || pursuer->GetPursuerType() == PURSUER_TYPE_GANG);

			// draw cop car frustum
			float size = hasAttention ? AI_COPVIEW_FAR_WANTED : AI_COPVIEW_FAR;
			float angFac = hasAttention ? 22.0f : 12.0f;
			float angOffs = hasAttention ? 0.0f : 45.0f;

			meshBuilder.Color4f(1,1,1,0.0f);
			for(int j = 0; j < 8; j++)
			{
				float ss,cs;
				float angle = (float(j)+0.5f) * angFac + Yangle + angOffs;
				SinCos(DEG2RAD(angle), &ss, &cs);

				if(j < 1 || j > 6)
					meshBuilder.Position3fv(copPos + Vector3D(ss, 0.0f, cs)*size*0.35f);
				else
					meshBuilder.Position3fv(copPos + Vector3D(ss, 0.0f, cs)*size);

				if(j > 0)
					meshBuilder.AdvanceVertexIndex(vtxIdx);

				meshBuilder.AdvanceVertexIndex(vtxIdx+j+1);
			}

			meshBuilder.AdvanceVertexIndex(0xFFFF);

			// draw dot also
			{
				float colorValue = inPursuit ? sin(m_lightsTime*16.0) : 0.0f;
				colorValue = clamp(colorValue*100.0f, 0.0f, 1.0f);

				meshBuilder.Color4fv(ColorRGBA(ColorRGB(colorValue), 1.0f));
				meshBuilder.Position3fv(copPos);
				int firstVert = meshBuilder.AdvanceVertexIndex();

				for(int d = 0; d < 11; d++)
				{
					float ss,cs;
					float angle = float(d-1) * 36.0f;
					SinCos(DEG2RAD(angle), &ss, &cs);

					meshBuilder.Position3fv( copPos + Vector3D(ss, 0.0f, cs)*6.0f );

					if(d > 0)
						meshBuilder.AdvanceVertexIndex( firstVert );

					meshBuilder.AdvanceVertexIndex( firstVert+d+1 );
				}

				meshBuilder.AdvanceVertexIndex(0xFFFF);
			}
		}
		
		for(hudDisplayObjIterator_t iterator = m_displayObjects.begin(); iterator != m_displayObjects.end(); iterator++)
		{
			hudDisplayObject_t& obj = iterator->second;

			if(!(obj.flags & HUD_DOBJ_IS_TARGET))
				continue;

			Vector3D objPos = (obj.object ? obj.object->GetOrigin() : obj.point) * Vector3D(1.0f, 0.0f,1.0f);

			// draw fading out arrow
			{
				ColorRGBA arrowCol1(0.0f,0.0f,0.0f,0.5f);
				ColorRGBA arrowCol2(0.0f,0.0f,0.0f,0.15f);

				meshBuilder.Color4fv(arrowCol1);
				meshBuilder.Position3fv(objPos);
				int firstVert = meshBuilder.AdvanceVertexIndex();

				Vector3D targetDir(objPos - mainVehiclePos);
				Vector3D ntargetDir(normalize(targetDir));
				Vector3D targetPerpendicular(-ntargetDir.z, 0.0f, ntargetDir.x);

				float distToTarg = length(targetDir);

				meshBuilder.Color4fv(arrowCol2);

				meshBuilder.Position3fv(mainVehiclePos - targetPerpendicular * distToTarg * 0.15f);
				meshBuilder.AdvanceVertexIndex(firstVert+1);

				meshBuilder.Position3fv(mainVehiclePos + targetPerpendicular * distToTarg * 0.15f);
				meshBuilder.AdvanceVertexIndex(firstVert+2);

				meshBuilder.AdvanceVertexIndex(firstVert+2);

				meshBuilder.AdvanceVertexIndex(0xFFFF);
			}

			// draw flashing point
			meshBuilder.Color4fv(1.0f);
			meshBuilder.Position3fv(objPos);
			int firstVert = meshBuilder.AdvanceVertexIndex();

			meshBuilder.Color4f(1.0f, 1.0f, 1.0f, 0.0f);
			for(int i = 0; i < 11; i++)
			{
				float ss,cs;
				float angle = float(i-1) * 36.0f;
				SinCos(DEG2RAD(angle), &ss, &cs);

				meshBuilder.Position3fv( objPos + Vector3D(ss, 0.0f, cs) * obj.flashValue * 30.0f );

				if(i > 0)
					meshBuilder.AdvanceVertexIndex( firstVert );

				meshBuilder.AdvanceVertexIndex( firstVert+i+1 );
			}

			meshBuilder.AdvanceVertexIndex(0xFFFF);
		}
		
		// draw player car dot
		{
			meshBuilder.Color4f(0,0,0,1);
			meshBuilder.Position3fv(mainVehiclePos);
			int firstVert = meshBuilder.AdvanceVertexIndex();

			//meshBuilder.Color4fv(0.0f);
			for(int i = 0; i < 11; i++)
			{
				float ss,cs;
				float angle = float(i-1) * 36.0f;
				SinCos(DEG2RAD(angle), &ss, &cs);

				meshBuilder.Position3fv(mainVehiclePos + Vector3D(ss, 0.0f, cs)*10.0f );

				if(i > 0)
					meshBuilder.AdvanceVertexIndex( firstVert );

				meshBuilder.AdvanceVertexIndex( firstVert+i+1 );
			}

			meshBuilder.AdvanceVertexIndex(0xFFFF);
		}
		
	meshBuilder.End();
}

void CDrvSynHUDManager::Render3D( float fDt )
{
	bool replayHud = (g_replayTracker->m_state == REPL_PLAYING);

	if (m_enable && !replayHud)
	{
		CViewParams& camera = *g_pGameWorld->GetView();

		float viewRotation = camera.GetAngles().y + 180.0f;

		for (hudDisplayObjIterator_t iterator = m_displayObjects.begin(); iterator != m_displayObjects.end(); iterator++)
		{
			hudDisplayObject_t& obj = iterator->second;

			Vector3D markPos = obj.point;

			CGameObject* gameObj = obj.object;

			if (gameObj && (gameObj->ObjType() == GO_CAR || gameObj->ObjType() == GO_CAR_AI))
			{
				CCar* car = (CCar*)gameObj;

				markPos = car->GetOrigin() + vec3_up * car->m_conf->cameraConf.height;
			}

			markPos += vec3_up * sinf(obj.flashValue * PI_F);

			if (obj.flags & HUD_DOBJ_IS_TARGET)
			{
				m_hudObjectDummy.SetAngles(Vector3D(0.0f, viewRotation, 0.0f));
				m_hudObjectDummy.SetOrigin(markPos);
				m_hudObjectDummy.Simulate(0.0f);

				m_hudObjectDummy.Draw(0);
			}
		}
	}
}

// render the screen with maps and shit
void CDrvSynHUDManager::Render( float fDt, const IVector2D& screenSize)
{
	bool replayHud = (g_replayTracker->m_state == REPL_PLAYING);

	bool hudEnabled =	(m_screenAlertType != HUD_ALERT_DANGER) && 
						m_enable && 
						r_drawHUD.GetBool() && 
						(!replayHud || m_enableInReplay && !Director_IsActive());

	m_hudLayout->SetVisible(hudEnabled);

	m_lightsTime += fDt;

	materials->Setup2D(screenSize.x,screenSize.y);

	if (m_showMotionBlur)
		RenderBlur(screenSize);

	materials->SetAmbientColor(ColorRGBA(1,1,1,1));
	m_hudLayout->SetSize(screenSize);

	if (m_hudMap)
	{
		int alignment = m_hudMap->GetAlignment();

		alignment &= ~(equi::UI_ALIGN_TOP | equi::UI_ALIGN_BOTTOM);
		alignment |= hud_map_pos.GetInt() ? equi::UI_ALIGN_TOP : equi::UI_ALIGN_BOTTOM;

		m_hudMap->SetAlignment(alignment);
	}

	static IEqFont* roboto10 = g_fontCache->GetFont("Roboto", 10);
	static IEqFont* roboto30 = g_fontCache->GetFont("Roboto", 30);
	static IEqFont* roboto30b = g_fontCache->GetFont("Roboto", 30, TEXT_STYLE_BOLD);
	static IEqFont* robotocon30b = g_fontCache->GetFont("Roboto Condensed", 30, TEXT_STYLE_BOLD);
	static IEqFont* robotocon30bi = g_fontCache->GetFont("Roboto Condensed", 30, TEXT_STYLE_BOLD | TEXT_STYLE_ITALIC);

	eqFontStyleParam_t fontParams;
	fontParams.styleFlag |= TEXT_STYLE_SHADOW;
	fontParams.textColor = color4_white;

	BlendStateParam_t blending;
	blending.srcFactor = BLENDFACTOR_SRC_ALPHA;
	blending.dstFactor = BLENDFACTOR_ONE_MINUS_SRC_ALPHA;

	BlendStateParam_t additiveBlend;
	additiveBlend.srcFactor = BLENDFACTOR_ONE;
	additiveBlend.dstFactor = BLENDFACTOR_ONE;

	RasterizerStateParams_t raster;
	raster.scissor = true;
	raster.cullMode = CULL_NONE;

	CMeshBuilder meshBuilder(materials->GetDynamicMesh());

	g_pShaderAPI->SetTexture(NULL, NULL, 0);
	materials->SetBlendingStates(blending);
	materials->SetRasterizerStates(CULL_FRONT, FILL_SOLID);
	materials->SetDepthStates(false, false);
	materials->BindMaterial(materials->GetDefaultMaterial());
	
	float fadeTarget = m_fadeTarget;
	float fadeValue = m_fadeValue;

	// fade screen
	if (fadeValue > 0.0f && fabs(fadeTarget) > 0)
	{
		ColorRGBA blockCol(0.0, 0.0, 0.0, 1.0f);

		float fadeFactor = fadeValue / fabs(fadeTarget);

		if (m_fadeCurtains)
		{
			Vector2D rectTop[] = { MAKEQUAD(0, 0,screenSize.x, screenSize.y*fadeFactor*0.5f, 0) };
			Vector2D rectBot[] = { MAKEQUAD(0, screenSize.y*0.5f + screenSize.y*(1.0f - fadeFactor)*0.5f,screenSize.x, screenSize.y, 0) };

			meshBuilder.Begin(PRIM_TRIANGLE_STRIP);
			meshBuilder.Color4fv(blockCol);
				meshBuilder.Quad2(rectTop[0], rectTop[1], rectTop[2], rectTop[3]);
				meshBuilder.Quad2(rectBot[0], rectBot[1], rectBot[2], rectBot[3]);
			meshBuilder.End();
		}
		else
		{
			blockCol.w = fadeFactor;

			Vector2D rect[] = { MAKEQUAD(0, 0,screenSize.x, screenSize.y, 0) };

			meshBuilder.Begin(PRIM_TRIANGLE_STRIP);
				meshBuilder.Color4fv(blockCol);
				meshBuilder.Quad2(rect[0], rect[1], rect[2], rect[3]);
			meshBuilder.End();
		}
	}

	if (fadeTarget < 0)	// fade in
		fadeValue -= fDt;
	else
		fadeValue += fDt;

	// stupid fuck named GCC could not get proper function match here
	m_fadeValue = clamp<float, float>(fadeValue, 0.0f, fabs(fadeTarget));

	if(hudEnabled)
	{
		bool mainVehicleInPursuit = (m_mainVehicle ? m_mainVehicle->GetPursuedCount() > 0 : false);
		bool damageBarVisible = m_hudDamageBar && m_hudDamageBar->IsVisible();
		bool felonyBarVisible = m_hudFelonyBar && m_hudFelonyBar->IsVisible();
		bool mapVisible = m_hudMap && m_hudMap->IsVisible() && m_showMap;

		meshBuilder.Begin(PRIM_TRIANGLES);

			Rectangle_t damageRect(35,65,410, 92);

			if( m_mainVehicle )
			{
				if(damageBarVisible)
				{
					damageRect = m_hudDamageBar->GetClientRectangle();

					// fill the damage bar
					float fDamage = m_mainVehicle->GetDamage() / m_mainVehicle->GetMaxDamage();

					DrawDamageBar(meshBuilder, damageRect, fDamage);

					// draw pursuit cop "triangles" on screen
					if(mainVehicleInPursuit)
					{
						for (int i = 0; i < g_pAIManager->m_trafficCars.numElem(); i++)
						{
							CAIPursuerCar* pursuer = UTIL_CastToPursuer(g_pAIManager->m_trafficCars[i]);

							if(!pursuer)
								continue;

							if(!pursuer->InPursuit())
								continue;

							Vector3D pursuerPos = pursuer->GetOrigin();

							float dist = length(m_mainVehicle->GetOrigin()-pursuerPos);

							Vector3D screenPos;
							PointToScreen_Z(pursuerPos, screenPos, g_pGameWorld->m_viewprojection, Vector2D((float)screenSize.x,(float)screenSize.y));

							if( screenPos.z < 0.0f && dist < 100.0f )
							{
								float distFadeFac = (100.0f - dist) / 100.0f;

								float height = (1.0f-clamp((10.0f - dist) / 10.0f, 0.0f, 1.0f)) * 90.0f;

								meshBuilder.Color4f(1, 0, 0, distFadeFac);
								meshBuilder.Triangle2(Vector2D(screenSize.x - (screenPos.x + 40.0f), screenSize.y),
														Vector2D(screenSize.x - (screenPos.x - 40.0f), screenSize.y),
														Vector2D(screenSize.x-screenPos.x, screenSize.y - height));
							}
						}
					}
				}

				// draw target car damage, proximity and other bars
				for(hudDisplayObjIterator_t iterator = m_displayObjects.begin(); iterator != m_displayObjects.end(); iterator++)
				{
					hudDisplayObject_t& obj = iterator->second;

					if(	(obj.flags & HUD_DOBJ_CAR_DAMAGE) || (obj.flags & HUD_DOBJ_IS_TARGET))
					{
						CGameObject* gameObj = obj.object;

						float fDamage = 0.0f;
						Vector3D markPos = obj.point;

						float maxVisibleDistance = 150.0f;

						if(gameObj && (gameObj->ObjType() == GO_CAR || gameObj->ObjType() == GO_CAR_AI))
						{
							CCar* car = (CCar*)gameObj;

							fDamage = car->GetDamage() / car->GetMaxDamage();
							markPos = car->GetOrigin() + vec3_up*car->m_conf->cameraConf.height;
							maxVisibleDistance = 50.0f;
						}

						obj.flashValue -= fDt * 1.5f;

						if (obj.flashValue < 0.0f)
							obj.flashValue = 1.0f;

						Vector3D screenPos;
						PointToScreen_Z(markPos, screenPos, g_pGameWorld->m_viewprojection, Vector2D((float)screenSize.x,(float)screenSize.y));

						float dist = length(m_mainVehicle->GetOrigin() - markPos);

						if( screenPos.z > 0.0f && dist < maxVisibleDistance)
						{
							CollisionData_t trace;
							bool visible = !g_pPhysics->TestLine(g_pGameWorld->GetView()->GetOrigin(), markPos, trace, OBJECTCONTENTS_SOLID_OBJECTS | OBJECTCONTENTS_OBJECT);

							if(!visible)
								continue;

							float targetAlpha = 1.0f - (dist / maxVisibleDistance);

							if( (obj.flags & HUD_DOBJ_CAR_DAMAGE) )
							{
								Rectangle_t targetDamageRect(-10, 0, 10, 4 );

								targetDamageRect.vleftTop += screenPos.xy();
								targetDamageRect.vrightBottom += screenPos.xy();

								DrawDamageBar(meshBuilder, targetDamageRect, fDamage, targetAlpha);
							}
						}
					}
				}
			}
		meshBuilder.End();

		DoDebugDisplay();

		// get felony
		float felonyPercent = 0.0f;

		bool mainVehicleHasFelony = false;

		if( m_mainVehicle )
		{
			felonyPercent = fabs(m_mainVehicle->GetFelony())*100;

			mainVehicleHasFelony = felonyPercent >= 10.0f;

			// if cars pursues me
			if(felonyPercent >= 10.0f && mainVehicleInPursuit)
			{
				float colorValue = clamp(sinf(m_lightsTime*16.0)*16,-1.0f,1.0f); //sin(g_pHost->m_fGameCurTime*8.0f);

				float v1 = pow(-min(0.0f,colorValue), 2.0f);
				float v2 = pow(max(0.0f,colorValue), 2.0f);

				fontParams.textColor = lerp(ColorRGBA(v1,0,v2,1), color4_white, 0.25f);
			}
			else if(felonyPercent >= 50.0f)
			{
				fontParams.textColor = ColorRGBA(1.0f,0.2f,0.2f,1);
			}
			else if(felonyPercent >= 10.0f)
			{
				fontParams.textColor = ColorRGBA(1.0f,0.8f,0.0f,1);
			}
		}

		if(felonyBarVisible)
		{
			Vector2D felonyTextPos = m_hudFelonyBar->GetClientRectangle().GetLeftTop();// (damageRect.vleftTop.x, damageRect.vrightBottom.y + 25);

			fontParams.styleFlag |= TEXT_STYLE_FROM_CAP;
			fontParams.scale = 14.0f * m_hudFelonyBar->CalcScaling();
			roboto30b->RenderText(varargs_w(m_felonyTok ? m_felonyTok->GetText() : L"Undefined", (int)felonyPercent), felonyTextPos, fontParams);
		}

		CViewParams& camera = *g_pGameWorld->GetView();

		// display radar and map
		if( mapVisible )
		{
			float viewRotation = camera.GetAngles().y + 180.0f;
			Vector3D viewPos = camera.GetOrigin() * Vector3D(1.0f, 0.0f, 1.0f);

			IRectangle mapRectangle = m_hudMap->GetClientRectangle();

			Vector2D mapCenter( mapRectangle.GetCenter() );

			if(m_mainVehicle)
			{
				Vector3D car_forward = m_mainVehicle->GetForwardVector();

				float Yangle = RAD2DEG(atan2f(car_forward.z, car_forward.x));

				if (Yangle < 0.0)
					Yangle += 360.0f;

				viewPos = m_mainVehicle->GetOrigin() * Vector3D(1.0f,0.0f,1.0f);
				viewRotation = Yangle + 90.0f;
			}

			Vector3D viewForward;
			AngleVectors(Vector3D(0.0f, viewRotation, 0.0f), &viewForward);

			g_pShaderAPI->ChangeRenderTarget(m_mapRenderTarget, 0, nullptr, 0);
			g_pShaderAPI->Clear(true, false, false);

			CViewParams mapView(viewPos + Vector3D(0,250.0f * hud_mapZoom.GetFloat(),0) + viewForward*100.0f * hud_mapZoom.GetFloat(), Vector3D(60.0f,viewRotation + 180.0f,0), 70.0f);

			DrawWorldIntoMap(mapView, fDt);

			// restore
			g_pShaderAPI->ChangeRenderTargetToBackBuffer();
			
			materials->Setup2D(screenSize.x, screenSize.y);
			materials->SetMatrix(MATRIXMODE_VIEW, identity4());
			materials->SetMatrix(MATRIXMODE_WORLD, identity4());

			materials->SetBlendingStates(blending);
			materials->SetRasterizerStates(raster);
			materials->SetDepthStates(false, false);

			g_pShaderAPI->SetScissorRectangle(mapRectangle);

			g_pShaderAPI->SetTexture(m_mapRenderTarget, nullptr,0);
			materials->BindMaterial(materials->GetDefaultMaterial());

			Vertex2D_t mapVerts[] = { MAKETEXQUAD(mapRectangle.vleftTop.x, mapRectangle.vleftTop.y, mapRectangle.vrightBottom.x, mapRectangle.vrightBottom.y, 0) };

			Rectangle_t mapTexCoords(0, 0, 1, 1);
			if (g_pShaderAPI->GetShaderAPIClass() == SHADERAPI_OPENGL)
				mapTexCoords.FlipY();

			// draw the map rectangle
			meshBuilder.Begin(PRIM_TRIANGLES);
				meshBuilder.Color4f(1, 1, 1, 1);
					meshBuilder.TexturedQuad2(mapVerts[0].position, mapVerts[2].position, mapVerts[1].position, mapVerts[3].position,
						mapTexCoords.GetLeftTop(), mapTexCoords.GetRightTop(), mapTexCoords.GetLeftBottom(), mapTexCoords.GetRightBottom());
			meshBuilder.End();

			materials->SetBlendingStates(additiveBlend);
			g_pShaderAPI->SetTexture(nullptr, nullptr, 0);
			materials->BindMaterial(materials->GetDefaultMaterial());

			meshBuilder.Begin(PRIM_TRIANGLES);
				if (mainVehicleInPursuit || !mainVehicleInPursuit && m_radarBlank > 0)
				{
					ColorRGBA color(m_radarBlank);

					// draw flashing red-blue radar
					if (mainVehicleInPursuit)
					{
						float colorValue = sin(m_lightsTime*16.0);

						float v1 = pow(-min(0.0f, colorValue), 2.0f);
						float v2 = pow(max(0.0f, colorValue), 2.0f);

						m_radarBlank = 1.0f;

						color = ColorRGBA(v1, 0.0f, v2, 1.0f) * 0.25f;
					}
					else //draw radar blank after the pursuit ends
					{
						m_radarBlank -= fDt;
					}

					// draw the map-sized rectangle
					meshBuilder.Color4fv(color);
					meshBuilder.TexturedQuad2(mapVerts[0].position, mapVerts[1].position, mapVerts[2].position, mapVerts[3].position,
						mapVerts[0].texCoord, mapVerts[1].texCoord, mapVerts[2].texCoord, mapVerts[3].texCoord);
				}
			meshBuilder.End();
		}
	}

	m_hudLayout->Render();

	if (hud_show_controls.GetBool() && m_mainVehicle)
	{
		eqFontStyleParam_t style;
		style.styleFlag |= TEXT_STYLE_SHADOW;
		style.align = TEXT_ALIGN_HCENTER;
		style.textColor = ColorRGBA(1.0f);
		style.scale = 35.0f;

		EqString controlsString;
		int controls = m_mainVehicle->GetControlButtons();

		if (controls & IN_BURNOUT)
			controlsString.Append("BURNOUT\n");
		else if (controls & IN_ACCELERATE)
			controlsString.Append("ACCELERATE\n");
		else if (controls & IN_HANDBRAKE)
			controlsString.Append("HANDBRAKE\n");
		else if (controls & IN_BRAKE)
			controlsString.Append("BRAKE/REVERSE\n");

		if (controls & IN_STEERLEFT)
		{
			if (controls & IN_FASTSTEER)
				controlsString.Append("LEFT + FAST STEER\n");
			else
				controlsString.Append("LEFT\n");
		}
		else if (controls & IN_STEERRIGHT)
		{
			if (controls & IN_FASTSTEER)
				controlsString.Append("RIGHT + FAST STEER\n");
			else
				controlsString.Append("RIGHT\n");
		}

		Vector2D screenMessagePos(screenSize.x / 2.0f, screenSize.y / 1.5f);

		if(controlsString.Length() > 0)
			robotocon30bi->RenderText(controlsString.c_str(), screenMessagePos, style);
	}

	if(!replayHud)
	{
		// show screen alert
		if( m_screenAlertTime > 0 && fDt > 0.0f )
		{
			m_screenAlertTime -= fDt;
			m_screenAlertInTime -= fDt;
			m_screenAlertInTime = max(m_screenAlertInTime, 0.0f);

			Vector2D screenMessagePos(screenSize.x / 2, screenSize.y / 3.5);

			float clampedAlertTime = clamp(m_screenAlertTime, 0.0f, 1.0f);

			float alpha = clampedAlertTime;

			Vertex2D_t verts[6];

			eqFontStyleParam_t scrMsgParams;
			scrMsgParams.styleFlag |= TEXT_STYLE_SHADOW | TEXT_STYLE_USE_TAGS;
			scrMsgParams.align = TEXT_ALIGN_HCENTER;
			scrMsgParams.textColor = ColorRGBA(1, 1, 1, alpha);
			scrMsgParams.scale = 35.0f;

			const float messageSizeY = robotocon30bi->GetLineHeight(scrMsgParams) + 5.0f;

			Vertex2D_t baseVerts[] = {MAKETEXQUAD(0.0f, screenMessagePos.y, screenSize.x, screenMessagePos.y + messageSizeY, 0.0f)};

			baseVerts[0].color.w = 0.0f;
			baseVerts[1].color.w = 0.0f;
			baseVerts[2].color.w = 0.0f;
			baseVerts[3].color.w = 0.0f;

			verts[0] = baseVerts[0];
			verts[1] = baseVerts[1];

			verts[4] = baseVerts[2];
			verts[5] = baseVerts[3];

			verts[2] = Vertex2D_t::Interpolate(verts[0], verts[4], 0.5f);
			verts[3] = Vertex2D_t::Interpolate(verts[1], verts[5], 0.5f);

			verts[2].color.w = 1.0f;
			verts[3].color.w = 1.0f;


			ColorRGBA alertColor(1.0f, 0.57f, 0.0f, alpha);

			if (m_screenAlertType == HUD_ALERT_SUCCESS)
			{
				alertColor = ColorRGBA(0.25f, 0.6f, 0.25f, alpha);
			}
			else if (m_screenAlertType == HUD_ALERT_DANGER)
			{
				alertColor = ColorRGBA(1.0f, 0.15f, 0.0f, alpha);
				scrMsgParams.textColor = ColorRGBA(0.15f, 0.15f, 0.15f, alpha);
				scrMsgParams.styleFlag &= ~TEXT_STYLE_SHADOW;
			}

			materials->DrawPrimitives2DFFP(PRIM_TRIANGLE_STRIP,verts,elementsOf(verts), NULL, alertColor, &blending);

			// ok for 3 seconds

			float textXPos = -2000.0f * pow(m_screenAlertInTime, 4.0f) + (2000.0f * pow(1.0f - clampedAlertTime, 4.0f));

			robotocon30bi->RenderText(m_screenAlertText.c_str(), screenMessagePos + Vector2D(textXPos, messageSizeY*0.7f), scrMsgParams);
		}

		// show message on screen
		if(m_screenMessageTime > 0 && fDt > 0.0f )
		{
			float textYOffs = 0.0f;
			float textAlpha = 1.0f;

			if(m_screenMessageTime <= 1.0f)
			{
				textAlpha = pow(m_screenMessageTime, 5.0f);
				textYOffs = (1.0f-m_screenMessageTime)*5.0f;
				textYOffs = pow(textYOffs, 5.0f);
			}

			eqFontStyleParam_t scrMsgParams;
			scrMsgParams.styleFlag |= TEXT_STYLE_SHADOW | TEXT_STYLE_USE_TAGS;
			scrMsgParams.align = TEXT_ALIGN_HCENTER;
			scrMsgParams.textColor = ColorRGBA(1, 0.7f, 0.0f, textAlpha);
			scrMsgParams.scale = 30.0f;

			Vector2D screenMessagePos(screenSize.x / 2, (float)screenSize.y / 2.5f - textYOffs);

			roboto30b->RenderText(m_screenMessageText.c_str(), screenMessagePos, scrMsgParams);

			m_screenMessageTime -= fDt;
		}
	}
}


void CDrvSynHUDManager::DoDebugDisplay()
{
	static IEqFont* defFont = g_fontCache->GetFont("default", 0);

	eqFontStyleParam_t style;
	style.styleFlag |= TEXT_STYLE_SHADOW;
	style.textColor = ColorRGBA(1, 1, 0.25f, 1.0f);

	if (hud_show_controls.GetBool())
	{

	}

	if (g_showCameraPosition.GetBool())
	{
		const Vector3D& viewpos = g_pGameWorld->GetView()->GetOrigin();
		const Vector3D& viewrot = g_pGameWorld->GetView()->GetAngles();

		defFont->RenderText(varargs("camera position: %.2f %.2f %.2f\ncamera angles: %.2f %.2f %.2f", viewpos.x, viewpos.y, viewpos.z, viewrot.x, viewrot.y, viewrot.z), Vector2D(20, 180), style);
	}

	if (g_showCarPosition.GetBool() && m_mainVehicle)
	{
		const Vector3D& carpos = m_mainVehicle->GetOrigin();
		const Vector3D& carrot = m_mainVehicle->GetAngles();

		defFont->RenderText(varargs("car position: %.2f %.2f %.2f\ncar angles: %.2f %.2f %.2f", carpos.x, carpos.y, carpos.z, carrot.x, carrot.y, carrot.z), Vector2D(20, 220), style);
	}

	if (hud_debug_car.GetBool() && m_mainVehicle)
	{
		const float DEBUG_FONT_SIZE = 20.0f;
		const Vector2D DEBUG_LINE_OFS = Vector2D(0, DEBUG_FONT_SIZE*2.0f);

		const Vector2D debugOffset(10, 400);

		float accel, brake, steer;
		m_mainVehicle->GetControlVars(accel, brake, steer);

		defFont->RenderText(varargs("Speed: %.2f KPH (%.2f MPS)", m_mainVehicle->GetSpeed(), m_mainVehicle->GetSpeed()*KPH_TO_MPS), debugOffset, style);
		defFont->RenderText(varargs("Speed from wheels: %.2f KPH (%.2f MPS) at gear: %d, RPM: %d", m_mainVehicle->GetSpeedWheels(), m_mainVehicle->GetSpeedWheels()*KPH_TO_MPS, m_mainVehicle->GetGear(), (int)m_mainVehicle->GetRPM()), debugOffset + DEBUG_LINE_OFS, style);
		defFont->RenderText(varargs("Lateral slide: %.2f", m_mainVehicle->GetLateralSlidingAtBody()), debugOffset + DEBUG_LINE_OFS * 2, style);
		defFont->RenderText(varargs("Traction slide: %.2f", m_mainVehicle->GetTractionSliding(true)), debugOffset + DEBUG_LINE_OFS * 3, style);
		defFont->RenderText(varargs("Steering - input: %.2f output: %.2f helper: %.2f", steer, (float)m_mainVehicle->m_steering, (float)m_mainVehicle->m_autobrake), debugOffset + DEBUG_LINE_OFS * 5, style);
		defFont->RenderText(varargs("Acceleration: %.2f", accel), debugOffset + DEBUG_LINE_OFS * 6, style);
		defFont->RenderText(varargs("Brake: %.2f", brake), debugOffset + DEBUG_LINE_OFS * 7, style);
	}

	if (hud_debug_roadmap.GetBool() && m_mainVehicle)
	{
		const Vector3D& carpos = m_mainVehicle->GetOrigin();

		levroadcell_t* cell = g_pGameWorld->m_level.Road_GetGlobalTile(carpos);

		if (cell)
		{
			defFont->RenderText(varargs("direction: %d, type: %d, trafficlight: %d", cell->direction, cell->type, (cell->flags & ROAD_FLAG_TRAFFICLIGHT) > 0), Vector2D(20, 320), style);
		}
		else
		{
			defFont->RenderText("not on road", Vector2D(20, 320), style);
		}
	}
}

// main object to display
void CDrvSynHUDManager::SetDisplayMainVehicle( CCar* car )
{
	if(m_mainVehicle == car)
		return;

	if(m_mainVehicle)
		m_mainVehicle->Ref_Drop();

	m_mainVehicle = car;

	if(m_mainVehicle)
		m_mainVehicle->Ref_Grab();
}

// HUD map management
int	CDrvSynHUDManager::AddTrackingObject( CGameObject* obj, int flags )
{
	hudDisplayObject_t dispObj;

	dispObj.object = obj;

	//if(obj)
	//	obj->Ref_Grab();

	dispObj.flags = flags;
	dispObj.point = vec3_zero;
	dispObj.flashValue = 1.0f;

	int handleId = m_handleCounter++;

	m_displayObjects[handleId] = dispObj;

	return handleId;
}

int	CDrvSynHUDManager::AddMapTargetPoint( const Vector3D& position )
{
	hudDisplayObject_t dispObj;

	dispObj.object = NULL;
	dispObj.flags = HUD_DOBJ_IS_TARGET;
	dispObj.point = position;
	dispObj.flashValue = 1.0f;

	int handleId = m_handleCounter++;

	m_displayObjects[handleId] = dispObj;

	return handleId;
}

void CDrvSynHUDManager::RemoveTrackingObject( int handle )
{
	if(m_displayObjects.count(handle) > 0)
	{
		//hudDisplayObject_t& obj = m_displayObjects[handle];
		//if(obj.object)
		//	obj.object->Ref_Drop();

		m_displayObjects.erase(handle);
	}
}

//-------------------------------------------------------

#ifndef __INTELLISENSE__
OOLUA_EXPORT_FUNCTIONS(
	CDrvSynHUDManager,
	SetHudScheme,
	AddTrackingObject,
	AddMapTargetPoint,
	RemoveTrackingObject,
	ShowMessage,
	ShowAlert,
	SetTimeDisplay,
	Enable,
	EnableInReplay,
	ShowMap,
	FadeIn,
	FadeOut
)

OOLUA_EXPORT_FUNCTIONS_CONST(
	CDrvSynHUDManager,
	IsEnabled,
	IsMapShown,
	FindChildElement
)


#endif // __INTELLISENSE__
