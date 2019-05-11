//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Title screen
//////////////////////////////////////////////////////////////////////////////////

#include "state_mainmenu.h"
#include "state_game.h"

#include "in_keys_ident.h"
#include "sys_host.h"
#include "FontCache.h"

#include "KeyBinding/InputCommandBinder.h"

#include "materialsystem/MeshBuilder.h"

enum
{
	MENU_ENTER = 1,
	MENU_BACK
};

CState_MainMenu::CState_MainMenu()
{
	m_uiLayout = nullptr;
	m_menuDummy = nullptr;
}

CState_MainMenu::~CState_MainMenu()
{

}

void CState_MainMenu::OnEnter( CBaseStateHandler* from )
{
	g_sounds->Init(100.0f);

	g_sounds->PrecacheSound( "menu.back" );
	g_sounds->PrecacheSound( "menu.roll" );
	g_sounds->PrecacheSound( "menu.click" );
	g_sounds->PrecacheSound("menu.music");

	m_fade = 0.0f;
	m_textFade = 0.0f;
	m_changesMenu = 0;
	m_goesFromMenu = false;

	soundsystem->SetPauseState(false);
	sndEffect_t* reverb = soundsystem->FindEffect( "menu_reverb" );

	soundsystem->SetListener(vec3_zero, vec3_forward, vec3_up, vec3_zero, reverb);

	//-------------------------

	OOLUA::Table mainMenuStack;
	if(!OOLUA::get_global(GetLuaState(), "MainMenuStack", mainMenuStack))
	{
		WarningMsg("Failed to get MainMenuStack table (DrvSynMenus.lua ???)!\n");
	}

	SetMenuObject( mainMenuStack );

	//EmitSound_t es("music.menu", EMITSOUND_FLAG_FORCE_CACHED);
	//g_sounds->Emit2DSound( &es );

	// init hud layout
	m_uiLayout = equi::Manager->CreateElement("Panel");

	kvkeybase_t uiKvs;

	if (KV_LoadFromFile("resources/ui_mainmenu.res", SP_MOD, &uiKvs))
		m_uiLayout->InitFromKeyValues(&uiKvs);

	m_menuDummy = m_uiLayout->FindChild("Menu");

	if (!m_menuDummy)
	{
		m_menuDummy = equi::Manager->CreateElement("HudElement");
		m_menuDummy->SetPosition(0);
		m_menuDummy->SetSize(IVector2D(640, 480));
		m_menuDummy->SetScaling(equi::UI_SCALING_ASPECT_H);
		m_menuDummy->SetTextAlignment(TEXT_ALIGN_HCENTER);

		m_uiLayout->AddChild(m_menuDummy);
	}

	EmitSound_t es("menu.music");
	g_sounds->Emit2DSound(&es);
}

void CState_MainMenu::OnEnterSelection( bool isFinal )
{
	m_goesFromMenu = isFinal;
}

void CState_MainMenu::OnLeave( CBaseStateHandler* to )
{
	delete m_uiLayout;
	m_uiLayout = m_menuDummy = nullptr;
	g_sounds->Shutdown();
}

