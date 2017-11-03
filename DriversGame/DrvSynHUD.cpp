//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: UI elements
//////////////////////////////////////////////////////////////////////////////////

#include "DrvSynHUD.h"
#include "AICarManager.h"
#include "heightfield.h"
#include "replay.h"

#define DEFAULT_HUD_SCHEME "resources/hud/defaulthud.res"

static CDrvSynHUDManager s_drvSynHUDManager;
CDrvSynHUDManager* g_pGameHUD = &s_drvSynHUDManager;

ConVar r_drawHUD("hud_draw", "1", NULL, CV_ARCHIVE);

ConVar hud_mapZoom("hud_mapZoom", "1.5", NULL, CV_ARCHIVE);
ConVar hud_mapSize("hud_mapSize", "250", NULL, CV_ARCHIVE);

ConVar hud_debug_car("hud_debug_car", "0", NULL, CV_CHEAT);

DECLARE_CMD(hud_showLastMessage, NULL, 0)
{
	g_pGameHUD->ShowLastMessage();
}

//----------------------------------------------------------------------------------

CDrvSynHUDManager::CDrvSynHUDManager()
	: m_handleCounter(0), m_mainVehicle(nullptr), m_curTime(0.0f), m_mapTexture(nullptr), m_showMap(true), m_screenAlertTime(0.0f), m_screenMessageTime(0.0f), 
		m_hudLayout(nullptr), m_hudDamageBar(nullptr), 	m_hudFelonyBar(nullptr), m_hudMap(nullptr)
{
}

void CDrvSynHUDManager::Init()
{
	m_enable = true;

	EqString mapTexName("levelmap/");
	mapTexName.Append( g_pGameWorld->GetLevelName() );

	m_mapTexture = g_pShaderAPI->LoadTexture( mapTexName.c_str() , TEXFILTER_LINEAR, TEXADDRESS_CLAMP, TEXFLAG_NOQUALITYLOD );

	if( m_mapTexture )
	{
		if(!stricmp(m_mapTexture->GetName(), "error"))
			m_mapTexture = NULL;
		else
			m_mapTexture->Ref_Grab();
	}

	m_showMap = (m_mapTexture != NULL);

	m_handleCounter = 0;
	m_curTime = 0.0f;

	m_screenMessageTime = 0.0f;
	m_screenMessageText.Clear();

	m_screenAlertTime = 0.0f;
	m_screenAlertInTime = 0.0f;
	m_screenAlertText.Clear();

	m_timeDisplayEnable = false;
	m_timeDisplayValue = 0.0;

	m_radarBlank = 0.0f;

	m_damageTok = g_localizer->GetToken("HUD_DAMAGE_TITLE");
	m_felonyTok = g_localizer->GetToken("HUD_FELONY_TITLE");

	// init hud layout
	m_hudLayout = equi::Manager->CreateElement("HudElement");

	//SetHudScheme( DEFAULT_HUD_SCHEME );
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

	kvkeybase_t* hudKvs = KV_LoadFromFile(m_schemeName.c_str(), SP_MOD);
	kvkeybase_t* baseHud = nullptr;

	if(hudKvs)
	{
		kvkeybase_t* hudBasePath = hudKvs->FindKeyBase("base");
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
			m_hudLayout->InitFromKeyValues( hudKvs, true );
		}
		else
			m_hudLayout->InitFromKeyValues( hudKvs );

		delete baseHud;
		delete hudKvs;
	}
	else
		MsgError("Failed to load HUD scheme\n");

	m_hudDamageBar = m_hudLayout->FindChild("damageBar");
	m_hudFelonyBar = m_hudLayout->FindChild("felonyLabel");
	m_hudMap = m_hudLayout->FindChild("map");
}

void CDrvSynHUDManager::Cleanup()
{
	g_pShaderAPI->FreeTexture( m_mapTexture );
	m_mapTexture = NULL;

	/*
	for(hudDisplayObjIterator_t iterator = m_displayObjects.begin(); iterator != m_displayObjects.end(); iterator++)
	{
		if(iterator->second.object)
			iterator->second.object->Ref_Drop();
	}
	*/

	m_displayObjects.clear();
	SetDisplayMainVehicle(nullptr);

	delete m_hudLayout;
	m_hudLayout = nullptr;

	m_schemeName.Empty();
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
	m_timeDisplayEnable = enabled;
	m_timeDisplayValue = time;
}

