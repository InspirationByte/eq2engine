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

enum EMenuMode
{
	MENUMODE_NONE = 0,
	MENUMODE_ENTER,
	MENUMODE_BACK,

	MENUMODE_INPUT_WAITER,
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
	m_menuMode = MENUMODE_NONE;
	m_goesFromMenu = false;

	ResetScroll();

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

	{
		Vector2D menuScaling = m_menuDummy->CalcScaling();

		eqFontStyleParam_t fontParam;
		fontParam.align = m_menuDummy->GetTextAlignment();
		fontParam.styleFlag |= TEXT_STYLE_SHADOW;
		fontParam.textColor = color4_white;
		fontParam.scale = m_menuDummy->GetFontScale()*menuScaling;

		IEqFont* font = m_menuDummy->GetFont();

		float lineHeight = font->GetLineHeight(fontParam);

		m_maxMenuItems = floor((m_menuDummy->GetSize().y * menuScaling.y) / lineHeight);
	}


	EmitSound_t es("menu.music");
	g_sounds->Emit2DSound(&es);

	ResetKeys();
	m_keysError = false;
}

void CState_MainMenu::OnEnterSelection( bool isFinal )
{
	m_goesFromMenu = isFinal;
}

void CState_MainMenu::OnMenuCommand(const char* command)
{
	if (!stricmp(command, "bindAction"))
	{
		OOLUA::Table elem;
		if (GetCurrentMenuElement(elem))
		{
			std::string actionName;
			elem.safe_at("inputActionName", actionName);

			ResetKeys();
			m_keysError = false;

			m_menuMode = MENUMODE_INPUT_WAITER;
		}
	}
	else if (!stricmp(command, "unbindAction"))
	{
		OOLUA::Table elem;
		if (GetCurrentMenuElement(elem))
		{
			std::string actionName;
			elem.safe_at("inputActionName", actionName);

			g_inputCommandBinder->UnbindCommandByName(actionName.c_str());
		}
	}
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

		if (m_menuMode != MENUMODE_INPUT_WAITER)
		{
			if (m_menuMode > 0)
				m_textFade -= fDt * 3.0f;
			else
				m_textFade += fDt * 6.0f;
		}
	}

	const IVector2D& screenSize = g_pHost->GetWindowSize();

	m_fade = clamp(m_fade, 0.0f, 1.0f);
	m_textFade = clamp(m_textFade, 0.0f, 1.0f);

	materials->Setup2D(screenSize.x,screenSize.y);

	materials->SetAmbientColor(color4_white);

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

	Vector2D screenMessagePos(m_menuDummy->GetPosition()*menuScaling);
	screenMessagePos.y -= 65;

	// interpolate scroll smoothly
	m_menuScrollInterp = lerp(m_menuScrollInterp, m_menuScrollTarget, fDt*16.0f);

	eqFontStyleParam_t fontParam;
	fontParam.align = m_menuDummy->GetTextAlignment();
	fontParam.styleFlag |= TEXT_STYLE_SHADOW | TEXT_STYLE_FROM_CAP;
	fontParam.textColor = color4_white;
	fontParam.scale = m_menuDummy->GetFontScale()*menuScaling;

	Vector2D menuPos = m_menuDummy->GetPosition()*menuScaling;
	Vector2D menuSize = m_menuDummy->GetSize()*menuScaling;

	DkList<OOLUA::Table> menuItems;
	{
		EqLua::LuaStackGuard g(GetLuaState());

		oolua_ipairs(m_menuElems)

			OOLUA::Table elem;
			m_menuElems.safe_at(_i_index_, elem);

			menuItems.append(elem);

		oolua_ipairs_end()
	}

	{
		EqLua::LuaStackGuard g(GetLuaState());

		for (int i = 0; i < menuItems.numElem(); i++)
		{
			OOLUA::Table& elem = menuItems[i];

			const wchar_t* token = GetMenuItemString(elem);

			if (m_selection == i)
				fontParam.textColor = ColorRGBA(1, 0.7f, 0.0f, pow(m_textFade, 5.0f));
			else
				fontParam.textColor = ColorRGBA(1, 1, 1, pow(m_textFade, 5.0f));

			if ((m_goesFromMenu || m_menuMode == MENUMODE_ENTER) && m_menuMode != MENUMODE_BACK)
			{
				if (m_selection == i)
				{
					float sqAlphaValue = clamp(sinf(m_textEffect*32.0f) * 16, 0.0f, 1.0f); //sin(g_pHost->m_fGameCurTime*8.0f);

					if (m_textEffect > 0.75f)
						fontParam.textColor.w = sqAlphaValue;
					else
					{
						float fadeVal = (m_textEffect - 0.25f) / 0.75f;
						if (fadeVal < 0)
							fadeVal = 0;

						fontParam.textColor.w = fadeVal;
					}
				}
				else
				{
					fontParam.textColor.w = pow(m_textFade, 10.0f);
				}
			}

			float lineHeight = font->GetLineHeight(fontParam);

			Vector2D elemPos(menuPos.x, menuPos.y + i * lineHeight + m_menuScrollInterp*lineHeight);

			if (elemPos.y < menuPos.y)
			{
				fontParam.textColor.w *= 1.0f - ((menuPos.y - elemPos.y) / 200.0f);

				if (i == m_selection && m_selection < -m_menuScrollTarget)
					m_menuScrollTarget = -m_selection;
			}
			else if (elemPos.y > menuPos.y + menuSize.y)
			{
				fontParam.textColor.w *= 1.0f - ((elemPos.y - (menuPos.y + menuSize.y)) / 80.0f);

				if (i == m_selection && m_selection > -m_menuScrollTarget)
					m_menuScrollTarget = -m_selection;
			}
				

			font->RenderText(token, elemPos, fontParam);

			std::string inputActionName;
			if (elem.safe_at("inputActionName", inputActionName))
			{
				EqString boundKeys;

				in_binding_t* binding = nullptr;
				while (binding = g_inputCommandBinder->FindBindingByCommandName(inputActionName.c_str(), nullptr, binding))
				{
					EqString bindStr;
					UTIL_GetBindingKeyString(bindStr, binding);

					if (boundKeys.Length() > 0)
						boundKeys.Append(", ");

					boundKeys.Append(bindStr);
				}

				EqWString boundKeysWString = boundKeys.Length() > 0 ? boundKeys.c_str() : "-";

				eqFontStyleParam_t inputActionFontParam(fontParam);

				if (m_menuMode == MENUMODE_INPUT_WAITER && m_selection == i)
				{
					EqString keysStr;
					GetEnteredKeysString(keysStr);

					EqWString keysWStr(keysStr);
					boundKeysWString = varargs_w(LocalizedString("#MENU_SETTINGS_CONTROL_PRESSKEY"), keysWStr.c_str());

					if (m_keysError)
					{
						inputActionFontParam.textColor = ColorRGBA(1, 0, 0, 1);
						boundKeysWString = varargs_w(LocalizedString("#MENU_SETTINGS_CONTROL_ALREADY_BOUND"), keysWStr.c_str());
					}
				}

				font->RenderText(boundKeysWString.c_str(), elemPos + Vector2D(160.0f, 0.0f)*menuScaling, inputActionFontParam);
			}

		}
	}

	{
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
		float textXPos = -2000.0f * pow(1.0f - m_textFade, 4.0f) + (-2000.0f * pow(1.0f - clampedAlertTime, 4.0f));

		font->RenderText(m_menuTitleStr.c_str(), screenMessagePos + Vector2D(textXPos, messageSizeY*0.7f), scrMsgParams);
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

	if(m_menuMode != MENUMODE_INPUT_WAITER && m_menuMode > 0 && m_textFade <= 0.0f || m_goesFromMenu)
	{
		if(m_menuMode == MENUMODE_ENTER)
		{
			EnterSelection();
		}
		else if(m_menuMode == MENUMODE_BACK)
		{
			PopMenu();
		}

		ResetScroll();

		m_menuMode = 0;
	}

	// fade music
	if(m_goesFromMenu)
		g_sounds->Set2DChannelsVolume(CHAN_STREAM, 0.0f);

	return !(m_goesFromMenu && m_fade == 0.0f);
}