bool CState_MainMenu::Update( float fDt )
{
	if( m_goesFromMenu )
	{
		m_fade -= fDt*5.0f;
		m_textFade -= fDt*8.0f;
	}
	else
	{
		m_fade += fDt;

		if(m_changesMenu > 0)
			m_textFade -= fDt*3.0f;
		else
			m_textFade += fDt*6.0f;
	}

	const IVector2D& screenSize = g_pHost->GetWindowSize();

	m_fade = clamp(m_fade, 0.0f, 1.0f);
	m_textFade = clamp(m_textFade, 0.0f, 1.0f);

	materials->Setup2D(screenSize.x,screenSize.y);;
	g_pShaderAPI->Clear( true,true, false, ColorRGBA(0));

	IVector2D halfScreen(screenSize.x/2, screenSize.y/2);

	m_uiLayout->SetSize(screenSize);
	m_uiLayout->Render();

	//materials->SetMatrix(MATRIXMODE_VIEW, rotateZ4(DEG2RAD(-7)));

	if (!m_menuDummy)
		return true;

	IEqFont* font = m_menuDummy->GetFont();

	CMeshBuilder meshBuilder(materials->GetDynamicMesh());

	BlendStateParam_t blending;
	blending.srcFactor = BLENDFACTOR_SRC_ALPHA;
	blending.dstFactor = BLENDFACTOR_ONE_MINUS_SRC_ALPHA;

	Vector2D menuScaling = m_menuDummy->CalcScaling();

	{
		Vector2D screenMessagePos(m_menuDummy->GetPosition()*menuScaling);
		screenMessagePos.y -= 45;

		Vertex2D_t verts[6];

		const float messageSizeY = 55.0f;

		Vertex2D_t baseVerts[] = { MAKETEXQUAD(0.0f, screenMessagePos.y, screenSize.x, screenMessagePos.y + messageSizeY, 0.0f) };

		baseVerts[0].color.w = 0.0f;
		baseVerts[1].color.w = 0.0f;
		baseVerts[2].color.w = 0.0f;
		baseVerts[3].color.w = 0.0f;

		verts[0] = baseVerts[0];
		verts[1] = baseVerts[1];

		verts[4] = baseVerts[2];
		verts[5] = baseVerts[3];

		verts[2] = Vertex2D_t::Interpolate(verts[0], verts[4], 0.25f);
		verts[3] = Vertex2D_t::Interpolate(verts[1], verts[5], 0.25f);

		verts[2].color.w = 1.0f;
		verts[3].color.w = 1.0f;

		float clampedAlertTime = clamp(m_fade, 0.0f, 1.0f);

		float alpha = clampedAlertTime;

		ColorRGBA alertColor(1.0f, 0.7f, 0.0f, alpha);

		materials->DrawPrimitives2DFFP(PRIM_TRIANGLE_STRIP, verts, elementsOf(verts), NULL, alertColor, &blending);

		eqFontStyleParam_t scrMsgParams;
		scrMsgParams.styleFlag |= TEXT_STYLE_SHADOW | TEXT_STYLE_USE_TAGS;
		scrMsgParams.textColor = ColorRGBA(1, 1, 1, alpha);
		scrMsgParams.scale = 30.0f;

		// ok for 3 seconds

		float textXPos = -2000.0f * pow(1.0f-m_textFade, 4.0f) + (-2000.0f * pow(1.0f - clampedAlertTime, 4.0f));

		font->RenderText(m_menuTitleStr.c_str(), screenMessagePos + Vector2D(textXPos, messageSizeY*0.7f), scrMsgParams);
	}

	eqFontStyleParam_t fontParam;
	fontParam.align = m_menuDummy->GetTextAlignment();
	fontParam.styleFlag |= TEXT_STYLE_SHADOW;
	fontParam.textColor = color4_white;
	fontParam.scale = m_menuDummy->GetFontScale()*menuScaling;

	Vector2D menuPos = m_menuDummy->GetPosition()*menuScaling;

	{
		EqLua::LuaStackGuard g(GetLuaState());

		oolua_ipairs(m_menuElems)
			int idx = _i_index_-1;

			OOLUA::Table elem;
			m_menuElems.safe_at(_i_index_, elem);

			const wchar_t* token = GetMenuItemString(elem);

			if(m_selection == idx)
				fontParam.textColor = ColorRGBA(1,0.7f,0.0f,pow(m_textFade,5.0f));
			else
				fontParam.textColor = ColorRGBA(1,1,1,pow(m_textFade,5.0f));

			if((m_goesFromMenu || m_changesMenu == MENU_ENTER) && m_changesMenu != MENU_BACK)
			{
				if(m_selection == idx)
				{
					float sqAlphaValue = clamp(sinf(m_textEffect*32.0f)*16,0.0f,1.0f); //sin(g_pHost->m_fGameCurTime*8.0f);

					if(m_textEffect > 0.75f)
						fontParam.textColor.w = sqAlphaValue;
					else
					{
						float fadeVal = (m_textEffect-0.25f) / 0.75f;
						if(fadeVal < 0)
							fadeVal = 0;

						fontParam.textColor.w = fadeVal;
					}
				}
				else
				{
					fontParam.textColor.w = pow(m_textFade,10.0f);
				}
			}

			Vector2D elemPos(menuPos.x, menuPos.y+_i_index_*font->GetLineHeight(fontParam));

			font->RenderText(token, elemPos, fontParam);
		oolua_ipairs_end()
	}

	materials->SetMatrix(MATRIXMODE_VIEW, identity4());

	g_pShaderAPI->SetTexture(NULL, NULL, 0);
	materials->SetBlendingStates(blending);
	materials->SetRasterizerStates(CULL_FRONT, FILL_SOLID);
	materials->SetDepthStates(false, false);
	materials->BindMaterial(materials->GetDefaultMaterial());

	if (m_goesFromMenu)
	{
		ColorRGBA blockCol(0, 0, 0, 1.0f);

		float fadeVal = 1.0f - m_fade;

		Vector2D rectTop[] = { MAKEQUAD(0, 0,screenSize.x, screenSize.y*fadeVal*0.5f, 0) };
		Vector2D rectBot[] = { MAKEQUAD(0, screenSize.y*0.5f + screenSize.y*(1.0f - fadeVal)*0.5f,screenSize.x, screenSize.y, 0) };

		meshBuilder.Begin(PRIM_TRIANGLE_STRIP);
			meshBuilder.Color4fv(blockCol);
			meshBuilder.Quad2(rectTop[0], rectTop[1], rectTop[2], rectTop[3]);
			meshBuilder.Quad2(rectBot[0], rectBot[1], rectBot[2], rectBot[3]);
		meshBuilder.End();
	}
	else
	{
		ColorRGBA blockCol(0, 0, 0, 1.0f - pow(m_fade, 2.0f));

		Vector2D screenRect[] = { MAKEQUAD(0, 0,screenSize.x, screenSize.y, 0) };

		meshBuilder.Begin(PRIM_TRIANGLE_STRIP);
			meshBuilder.Color4fv(blockCol);
			meshBuilder.Quad2(screenRect[0], screenRect[1], screenRect[2], screenRect[3]);
		meshBuilder.End();
	}

	m_textEffect -= fDt;

	if(m_textEffect < 0)
		m_textEffect = 0.0f;

	if(m_changesMenu > 0 && m_textFade <= 0.0f)
	{
		if(m_changesMenu == MENU_ENTER)
		{
			EnterSelection();
		}
		else if(m_changesMenu == MENU_BACK)
		{
			PopMenu();
		}

		m_changesMenu = 0;
	}

	// fade music
	if(m_goesFromMenu)
		g_sounds->Set2DChannelsVolume(CHAN_STREAM, 0.0f);

	return !(m_goesFromMenu && m_fade == 0.0f);
}