void CDrvSynHUDManager::FadeIn( bool useCurtains )
{

}

void CDrvSynHUDManager::FadeOut()
{

}

equi::IUIControl* CDrvSynHUDManager::FindChildElement(const char* name) const
{
	return m_hudLayout->FindChild(name);
}

ConVar hud_map_pos("hud_map_pos", "0", "Map position (0 - bottom, 1 - top)", CV_ARCHIVE);

ConVar g_showCameraPosition("g_showCameraPosition", "0", NULL, CV_CHEAT);
ConVar g_showCarPosition("g_showCarPosition", "0", NULL, CV_CHEAT);

void CDrvSynHUDManager::DrawDamageRectangle(CMeshBuilder& meshBuilder, Rectangle_t& rect, float percentage, float alpha)
{
	meshBuilder.Color4f(0.435, 0.435, 0.435, 0.35f*alpha);
	meshBuilder.Quad2(rect.GetLeftTop(), rect.GetRightTop(), rect.GetLeftBottom(), rect.GetRightBottom());

	if(percentage > 0)
	{
		Rectangle_t fillRect(rect.vleftTop, Vector2D(lerp(rect.vleftTop.x, rect.vrightBottom.x, percentage), rect.vrightBottom.y));

		ColorRGBA damageColor = lerp(ColorRGBA(0,0.6f,0,alpha), ColorRGBA(0.6f,0,0,alpha), percentage) * 1.5f;

		if(percentage > 0.85f)
			damageColor = lerp(ColorRGBA(0.1f,0,0,alpha), ColorRGBA(0.8f,0,0,alpha), fabs(sin(m_curTime*3.0f))) * 1.5f;

		// draw damage bar foreground
		meshBuilder.Color4fv(damageColor);
		meshBuilder.Quad2(fillRect.GetLeftTop(), fillRect.GetRightTop(), fillRect.GetLeftBottom(), fillRect.GetRightBottom());
	}
}

