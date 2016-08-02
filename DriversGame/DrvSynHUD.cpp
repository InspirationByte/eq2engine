//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: UI elements
//////////////////////////////////////////////////////////////////////////////////

#include "DrvSynHUD.h"
#include "AICarManager.h"
#include "heightfield.h"

static CDrvSynHUDManager s_drvSynHUDManager;
CDrvSynHUDManager* g_pGameHUD = &s_drvSynHUDManager;

ConVar r_drawHUD("hud_draw", "1", NULL, CV_ARCHIVE);

ConVar hud_mapZoom("hud_mapZoom", "1.5", NULL, CV_ARCHIVE);
ConVar hud_mapSize("hud_mapSize", "250", NULL, CV_ARCHIVE);

DECLARE_CMD(hud_showLastMessage, NULL, 0)
{
	g_pGameHUD->ShowLastScreenMessage();
}

//----------------------------------------------------------------------------------
/*
#define SCREEN_MIN_WIDTH	1280
#define SCREEN_MIN_HEIGHT	768

float ScreenScaler_GetBestFontSize( const IVector2D& screenSize,  float initialFontSize )
{
	
}
*/
//----------------------------------------------------------------------------------

CDrvSynHUDManager::CDrvSynHUDManager() : m_handleCounter(0), m_mainVehicle(NULL), m_curTime(0.0f), m_mapTexture(NULL), m_showMap(true)
{
}

void CDrvSynHUDManager::Init()
{
	m_enable = true;

	EqString mapTexName("levelmap/");
	mapTexName.Append( g_pGameWorld->GetLevelName() );

	m_mapTexture = g_pShaderAPI->LoadTexture( mapTexName.c_str() , TEXFILTER_LINEAR, ADDRESSMODE_CLAMP, TEXFLAG_NOQUALITYLOD );

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

	m_timeDisplayEnable = false;
	m_timeDisplayValue = 0.0;

	m_radarBlank = 0.0f;

	m_damageTok = g_localizer->GetToken("HUD_DAMAGE_TITLE");
	m_felonyTok = g_localizer->GetToken("HUD_FELONY_TITLE");
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
	SetDisplayMainVehicle(NULL);
}

void CDrvSynHUDManager::ShowScreenMessage( const char* token, float time )
{
	m_screenMessageText = LocalizedString(token);

	m_screenMessageTime = time;

	if(time != 0.0f && time <= 1.0f)
		m_screenMessageTime += 0.5f;
}