void CState_MainMenu::HandleKeyPress( int key, bool down )
{
	if(m_changesMenu > 0)
		return;

	if(m_goesFromMenu)
		return;

	if(down)
	{
		if (key == KEY_ENTER || key == KEY_JOY_A)
		{
			Event_SelectionEnter();
		}
		else if (key == KEY_LEFT || key == KEY_RIGHT || key == KEY_JOY_DPAD_LEFT || key == KEY_JOY_DPAD_RIGHT)
		{
			int direction = (key == KEY_LEFT || key == KEY_JOY_DPAD_LEFT) ? -1 : 1;

			if (ChangeSelection(key == KEY_LEFT ? -1 : 1))
			{
				EmitSound_t es("menu.roll");
				g_sounds->EmitSound(&es);
			}
		}
		else if (key == KEY_ESCAPE || key == KEY_JOY_START || key == KEY_JOY_Y)
		{
			Event_BackToPrevious();
		}
		else if (key == KEY_UP || key == KEY_JOY_DPAD_UP)
		{
			Event_SelectionUp();
		}
		else if (key == KEY_DOWN || key == KEY_JOY_DPAD_DOWN)
		{
			Event_SelectionDown();
		}
	}

	g_inputCommandBinder->OnKeyEvent( key, down );
}

void CState_MainMenu::HandleMouseMove( int x, int y, float deltaX, float deltaY )
{
	if (!m_menuDummy)
		return;

	// perform a hit test
	const IVector2D& screenSize = g_pHost->GetWindowSize();
	IVector2D halfScreen(screenSize.x/2, screenSize.y/2);

	IEqFont* font = m_menuDummy->GetFont();

	Vector2D menuScaling = m_menuDummy->CalcScaling();

	eqFontStyleParam_t fontParam;
	fontParam.align = m_menuDummy->GetTextAlignment();
	fontParam.styleFlag |= TEXT_STYLE_SHADOW;
	fontParam.textColor = color4_white;
	fontParam.scale = m_menuDummy->GetFontScale()*menuScaling;

	Vector2D menuPos = m_menuDummy->GetPosition()*menuScaling;

	{
		EqLua::LuaStackGuard g(GetLuaState());

		oolua_ipairs(m_menuElems)
			int idx = _i_index_-1;

			OOLUA::Table elem;
			m_menuElems.safe_at(_i_index_, elem);

			float lineWidth = 400;
			float lineHeight = font->GetLineHeight(fontParam);

			Vector2D elemPos(menuPos.x, menuPos.y+_i_index_*lineHeight);

			Rectangle_t rect(elemPos - Vector2D(0, lineHeight), elemPos + Vector2D(lineWidth, 0));

			if(rect.IsInRectangle(Vector2D(x,y)))
				Event_SelectMenuItem( idx );

		oolua_ipairs_end()
	}
}