void CState_MainMenu::ResetScroll()
{
	m_menuScrollInterp = m_menuScrollTarget = 0.0f;
}

void CState_MainMenu::ResetKeys()
{
	memset(m_keysEntered, 0, sizeof(m_keysEntered));
	m_keysPos = 0;
}

void CState_MainMenu::GetEnteredKeysString(EqString& keysStr)
{
	int* currKey = m_keysEntered;

	for (; *currKey; currKey++)
	{
		if (currKey > m_keysEntered)
			keysStr.Append('+');

		keysStr.Append(KeyIndexToString(*currKey));
	}
}

// mapping groups

bool CState_MainMenu::MapKeysToCurrentAction()
{
	OOLUA::Table elem;
	if (!GetCurrentMenuElement(elem))
		return true;

	EqString keysStr;
	GetEnteredKeysString(keysStr);

	std::string actionNameStr;
	if (elem.safe_at("inputActionName", actionNameStr))
	{
		in_binding_t* existingBinding;
		while (existingBinding = g_inputCommandBinder->FindBinding(keysStr.c_str()))
		{
			g_inputCommandBinder->DeleteBinding(existingBinding);
		}

		g_inputCommandBinder->BindKey(keysStr.c_str(), actionNameStr.c_str(), "");
	}

	return true;
}

