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

ConVar r_drawHUD("r_drawHUD", "1", "Draw Heads-Up display", CV_ARCHIVE);

#define MAP_ZOOM	80.0f

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

	m_damageTok = g_localizer->GetToken("HUD_DAMAGE_TITLE");
	m_felonyTok = g_localizer->GetToken("HUD_FELONY_TITLE");
}

void CDrvSynHUDManager::Cleanup()
{
	g_pShaderAPI->FreeTexture( m_mapTexture );
	m_mapTexture = NULL;

	for(int i = 0; i < m_displayObjects.numElem(); i++)
	{
		if(m_displayObjects[i].object)
			m_displayObjects[i].object->Ref_Drop();
	}

	m_displayObjects.clear();
	SetDisplayMainVehicle(NULL);
}

void CDrvSynHUDManager::ShowScreenMessage( const char* token, float time )
{
	m_screenMessageText = LocalizedString(token);
	m_screenMessageTime = time;
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

		if( m_mainVehicle )
		{
			felonyPercent = m_mainVehicle->GetFelony()*100;

			// if cars pursues me
			if(felonyPercent >= 10.0f && m_mainVehicle->GetPursuedCount() > 0)
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

		if( m_mainVehicle && m_mainVehicle->GetPursuedCount() > 0 )
		{
			static DkList<Vertex2D_t> copTriangles(32);

			for(int i = 0; i < g_pAIManager->m_copCars.numElem(); i++)
			{
				CAIPursuerCar* pursuer = g_pAIManager->m_copCars[i];

				if(!pursuer)
					continue;

				if(!pursuer->IsAlive())
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
			IVector2D mapSize(250,250);
			IVector2D mapPos = screenSize-mapSize-IVector2D(55,55);

			float fWidthRate = mapSize.y / mapSize.x;
			
			Rectangle_t mapRect(mapPos, mapPos+mapSize);
			Vertex2D_t tmprect[] = { MAKETEXQUAD(mapRect.vleftTop.x, mapRect.vleftTop.y,mapRect.vrightBottom.x, mapRect.vrightBottom.y, 0) };

			Vector2D imgSize(1.0f);
			
			if(m_mapTexture)
				imgSize = Vector2D(m_mapTexture->GetWidth(),m_mapTexture->GetHeight());

			Vector2D invMapSize = 1.0f / imgSize;

			Vector2D mapCoords[] = {
				Vector2D(-1, -1),
				Vector2D(-1, 1),
				Vector2D(1, -1),
				Vector2D(1, 1),
			};

			Vector2D worldSize( g_pGameWorld->m_level.m_wide*g_pGameWorld->m_level.m_cellsSize, g_pGameWorld->m_level.m_tall*g_pGameWorld->m_level.m_cellsSize);

			Vector2D worldToImg = (1.0f / (imgSize*HFIELD_POINT_SIZE));

			Vector2D camOnMap = camera.GetOrigin().xz() * Vector2D(-1,1);
			Matrix2x2 camOnMapRot = rotate2( -DEG2RAD( camera.GetAngles().y + 180) );

			if(m_mainVehicle)
			{
				Vector3D car_forward = m_mainVehicle->GetForwardVector();

				float Yangle = RAD2DEG(atan2f(car_forward.z, car_forward.x));

				if (Yangle < 0.0)
					Yangle += 360.0f;

				camOnMap = m_mainVehicle->GetOrigin().xz() * Vector2D(-1,1);
				camOnMapRot = rotate2( -DEG2RAD( Yangle + 90) );
			}

			for(int i = 0; i < 4; i++)
			{
				Vector2D rotated = camOnMapRot*mapCoords[i];

				Vector2D texCoord = (Vector2D(0.5f,0.5f) + camOnMap*worldToImg + rotated*invMapSize*Vector2D(fWidthRate,1.0f)*MAP_ZOOM);

				tmprect[i].m_vTexCoord = texCoord;
			}

			Vertex2D_t plrFan[16];

			plrFan[0].m_vPosition = mapRect.GetCenter();
			plrFan[0].m_vColor = ColorRGBA(0,0,0,1);

			for(int i = 1; i < 16; i++)
			{
				plrFan[i].m_vPosition = plrFan[0].m_vPosition + Vector2D(sin((float)i-1/15.0f), cos((float)i-1/15.0f))*4.0f;
				plrFan[i].m_vColor = 0.0f;
			}

			BlendStateParam_t blending;
			blending.srcFactor = BLENDFACTOR_SRC_ALPHA;
			blending.dstFactor = BLENDFACTOR_ONE_MINUS_SRC_ALPHA;

			materials->DrawPrimitives2DFFP(PRIM_TRIANGLE_STRIP,tmprect,elementsOf(tmprect), m_mapTexture, ColorRGBA(1,1,1,0.8f), &blending);
			materials->DrawPrimitives2DFFP(PRIM_TRIANGLE_FAN,plrFan,elementsOf(plrFan), NULL, ColorRGBA(1), &blending);

			// display objects on map
		}
	}

	// show message on screen
	if( m_screenMessageTime > 0 && fDt > 0.0f )
	{
		m_screenMessageTime -= fDt;

		eqFontStyleParam_t scrMsgParams;
		scrMsgParams.styleFlag |= TEXT_STYLE_SHADOW | TEXT_STYLE_USE_TAGS;
		scrMsgParams.align = TEXT_ALIGN_HCENTER;
		scrMsgParams.textColor = ColorRGBA(1,1,0.25f,1);

		Vector2D screenMessagePos(screenSize.x / 2, screenSize.y / 3);

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
	return -1;
}

void CDrvSynHUDManager::RemoveTrackingObject( int handle )
{

}

#ifndef __INTELLISENSE__
OOLUA_EXPORT_FUNCTIONS(
	CDrvSynHUDManager, 
	AddTrackingObject, 
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