void CState_MainMenu::HandleMouseClick( int x, int y, int buttons, bool down )
{
	if(buttons == MOU_B1 && !down)
		Event_SelectionEnter();
}

void CState_MainMenu::Event_BackToPrevious()
{
	EmitSound_t es("menu.back");
	g_sounds->EmitSound( &es );

	if(IsCanPopMenu())
	{
		m_changesMenu = MENU_BACK;
		m_textEffect = 1.0f;
		m_textFade = 1.0f;
	}
	else
	{
		m_changesMenu = MENU_BACK;
		m_goesFromMenu = true;
		//m_textEffect = 1.0f;
		//m_textFade = 1.0f;
		SetNextState(g_states[GAME_STATE_TITLESCREEN]);
	}
}

void CState_MainMenu::Event_SelectionEnter()
{
	if(m_changesMenu > 0)
		return;

	if(m_selection == -1)
		return;

	if (PreEnterSelection())
	{
		EmitSound_t es("menu.click");
		g_sounds->EmitSound(&es);

		m_changesMenu = MENU_ENTER;
		m_textEffect = 1.0f;
		m_textFade = 1.0f;
	}
}

void CState_MainMenu::Event_SelectionUp()
{
	if(m_goesFromMenu || m_changesMenu > 0)
		return;

redecrement:
	m_selection--;

	if(m_selection < 0)
	{
		int nItem = 0;
		m_selection = m_numElems-1;
	}

	//if(pItem->type == MIT_SPACER)
	//	goto redecrement;

	EmitSound_t ep("menu.roll");
	g_sounds->EmitSound(&ep);
}

void CState_MainMenu::Event_SelectionDown()
{
	if(m_goesFromMenu || m_changesMenu > 0)
		return;

reincrement:
	m_selection++;

	if(m_selection > m_numElems-1)
		m_selection = 0;

	//if(pItem->type == MIT_SPACER)
	//	goto reincrement;

	EmitSound_t ep("menu.roll");
	g_sounds->EmitSound(&ep);
}

void CState_MainMenu::Event_SelectMenuItem(int index)
{
	if(m_goesFromMenu || m_changesMenu > 0)
		return;

	if(index > m_numElems-1)
		return;

	if(m_selection == index)
		return;

	m_selection = index;

	EmitSound_t ep("menu.roll");
	g_sounds->EmitSound(&ep);
}

CState_MainMenu* g_State_MainMenu = new CState_MainMenu();