void CState_MainMenu::HandleKeyPress( int key, bool down )
{
	if (m_goesFromMenu)
		return;

	if (m_menuMode == MENUMODE_INPUT_WAITER)
	{
		if (down)
		{
			// cancel input waiter
			if (key == KEY_ESCAPE || key == KEY_JOY_START)
			{
				ResetKeys();

				EmitSound_t es("menu.back");
				g_sounds->EmitSound(&es);

				m_menuMode = MENUMODE_NONE;
				return;
			}
			else
			{
				// don't add already pressed key
				int *currKey = m_keysEntered;
				for (; *currKey; currKey++)
				{
					if (*currKey == key)
						return;
				}

				m_keysEntered[m_keysPos++] = key;
				m_keysError = false;

				if (m_keysPos >= 16)
					ResetKeys();

				return;
			}
		}
		else
		{
			// bind command
			if(MapKeysToCurrentAction())
				m_menuMode = MENUMODE_NONE;

			return;
		}
	}
	else if(m_menuMode != MENUMODE_NONE)
		return;

	if(down)
	{
		if (key == KEY_LEFT || key == KEY_RIGHT || key == KEY_JOY_DPAD_LEFT || key == KEY_JOY_DPAD_RIGHT)
		{
			int direction = (key == KEY_LEFT || key == KEY_JOY_DPAD_LEFT) ? -1 : 1;

			if (ChangeSelection(direction))
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
	else
	{
		if (key == KEY_ENTER || key == KEY_JOY_A)
		{
			Event_SelectionEnter();
		}
		else if (key == KEY_DELETE || key == KEY_JOY_B)
		{
			Event_SelectionEnter("onDelete");
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

			Vector2D elemPos(menuPos.x, menuPos.y+_i_index_*lineHeight + m_menuScrollInterp * lineHeight);

			Rectangle_t rect(elemPos - Vector2D(0, lineHeight), elemPos + Vector2D(lineWidth, 0));

			if(rect.IsInRectangle(Vector2D(x,y)))
				Event_SelectMenuItem( idx );

		oolua_ipairs_end()
	}
}

void CState_MainMenu::HandleMouseWheel(int x, int y, int scroll)
{
	if (scroll > 0)
		Event_SelectionUp();
	else
		Event_SelectionDown();

	/*
	m_menuScrollTarget += scroll;

	if (m_menuScrollTarget > 0)
		m_menuScrollTarget = 0;

	if (m_menuScrollTarget < -m_numElems)
		m_menuScrollTarget = -m_numElems;
	*/
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
		m_menuMode = MENUMODE_BACK;
		m_textEffect = 1.0f;
		m_textFade = 1.0f;
	}
	else
	{
		m_menuMode = MENUMODE_BACK;
		m_goesFromMenu = true;
		//m_textEffect = 1.0f;
		//m_textFade = 1.0f;
		SetNextState(g_states[GAME_STATE_TITLESCREEN]);
	}
}

void CState_MainMenu::Event_SelectionEnter(const char* actionName /*= "onEnter"*/)
{
	if(m_menuMode > 0)
		return;

	OOLUA::Table elem;
	if (GetCurrentMenuElement(elem))
	{
		EmitSound_t es(!stricmp(actionName, "onDelete") ? "menu.back" : "menu.click");
		g_sounds->EmitSound(&es);

		std::string actionNameStr;
		if (elem.safe_at("inputActionName", actionNameStr))
		{
			EnterSelection(actionName);
		}
		else
		{
			PreEnterSelection();

			m_menuMode = MENUMODE_ENTER;
			m_textEffect = 1.0f;
			m_textFade = 1.0f;
		}
	}
}

void CState_MainMenu::Event_SelectionUp()
{
	if(m_goesFromMenu || m_menuMode > 0)
		return;

redecrement:
	m_selection--;

	if(m_selection < 0)
	{
		int nItem = 0;
		m_selection = m_numElems-1;
	}

	bool _spacer;
	OOLUA::Table elem;
	if (GetCurrentMenuElement(elem) && elem.safe_at("_spacer", _spacer))
		goto redecrement;

	EmitSound_t ep("menu.roll");
	g_sounds->EmitSound(&ep);
}

void CState_MainMenu::Event_SelectionDown()
{
	if(m_goesFromMenu || m_menuMode > 0)
		return;

reincrement:
	m_selection++;

	if(m_selection > m_numElems-1)
		m_selection = 0;

	bool _spacer;
	OOLUA::Table elem;
	if (GetCurrentMenuElement(elem) && elem.safe_at("_spacer", _spacer))
		goto reincrement;

	EmitSound_t ep("menu.roll");
	g_sounds->EmitSound(&ep);
}

void CState_MainMenu::Event_SelectMenuItem(int index)
{
	if(m_goesFromMenu || m_menuMode > 0)
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
