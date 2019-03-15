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
}

void CState_MainMenu::OnEnterSelection( bool isFinal )
{
	m_goesFromMenu = isFinal;
}

void CState_MainMenu::OnLeave( CBaseStateHandler* to )
{
	delete m_uiLayout;
	m_uiLayout = nullptr;
	g_sounds->Shutdown();
}

bool CState_MainMenu::Update( float fDt )
{
	if( m_goesFromMenu )
	{
		m_fade -= fDt;
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

	materials->Setup2D(screenSize.x,screenSize.y);
	g_pShaderAPI->Clear( true,true, false, ColorRGBA(0.25f,0,0,0.0f));

	IVector2D halfScreen(screenSize.x/2, screenSize.y/2);

	m_uiLayout->SetSize(screenSize);
	m_uiLayout->Render();

	if (!m_menuDummy)
		return true;

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

			Vector2D elemPos(menuPos.x, menuPos.y+idx*font->GetLineHeight(fontParam));

			font->RenderText(token, elemPos, fontParam);
		oolua_ipairs_end()
	}

	Vector2D screenRect[] = { MAKEQUAD(0, 0,screenSize.x, screenSize.y, 0) };

	CMeshBuilder meshBuilder(materials->GetDynamicMesh());

	BlendStateParam_t blending;
	blending.srcFactor = BLENDFACTOR_SRC_ALPHA;
	blending.dstFactor = BLENDFACTOR_ONE_MINUS_SRC_ALPHA;

	ColorRGBA blockCol(0,0,0, 1.0f-pow(m_fade, 2.0f));

	g_pShaderAPI->SetTexture(NULL, NULL, 0);
	materials->SetBlendingStates(blending);
	materials->SetRasterizerStates(CULL_FRONT, FILL_SOLID);
	materials->SetDepthStates(false, false);
	materials->BindMaterial(materials->GetDefaultMaterial());

	meshBuilder.Begin(PRIM_TRIANGLE_STRIP);
		meshBuilder.Color4fv(blockCol);
		meshBuilder.Quad2(screenRect[0], screenRect[1], screenRect[2], screenRect[3]);
	meshBuilder.End();

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
		if(key == KEY_ENTER)
		{
			Event_SelectionEnter();
		}
		else if (key == KEY_LEFT || key == KEY_RIGHT)
		{
			if (ChangeSelection(key == KEY_LEFT ? -1 : 1))
			{
				EmitSound_t es("menu.roll");
				g_sounds->EmitSound(&es);
			}
		}
		else if(key == KEY_ESCAPE)
		{
			Event_BackToPrevious();
		}
		else if(key == KEY_UP)
		{
			Event_SelectionUp();
		}
		else if(key == KEY_DOWN)
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

			Vector2D elemPos(menuPos.x, menuPos.y+idx*lineHeight);

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

	EmitSound_t es("menu.click");
	g_sounds->EmitSound( &es );

	m_changesMenu = MENU_ENTER;
	m_textEffect = 1.0f;
	m_textFade = 1.0f;

	PreEnterSelection();
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