// render the screen with maps and shit
void CDrvSynHUDManager::Render( float fDt, const IVector2D& screenSize) // , const Matrix4x4& projMatrix, const Matrix4x4& viewMatrix )
{
	bool replayHud = (g_replayData->m_state == REPL_PLAYING);

	m_hudLayout->SetVisible(m_enable && r_drawHUD.GetBool() && !replayHud);

	if( !r_drawHUD.GetBool() )
		return;

	m_curTime += fDt;

	materials->Setup2D(screenSize.x,screenSize.y);

	materials->SetAmbientColor(ColorRGBA(1,1,1,1));
	m_hudLayout->SetSize(screenSize);
	m_hudLayout->Render();

	static IEqFont* roboto10 = g_fontCache->GetFont("Roboto", 10);
	static IEqFont* roboto30 = g_fontCache->GetFont("Roboto", 30);
	static IEqFont* roboto30b = g_fontCache->GetFont("Roboto", 30, TEXT_STYLE_BOLD);
	static IEqFont* robotocon30b = g_fontCache->GetFont("Roboto Condensed", 30, TEXT_STYLE_BOLD);
	static IEqFont* robotocon30bi = g_fontCache->GetFont("Roboto Condensed", 30, TEXT_STYLE_BOLD | TEXT_STYLE_ITALIC);
	static IEqFont* defFont = g_fontCache->GetFont("default", 0);

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
	raster.cullMode = CULL_FRONT;

	if(m_enable && !replayHud)
	{
		bool inPursuit = (m_mainVehicle ? m_mainVehicle->GetPursuedCount() > 0 : false);
		bool damageBarVisible = m_hudDamageBar && m_hudDamageBar->IsVisible();
		bool felonyBarVisible = m_hudFelonyBar && m_hudFelonyBar->IsVisible();
		bool mapVisible = m_hudMap && m_hudMap->IsVisible() && m_showMap;

		//Vertex2D_t dmgrect[] = { MAKETEXQUAD(damageRect.vleftTop.x, damageRect.vleftTop.y,damageRect.vrightBottom.x, damageRect.vrightBottom.y, 0) };

		CMeshBuilder meshBuilder(materials->GetDynamicMesh());

		g_pShaderAPI->SetTexture(NULL,NULL,0);
		materials->SetBlendingStates(blending);
		materials->SetRasterizerStates(CULL_BACK, FILL_SOLID);
		materials->SetDepthStates(false,false);
		materials->BindMaterial(materials->GetDefaultMaterial());

		meshBuilder.Begin(PRIM_TRIANGLES);

			Rectangle_t damageRect(35,65,410, 92);

			if( m_mainVehicle )
			{
				if(damageBarVisible)
				{
					damageRect = m_hudDamageBar->GetClientRectangle();

					// fill the damage bar
					float fDamage = m_mainVehicle->GetDamage() / m_mainVehicle->GetMaxDamage();

					DrawDamageRectangle(meshBuilder, damageRect, fDamage);

					// draw pursuit cop "triangles" on screen
					if( inPursuit )
					{
						for(int i = 0; i < g_pAIManager->m_copCars.numElem(); i++)
						{
							CAIPursuerCar* pursuer = g_pAIManager->m_copCars[i];

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
								meshBuilder.Triangle2(	Vector2D(screenSize.x-(screenPos.x-40.0f), screenSize.y),
														Vector2D(screenSize.x-(screenPos.x+40.0f), screenSize.y),
														Vector2D(screenSize.x-screenPos.x, screenSize.y - height));
							}
						}
					}
				}

				// draw target car damage, proximity and other bars
				for(hudDisplayObjIterator_t iterator = m_displayObjects.begin(); iterator != m_displayObjects.end(); iterator++)
				{
					hudDisplayObject_t& obj = iterator->second;

					obj.flashValue -= fDt;

					if(obj.flashValue < 0.0f)
						obj.flashValue = 1.0f;

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

							if(obj.flags & HUD_DOBJ_IS_TARGET)
							{
								Vector2D targetScreenPos = screenPos.xy() - Vector2D(0, obj.flashValue*30.0f);

								meshBuilder.Color4f( 1.0f, 0.0f, 0.0f, pow(targetAlpha, 0.5f) );
								meshBuilder.Triangle2(targetScreenPos, targetScreenPos+Vector2D(-20, -20), targetScreenPos+Vector2D(20, -20));
							}

							if( (obj.flags & HUD_DOBJ_CAR_DAMAGE) )
							{
								Rectangle_t targetDamageRect(-10, 0, 10, 4 );

								targetDamageRect.vleftTop += screenPos.xy();
								targetDamageRect.vrightBottom += screenPos.xy();

								DrawDamageRectangle(meshBuilder, targetDamageRect, fDamage, targetAlpha);
							}
						}
					}
				}
			}
		meshBuilder.End();

		if(damageBarVisible)
		{
			Vector2D damageTextPos( damageRect.vleftTop.x+5, damageRect.vleftTop.y+15);
			fontParams.scale = 30.0f;
			robotocon30b->RenderText(m_damageTok ? m_damageTok->GetText() : L"Undefined", damageTextPos, fontParams);
		}

		if(hud_debug_car.GetBool() && m_mainVehicle)
		{
			fontParams.scale = 20.0f;
			robotocon30b->RenderText(varargs("Speed: %.2f KPH (%.2f MPS)", m_mainVehicle->GetSpeed(), m_mainVehicle->GetSpeed()*KPH_TO_MPS), Vector2D(10,400), fontParams);
			robotocon30b->RenderText(varargs("Speed from wheels: %.2f KPH (%.2f MPS)", m_mainVehicle->GetSpeedWheels(), m_mainVehicle->GetSpeedWheels()*KPH_TO_MPS), Vector2D(10,440), fontParams);
			robotocon30b->RenderText(varargs("Lateral slide: %.2f", m_mainVehicle->GetLateralSlidingAtBody()), Vector2D(10,480), fontParams);
			robotocon30b->RenderText(varargs("Traction slide: %.2f", m_mainVehicle->GetTractionSliding(true)), Vector2D(10,520), fontParams);
			
		}

		// get felony
		float felonyPercent = 0.0f;

		bool hasFelony = false;

		if( m_mainVehicle )
		{
			felonyPercent = m_mainVehicle->GetFelony()*100;

			hasFelony = felonyPercent >= 10.0f;

			// if cars pursues me
			if(felonyPercent >= 10.0f && inPursuit)
			{
				float colorValue = clamp(sinf(m_curTime*16.0)*16,-1.0f,1.0f); //sin(g_pHost->m_fGameCurTime*8.0f);

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
			Vector2D felonyTextPos(damageRect.vleftTop.x, damageRect.vrightBottom.y+25);

			fontParams.styleFlag |= TEXT_STYLE_FROM_CAP;
			fontParams.scale = 24.0f;
			roboto30b->RenderText(varargs_w(m_felonyTok ? m_felonyTok->GetText() : L"Undefined", (int)felonyPercent), felonyTextPos, fontParams);
		}

		if(m_timeDisplayEnable)
		{
			static IEqFont* numbers50 = g_fontCache->GetFont("Roboto Condensed", 50);
			static IEqFont* numbers20 = g_fontCache->GetFont("Roboto Condensed", 20);

			int secs,mins, millisecs;

			secs = m_timeDisplayValue;
			mins = secs / 60;

			millisecs = m_timeDisplayValue * 100.0f - secs*100;

			secs -= mins*60;

			eqFontStyleParam_t numFontParams;
			numFontParams.styleFlag |= TEXT_STYLE_SHADOW;
			numFontParams.align = TEXT_ALIGN_HCENTER;
			numFontParams.textColor = color4_white;
			numFontParams.scale = 50.0f;

			Vector2D timeDisplayTextPos(screenSize.x / 2, 80);

			wchar_t* str = varargs_w(L"%.2i:%.2i", mins, secs);

			float minSecWidth = numbers50->GetStringWidth(str, numFontParams);
			numbers50->RenderText(str, timeDisplayTextPos, numFontParams);

			numFontParams.align = 0;
			numFontParams.scale = 20.0f;

			Vector2D millisDisplayTextPos = timeDisplayTextPos + Vector2D(floor(minSecWidth*0.5f), 0.0f);

			numbers20->RenderText(varargs_w(L"'%02d", millisecs), millisDisplayTextPos, numFontParams);
		}

		CViewParams& camera = *g_pGameWorld->GetView();

		// display radar and map
		if( mapVisible )
		{
			IVector2D mapSize(hud_mapSize.GetInt());
			IVector2D mapPos = screenSize-mapSize-IVector2D(55);

			if(hud_map_pos.GetInt() == 1)
			{
				mapPos.y = 55;
			}

			float viewRotation = DEG2RAD( camera.GetAngles().y + 180);
			Vector3D viewPos = Vector3D(camera.GetOrigin().xz() * Vector2D(1.0f,-1.0f), 0.0f);
			Vector2D playerPos(0);

			float mapZoom = (hud_mapZoom.GetFloat() / HFIELD_POINT_SIZE) * m_hudMap->CalcScaling().y;

			IRectangle mapRectangle = m_hudMap->GetClientRectangle(); //(mapPos, mapPos+mapSize);

			g_pShaderAPI->SetScissorRectangle(mapRectangle);

			Vector2D mapCenter( mapRectangle.GetCenter() );

			if(m_mainVehicle)
			{
				Vector3D car_forward = m_mainVehicle->GetForwardVector();

				float Yangle = RAD2DEG(atan2f(car_forward.z, car_forward.x));

				if (Yangle < 0.0)
					Yangle += 360.0f;

				viewPos = Vector3D(m_mainVehicle->GetOrigin().xz()*Vector2D(1.0f,-1.0f), 0.0f);
				viewRotation = DEG2RAD( Yangle + 90);

				playerPos = m_mainVehicle->GetOrigin().xz() * Vector2D(-1.0f,1.0f);
			}

			Matrix4x4 mapTransform = rotateZ4(viewRotation);// * rotateX4(DEG2RAD(25));

			//materials->SetMatrix(MATRIXMODE_PROJECTION, perspectiveMatrixY(mapSize.x,mapSize.y, 90.0f, 0.0f, 1000.0f));
			//materials->SetMatrix(MATRIXMODE_VIEW, rotateZXY4(DEG2RAD(40.0f), DEG2RAD(180.0f), DEG2RAD(180.0f)) * translate(0.0f, 200.0f, -500.0f));
			//materials->SetMatrix(MATRIXMODE_WORLD, mapTransform * translate(viewPos));

			materials->SetMatrix(MATRIXMODE_VIEW, translate(mapCenter.x,mapCenter.y, 0.0f) * scale4(mapZoom, mapZoom, 1.0f));
			materials->SetMatrix(MATRIXMODE_WORLD, mapTransform * translate(viewPos));

			Vector2D imgSize(1.0f);

			if( m_mapTexture )
				imgSize = Vector2D(m_mapTexture->GetWidth(),m_mapTexture->GetHeight());

			Vector2D imgToWorld = imgSize*HFIELD_POINT_SIZE;
			Vector2D imgHalf = imgToWorld * 0.5f;

			Vertex2D_t mapVerts[] = { MAKETEXQUAD(-imgHalf.x, -imgHalf.y, imgHalf.x, imgHalf.y, 0) };

			g_pShaderAPI->SetTexture(m_mapTexture,NULL,0);
			materials->SetBlendingStates(blending);
			materials->SetRasterizerStates(raster);
			materials->SetDepthStates(false,false);
			materials->BindMaterial(materials->GetDefaultMaterial());

			// draw the map rectangle
			meshBuilder.Begin(PRIM_TRIANGLES);
				meshBuilder.Color4f( 1,1,1,0.5f );
				meshBuilder.TexturedQuad2(	mapVerts[0].position, mapVerts[1].position, mapVerts[2].position, mapVerts[3].position,
											mapVerts[0].texCoord, mapVerts[1].texCoord, mapVerts[2].texCoord, mapVerts[3].texCoord);
			meshBuilder.End();

			g_pShaderAPI->SetTexture(m_mapTexture,NULL,0);
			materials->BindMaterial(materials->GetDefaultMaterial());

			meshBuilder.Begin(PRIM_TRIANGLE_STRIP);

				// display cop cars on map
				for(int i = 0; i < g_pAIManager->m_copCars.numElem(); i++)
				{
					CAIPursuerCar* pursuer = g_pAIManager->m_copCars[i];

					if(!pursuer)
						continue;

					if(!pursuer->IsAlive() || !pursuer->IsEnabled())
						continue;

					// don't reveal cop position
					if(inPursuit && !pursuer->InPursuit())
						continue;

					Vector3D cop_forward = pursuer->GetForwardVector();

					float Yangle = RAD2DEG(atan2f(cop_forward.z, cop_forward.x));

					if (Yangle < 0.0)
						Yangle += 360.0f;

					Vector2D copPos = (pursuer->GetOrigin().xz() + pursuer->GetForwardVector().xz()) * Vector2D(-1.0f,1.0f);

					meshBuilder.Color4fv(1.0f);
					meshBuilder.Position2fv(copPos);

					int vtxIdx = meshBuilder.AdvanceVertexIndex();

					bool inPursuit = pursuer->InPursuit();

					bool hasAttention = (hasFelony || pursuer->GetPursuerType() == PURSUER_TYPE_GANG);

					// draw cop car frustum
					float size = hasAttention ? AI_COPVIEW_FAR_WANTED : AI_COPVIEW_FAR;
					float angFac = hasAttention ? 22.0f : 12.0f;
					float angOffs = hasAttention ? 176.0f : 228.0f;

					meshBuilder.Color4f(1,1,1,0.0f);
					for(int j = 0; j < 8; j++)
					{
						float ss,cs;
						float angle = (float(j)+0.5f) * angFac + Yangle + angOffs;
						SinCos(DEG2RAD(angle), &ss, &cs);

						if(j < 1 || j > 6)
							meshBuilder.Position2fv(copPos + Vector2D(ss, cs)*size*0.35f);
						else
							meshBuilder.Position2fv(copPos + Vector2D(ss, cs)*size);

						if(j > 0)
							meshBuilder.AdvanceVertexIndex(vtxIdx);

						meshBuilder.AdvanceVertexIndex(vtxIdx+j+1);
					}

					meshBuilder.AdvanceVertexIndex(0xFFFF);

					// draw dot also
					{
						float colorValue = inPursuit ? sin(m_curTime*16.0) : 0.0f;
						colorValue = clamp(colorValue*100.0f, 0.0f, 1.0f);

						meshBuilder.Color4fv(ColorRGBA(ColorRGB(colorValue), 1.0f));
						meshBuilder.Position2fv(copPos);
						int firstVert = meshBuilder.AdvanceVertexIndex();

						//meshBuilder.Color4fv(ColorRGBA(ColorRGB(colorValue), 0.0f));
						for(int i = 0; i < 11; i++)
						{
							float ss,cs;
							float angle = float(i-1) * 36.0f;
							SinCos(DEG2RAD(angle), &ss, &cs);

							meshBuilder.Position2fv( copPos + Vector2D(ss, cs)*6.0f );

							if(i > 0)
								meshBuilder.AdvanceVertexIndex( firstVert );

							meshBuilder.AdvanceVertexIndex( firstVert+i+1 );
						}

						meshBuilder.AdvanceVertexIndex(0xFFFF);
					}
				}

				for(hudDisplayObjIterator_t iterator = m_displayObjects.begin(); iterator != m_displayObjects.end(); iterator++)
				{
					hudDisplayObject_t& obj = iterator->second;

					if(!(obj.flags & HUD_DOBJ_IS_TARGET))
						continue;

					Vector2D objPos = (obj.object ? obj.object->GetOrigin().xz() : obj.point.xz()) * Vector2D(-1.0f,1.0f);

					// draw fading out arrow
					{
						ColorRGBA arrowCol1(0.0f,0.0f,0.0f,0.5f);
						ColorRGBA arrowCol2(0.0f,0.0f,0.0f,0.15f);

						meshBuilder.Color4fv(arrowCol1);
						meshBuilder.Position2fv(objPos);
						int firstVert = meshBuilder.AdvanceVertexIndex();

						Vector2D targetDir = (objPos - playerPos);
						Vector2D ntargetDir = normalize(targetDir);
						Vector2D targetPerpendicular = Vector2D(-ntargetDir.y,ntargetDir.x);

						float distToTarg = length(targetDir);

						meshBuilder.Color4fv(arrowCol2);

						meshBuilder.Position2fv(playerPos - targetPerpendicular*distToTarg*0.25f);
						meshBuilder.AdvanceVertexIndex(firstVert+1);

						meshBuilder.Position2fv(playerPos + targetPerpendicular*distToTarg*0.25f);
						meshBuilder.AdvanceVertexIndex(firstVert+2);

						meshBuilder.AdvanceVertexIndex(firstVert+2);

						meshBuilder.AdvanceVertexIndex(0xFFFF);
					}

					// draw flashing point
					meshBuilder.Color4fv(1.0f);
					meshBuilder.Position2fv(objPos);
					int firstVert = meshBuilder.AdvanceVertexIndex();

					meshBuilder.Color4f(1.0f, 1.0f, 1.0f, 0.0f);
					for(int i = 0; i < 11; i++)
					{
						float ss,cs;
						float angle = float(i-1) * 36.0f;
						SinCos(DEG2RAD(angle), &ss, &cs);

						meshBuilder.Position2fv( objPos + Vector2D(ss, cs) * obj.flashValue * 30.0f );

						if(i > 0)
							meshBuilder.AdvanceVertexIndex( firstVert );

						meshBuilder.AdvanceVertexIndex( firstVert+i+1 );
					}

					meshBuilder.AdvanceVertexIndex(0xFFFF);
				}

				// draw player car dot
				{
					meshBuilder.Color4f(0,0,0,1);
					meshBuilder.Position2fv(playerPos);
					int firstVert = meshBuilder.AdvanceVertexIndex();

					//meshBuilder.Color4fv(0.0f);
					for(int i = 0; i < 11; i++)
					{
						float ss,cs;
						float angle = float(i-1) * 36.0f;
						SinCos(DEG2RAD(angle), &ss, &cs);

						meshBuilder.Position2fv( playerPos + Vector2D(ss, cs)*10.0f );

						if(i > 0)
							meshBuilder.AdvanceVertexIndex( firstVert );

						meshBuilder.AdvanceVertexIndex( firstVert+i+1 );
					}

					meshBuilder.AdvanceVertexIndex(0xFFFF);
				}

				if(inPursuit || !inPursuit && m_radarBlank > 0)
				{
					ColorRGBA color(1,1,1,m_radarBlank);

					// draw flashing red-blue radar
					if(inPursuit)
					{
						float colorValue = sin(m_curTime*16.0);

						float v1 = pow(-min(0.0f,colorValue), 2.0f);
						float v2 = pow(max(0.0f,colorValue), 2.0f);

						m_radarBlank = 1.0f;

						color = ColorRGBA(v1,0.0f,v2,0.35f);
					}
					else //draw radar blank after the pursuit ends
					{
						m_radarBlank -= fDt;
					}

					// draw the map-sized rectangle
					meshBuilder.Color4fv( color );
					meshBuilder.Quad2(	mapVerts[0].position, mapVerts[1].position, mapVerts[2].position, mapVerts[3].position);
				}

			meshBuilder.End();
		}

		materials->SetMatrix(MATRIXMODE_VIEW, identity4());
		materials->SetMatrix(MATRIXMODE_WORLD, identity4());
	}

	if(!replayHud)
	{
		// show screen alert
		if( m_screenAlertTime > 0 && fDt > 0.0f )
		{
			m_screenAlertTime -= fDt;
			m_screenAlertInTime -= fDt;
			m_screenAlertInTime = max(m_screenAlertInTime, 0.0f);

			Vector2D screenMessagePos(screenSize.x / 2, screenSize.y / 2.8);

			Vertex2D_t verts[6];

			const float messageSizeY = 50.0f;

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

			float clampedAlertTime = clamp(m_screenAlertTime, 0.0f, 1.0f);

			float alpha = clampedAlertTime;

			ColorRGBA alertColor(1.0f, 0.7f, 0.0f, alpha);

			if(m_screenAlertType == HUD_ALERT_SUCCESS)
				alertColor = ColorRGBA(0.25f, 0.6f, 0.25f, alpha);
			else if(m_screenAlertType == HUD_ALERT_DANGER)
				alertColor = ColorRGBA(1.0f, 0.15f, 0.0f, alpha);

			materials->DrawPrimitives2DFFP(PRIM_TRIANGLE_STRIP,verts,elementsOf(verts), NULL, alertColor, &blending);

			eqFontStyleParam_t scrMsgParams;
			scrMsgParams.styleFlag |= TEXT_STYLE_SHADOW | TEXT_STYLE_USE_TAGS;
			scrMsgParams.align = TEXT_ALIGN_HCENTER;
			scrMsgParams.textColor = ColorRGBA(1,1,1,alpha);
			scrMsgParams.scale = 30.0f;

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
			scrMsgParams.textColor = ColorRGBA(1,1,0.25f,textAlpha);
			scrMsgParams.scale = 30.0f;

			Vector2D screenMessagePos(screenSize.x / 2, (float)screenSize.y / 3 - textYOffs);

			roboto30b->RenderText(m_screenMessageText.c_str(), screenMessagePos, scrMsgParams);

			m_screenMessageTime -= fDt;
		}
	}

	if(g_showCameraPosition.GetBool())
	{
		Vector3D viewpos = g_pGameWorld->GetView()->GetOrigin();
		Vector3D viewrot = g_pGameWorld->GetView()->GetAngles();

		eqFontStyleParam_t style;
		style.styleFlag |= TEXT_STYLE_SHADOW;
		style.textColor = ColorRGBA(0.25f,1,0.25f, 1.0f);
		defFont->RenderText(varargs("camera position: %.2f %g.2f %g.2f\ncamera angles: %.2f %.2f %.2f", viewpos.x,viewpos.y,viewpos.z, viewrot.x,viewrot.y,viewrot.z), Vector2D(20, 180), style);
	}

	if(g_showCarPosition.GetBool() && m_mainVehicle)
	{
		Vector3D carpos = m_mainVehicle->GetOrigin();
		Vector3D carrot = m_mainVehicle->GetAngles();

		eqFontStyleParam_t style;
		style.styleFlag |= TEXT_STYLE_SHADOW;
		style.textColor = ColorRGBA(1,1,0.25f, 1.0f);

		defFont->RenderText(varargs("car position: %.2f %.2f %.2f\ncar angles: %.2f %.2f %.2f", carpos.x,carpos.y,carpos.z, carrot.x,carrot.y,carrot.z), Vector2D(20, 220), style);
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
		hudDisplayObject_t& obj = m_displayObjects[handle];
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