void CDrvSynHUDManager::ShowLastScreenMessage()
{
	m_screenMessageTime = 2.0f;
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

ConVar hud_map_pos("hud_map_pos", "0", "Map position (0 - bottom, 1 - top)", CV_ARCHIVE);

// render the screen with maps and shit
void CDrvSynHUDManager::Render( float fDt, const IVector2D& screenSize) // , const Matrix4x4& projMatrix, const Matrix4x4& viewMatrix )
{
	if( !r_drawHUD.GetBool() )
		return;

	m_curTime += fDt;

	materials->Setup2D(screenSize.x,screenSize.y);

	static IEqFont* roboto30 = g_fontCache->GetFont("Roboto", 30);
	static IEqFont* roboto30b = g_fontCache->GetFont("Roboto", 30, TEXT_STYLE_BOLD);
	static IEqFont* robotocon30b = g_fontCache->GetFont("Roboto Condensed", 30, TEXT_STYLE_BOLD);
		
	eqFontStyleParam_t fontParams;
	fontParams.styleFlag |= TEXT_STYLE_SHADOW;
	fontParams.textColor = color4_white;

	BlendStateParam_t blending;
	blending.srcFactor = BLENDFACTOR_SRC_ALPHA;
	blending.dstFactor = BLENDFACTOR_ONE_MINUS_SRC_ALPHA;

	if(m_enable)
	{
		bool inPursuit = (m_mainVehicle ? m_mainVehicle->GetPursuedCount() > 0 : false);

		Rectangle_t damageRect(35,65,410, 92);

		Vertex2D_t dmgrect[] = { MAKETEXQUAD(damageRect.vleftTop.x, damageRect.vleftTop.y,damageRect.vrightBottom.x, damageRect.vrightBottom.y, 0) };

		// Draw damage bar
		materials->DrawPrimitives2DFFP(PRIM_TRIANGLE_STRIP,dmgrect,elementsOf(dmgrect), NULL, ColorRGBA(0.435,0.435,0.435,0.35f), &blending);

		if( m_mainVehicle )
		{
			// fill the damage bar
			float fDamage = m_mainVehicle->GetDamage() / m_mainVehicle->GetMaxDamage();

			Vertex2D_t tmprect[] = { MAKETEXQUAD(damageRect.vleftTop.x, damageRect.vleftTop.y,lerp(damageRect.vleftTop.x, damageRect.vrightBottom.x, fDamage), damageRect.vrightBottom.y, 0) };

			// Cancel textures
			g_pShaderAPI->Reset(STATE_RESET_TEX);

			ColorRGBA damageColor = lerp(ColorRGBA(0,0.6f,0,0.5f), ColorRGBA(0.6f,0,0,0.5f), fDamage);

			if(fDamage > 0.85f)
			{
				damageColor = lerp(ColorRGBA(0.1f,0,0,0.5f), ColorRGBA(0.8f,0,0,0.5f), fabs(sin(m_curTime*3.0f)));
			}

			materials->DrawPrimitives2DFFP(PRIM_TRIANGLE_STRIP,tmprect,elementsOf(tmprect), NULL, damageColor, &blending);
		}

		Vector2D damageTextPos( damageRect.vleftTop.x+5, damageRect.vleftTop.y+15);

		robotocon30b->RenderText(m_damageTok ? m_damageTok->GetText() : L"Undefined", damageTextPos, fontParams);

		// Draw felony text
		float felonyPercent = 0.0f;

		bool hasFelony = false;

		if( m_mainVehicle )
		{
			felonyPercent = m_mainVehicle->GetFelony()*100;

			hasFelony = felonyPercent >= 10.0f;

			// if cars pursues me
			if(felonyPercent >= 10.0f && inPursuit)
			{
				float colorValue = clamp(sin(m_curTime*16.0)*16,-1,1); //sin(g_pHost->m_fGameCurTime*8.0f);

				float v1 = pow(-min(0,colorValue), 2.0f);
				float v2 = pow(max(0,colorValue), 2.0f);

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

		Vector2D felonyTextPos(damageRect.vleftTop.x, damageRect.vrightBottom.y+25);

		fontParams.styleFlag |= TEXT_STYLE_FROM_CAP;

		roboto30b->RenderText(varargs_w(m_felonyTok ? m_felonyTok->GetText() : L"Undefined", (int)felonyPercent), felonyTextPos, fontParams);

		if( m_mainVehicle && inPursuit )
		{
			static DkList<Vertex2D_t> copTriangles(32);

			for(int i = 0; i < g_pAIManager->m_copCars.numElem(); i++)
			{
				CAIPursuerCar* pursuer = g_pAIManager->m_copCars[i];

				if(!pursuer)
					continue;

				if(!pursuer->InPursuit())
					continue;

				Vector3D pursuerPos = pursuer->GetOrigin();

				float dist = length(m_mainVehicle->GetOrigin()-pursuerPos);

				Vector3D screenPos;
				PointToScreen_Z(pursuerPos, screenPos, g_pGameWorld->m_viewprojection, Vector2D(screenSize));

				if( screenPos.z < 0.0f && dist < 100.0f )
				{
					float distFadeFac = (100.0f - dist) / 100.0f;

					static Vertex2D_t copTriangle[3];
					copTriangle[0].m_vPosition = Vector2D(screenSize.x-(screenPos.x+40.0f), screenSize.y);
					copTriangle[1].m_vPosition = Vector2D(screenSize.x-(screenPos.x-40.0f), screenSize.y);
					copTriangle[2].m_vPosition = Vector2D(screenSize.x-screenPos.x, screenSize.y-90.0f);

					copTriangle[0].m_vColor = copTriangle[1].m_vColor = copTriangle[2].m_vColor = ColorRGBA(1,0,0,distFadeFac);

					copTriangles.append(copTriangle[0]);
					copTriangles.append(copTriangle[1]);
					copTriangles.append(copTriangle[2]);
				}
			}

			materials->DrawPrimitives2DFFP(PRIM_TRIANGLES,copTriangles.ptr(), copTriangles.numElem(), NULL, color4_white, &blending);

			copTriangles.clear(false);
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

			Vector2D timeDisplayTextPos(screenSize.x / 2, 80);

			wchar_t* str = varargs_w(L"%.2i:%.2i", mins, secs);
		
			float minSecWidth = numbers50->GetStringWidth(str, numFontParams.styleFlag);
			numbers50->RenderText(str, timeDisplayTextPos, numFontParams);

			float size = numbers50->GetStringWidth(varargs("%.2i:%.2i", mins, secs), numFontParams.styleFlag);

			numFontParams.align = 0;

			Vector2D millisDisplayTextPos = timeDisplayTextPos + Vector2D(floor(minSecWidth*0.5f), 0.0f);

			numbers20->RenderText(varargs_w(L"'%02d", millisecs), millisDisplayTextPos, numFontParams);
		}

		CViewParams& camera = *g_pGameWorld->GetCameraParams();

		// display radar and map
		if( m_showMap )
		{
			BlendStateParam_t mapBlending;
			mapBlending.srcFactor = BLENDFACTOR_SRC_ALPHA;
			mapBlending.dstFactor = BLENDFACTOR_ONE_MINUS_SRC_ALPHA;

			BlendStateParam_t additiveBlend;
			additiveBlend.srcFactor = BLENDFACTOR_ONE;
			additiveBlend.dstFactor = BLENDFACTOR_ONE;
			

			RasterizerStateParams_t raster;
			raster.scissor = true;
			raster.cullMode = CULL_FRONT;

			IVector2D mapSize(hud_mapSize.GetInt());
			IVector2D mapPos = screenSize-mapSize-IVector2D(55);

			if(hud_map_pos.GetInt() == 1)
			{
				mapPos.y = 55;
			}
			
			float viewRotation = DEG2RAD( camera.GetAngles().y + 180);
			Vector3D viewPos = Vector3D(camera.GetOrigin().xz() * Vector2D(1.0f,-1.0f), 0.0f);
			Vector2D playerPos(0);

			float mapZoom = hud_mapZoom.GetFloat() / HFIELD_POINT_SIZE;

			IRectangle mapRectangle(mapPos, mapPos+mapSize);

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

			ColorRGBA mapColor = ColorRGBA(1,1,1,0.5f);

			// draw the map rectangle
			materials->DrawPrimitives2DFFP(PRIM_TRIANGLE_STRIP, mapVerts, elementsOf(mapVerts), m_mapTexture, mapColor, &blending, NULL, &raster);

			Vertex2D_t targetFan[16];


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

				Vector2D copPos = pursuer->GetOrigin().xz() * Vector2D(-1.0f,1.0f);

				targetFan[0].m_vPosition = copPos + Vector2D(cop_forward.z, cop_forward.x)*10.0f;
				targetFan[0].m_vColor = ColorRGBA(1,1,1,1);

				float size = hasFelony ? AI_COPVIEW_FAR_WANTED : AI_COPVIEW_FAR;
				float angFac = hasFelony ? 36.0f : 12.0f;
				float angOffs = hasFelony ? 180.0f : -120.0f;

				for(int i = 1; i < 7; i++)
				{
					float ss,cs;
					float angle = float(i-1) * angFac + Yangle + angOffs;
					SinCos(DEG2RAD(angle), &ss, &cs);

					if(i == 1 || i == 6)
						targetFan[i].m_vPosition = copPos + Vector2D(ss, cs)*10.0f;
					else
						targetFan[i].m_vPosition = copPos + Vector2D(ss, cs)*size;

					targetFan[i].m_vColor = ColorRGBA(1,1,1,0.0f);
				}

				// draw cop car dot
				materials->DrawPrimitives2DFFP(PRIM_TRIANGLE_FAN,targetFan,7, NULL, ColorRGBA(1), &blending, NULL, &raster);
			}

			Vertex2D_t arrowFan[3];

			for(hudDisplayObjIterator_t iterator = m_displayObjects.begin(); iterator != m_displayObjects.end(); iterator++)
			{
				hudDisplayObject_t& obj = iterator->second;

				if(obj.flags & HUD_DOBJ_IS_TARGET)
				{
					Vector2D objPos = (obj.object ? obj.object->GetOrigin().xz() : obj.point.xz()) * Vector2D(-1.0f,1.0f);

					targetFan[0].m_vPosition = objPos;
					targetFan[0].m_vColor = ColorRGBA(1,1,1,1);

					for(int i = 1; i < 12; i++)
					{
						float ss,cs;
						float angle = float(i-1) * 36.0f;
						SinCos(DEG2RAD(angle), &ss, &cs);

						targetFan[i].m_vPosition = objPos + Vector2D(ss, cs) * obj.flashValue * 24.0f;
						targetFan[i].m_vColor = 0.0f;
					}

					obj.flashValue -= fDt;
					if(obj.flashValue < 0.0f)
						obj.flashValue = 1.0f;

					ColorRGBA arrowCol1(0.0f,0.0f,0.0f,0.5f);
					ColorRGBA arrowCol2(0.0f,0.0f,0.0f,0.15f);

					arrowFan[0].m_vPosition = objPos;
					arrowFan[0].m_vColor = arrowCol1;

					Vector2D targetDir = (objPos - playerPos);
					Vector2D ntargetDir = normalize(targetDir);
					Vector2D targetPerpendicular = Vector2D(-ntargetDir.y,ntargetDir.x);

					float distToTarg = length(targetDir);

					arrowFan[1].m_vPosition = playerPos - targetPerpendicular*distToTarg*0.25f;
					arrowFan[1].m_vColor = arrowCol2;

					arrowFan[2].m_vPosition = playerPos + targetPerpendicular*distToTarg*0.25f;
					arrowFan[2].m_vColor = arrowCol2;

					// draw target arrow
					materials->DrawPrimitives2DFFP(PRIM_TRIANGLE_FAN,arrowFan,3, NULL, ColorRGBA(1), &blending, NULL, &raster);

					// draw flashing dot
					materials->DrawPrimitives2DFFP(PRIM_TRIANGLE_FAN,targetFan,12, NULL, ColorRGBA(1), &additiveBlend, NULL, &raster);
				}
			}

			if(inPursuit)
			{
				float colorValue = sin(m_curTime*16.0);

				float v1 = pow(-min(0,colorValue), 2.0f);
				float v2 = pow(max(0,colorValue), 2.0f);

				m_radarBlank = 1.0f;

				materials->DrawPrimitives2DFFP(PRIM_TRIANGLE_STRIP,mapVerts,elementsOf(mapVerts), NULL, ColorRGBA(v1,0,v2,1), &additiveBlend, NULL, &raster);
			}

			Vertex2D_t plrFan[16];

			plrFan[0].m_vPosition = playerPos;
			plrFan[0].m_vColor = ColorRGBA(0,0,0,1);

			for(int i = 1; i < 16; i++)
			{
				float ss,cs;
				float angle = float(i-1) * 25.7142f;
				SinCos(DEG2RAD(angle), &ss, &cs);

				plrFan[i].m_vPosition = playerPos + Vector2D(ss, cs)*10.0f;
				plrFan[i].m_vColor = 0.0f;
			}

			// draw player car dot
			materials->DrawPrimitives2DFFP(PRIM_TRIANGLE_FAN,plrFan,elementsOf(plrFan), NULL, ColorRGBA(1), &blending, NULL, &raster);

			// draw radar blank after the pursuit ends
			if(!inPursuit && m_radarBlank > 0)
			{
				materials->DrawPrimitives2DFFP(PRIM_TRIANGLE_STRIP,mapVerts,elementsOf(mapVerts), NULL, ColorRGBA(m_radarBlank), &additiveBlend, NULL, &raster);
				m_radarBlank -= fDt;
			}
		}

		materials->SetMatrix(MATRIXMODE_VIEW, identity4());
		materials->SetMatrix(MATRIXMODE_WORLD, identity4());
	}

	// show message on screen
	if( m_screenMessageTime > 0 && fDt > 0.0f )
	{
		m_screenMessageTime -= fDt;

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

		Vector2D screenMessagePos(screenSize.x / 2, screenSize.y / 3 - textYOffs);

		roboto30b->RenderText(m_screenMessageText.c_str(), screenMessagePos, scrMsgParams);
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

#ifndef __INTELLISENSE__
OOLUA_EXPORT_FUNCTIONS(
	CDrvSynHUDManager, 
	AddTrackingObject, 
	AddMapTargetPoint,
	RemoveTrackingObject,
	ShowScreenMessage, 
	SetTimeDisplay,
	Enable,
	ShowMap,
	FadeIn,
	FadeOut
)

OOLUA_EXPORT_FUNCTIONS_CONST(
	CDrvSynHUDManager,
	IsEnabled,
	IsMapShown
)


#endif // __INTELLISENSE__