//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Provides base console interface
//////////////////////////////////////////////////////////////////////////////////

#include "sys_in_console.h"

#include "FontLayoutBuilders.h"

#include "IConCommandFactory.h"
#include "EngineVersion.h"
#include "FontCache.h"

#include "IDebugOverlay.h"

#include "materialsystem/MeshBuilder.h"
#include "KeyBinding/InputCommandBinder.h"

#include "utils/strtools.h"

#ifdef _DEBUG
#define CONSOLE_ENGINEVERSION_STR varargs(ENGINE_NAME " Engine " ENGINE_VERSION " DEBUG build %d (" COMPILE_DATE ")", BUILD_NUMBER_ENGINE)
#else
#define CONSOLE_ENGINEVERSION_STR varargs(ENGINE_NAME " Engine " ENGINE_VERSION " build %d (" COMPILE_DATE ")", BUILD_NUMBER_ENGINE)
#endif // _DEBUG

#define CONSOLE_INPUT_STARTSTR ("> ")
#define CURSOR_BLINK_TIME (0.2f)
const float CMDLIST_SYMBOL_SIZE = 15.0f;

// dummy command
DECLARE_CMD(toggleconsole, NULL, CV_INVISIBLE)	// dummy console command
{
}

// shows console
DECLARE_CMD(con_show, "Show console", 0)
{
	g_consoleInput->SetVisible(true);
	g_consoleInput->SetLogVisible(false);
}

// shows console with log
DECLARE_CMD(con_show_full, "Show console", 0)
{
	g_consoleInput->SetVisible(true);
	g_consoleInput->SetLogVisible(true);
}

// hides console
DECLARE_CMD(con_hide, "Hides console", 0)
{
	g_consoleInput->SetVisible(false);
	g_consoleInput->SetLogVisible(false);
}

// spew function for console
DECLARE_CMD(clear,NULL,0)
{
	CEqConsoleInput::SpewClear();
}

static int CON_SUGGESTIONS_MAX	= 40;
static float CON_MINICON_TIME	= 5.0f;

const float LOG_SCROLL_DELAY_START	= 0.15f;
const float LOG_SCROLL_DELAY_END	= 0.0f;

const float LOG_SCROLL_DELAY_STEP	= 0.01f;
const float LOG_SCROLL_POWER_INC	= 0.05f;

ConVar con_suggest("con_suggest","1",NULL,CV_ARCHIVE);

static ConVar con_minicon("con_minicon", "0", NULL, CV_ARCHIVE);

static CEqConsoleInput s_SysConsole;
CEqConsoleInput* g_consoleInput = &s_SysConsole;

static ColorRGBA s_conBackColor = ColorRGBA(0.15f, 0.25f, 0.25f, 0.85f);
static ColorRGBA s_conInputBackColor = ColorRGBA(0.15f, 0.25f, 0.25f, 0.85f);
static ColorRGBA s_conBorderColor = ColorRGBA(0.05f, 0.05f, 0.1f, 1.0f);

static ColorRGBA s_conBackFastFind = ColorRGBA(s_conBackColor.xyz(), 0.8f);

static ColorRGBA s_conTextColor = ColorRGBA(0.7f,0.7f,0.8f,1);
static ColorRGBA s_conSelectedTextColor = ColorRGBA(0.2f,0.2f,0.2f,1);
static ColorRGBA s_conInputTextColor = ColorRGBA(0.7f,0.7f,0.6f,1);
static ColorRGBA s_conHelpTextColor = ColorRGBA(0.7f,0.7f,0.8f,1);

static ColorRGBA s_spewColors[] =
{
	Vector4D(1,1,1,1),				// SPEW_NORM
	Vector4D(0.8f,0.8f,0.8f,1),		// SPEW_INFO
	Vector4D(1,1,0,1),				// SPEW_WARNING
	Vector4D(0.8f,0,0,1),			// SPEW_ERROR
	Vector4D(0.2f,1,0.2f,1)			// SPEW_SUCCESS
};

DECLARE_CMD(con_addAutoCompletion,"Adds autocompletion variants", CV_INVISIBLE)
{
	if(CMD_ARGC == 0)
	{
		Msg("Usage: con_addAutoCompletion <console command/variable> [argument1,argument2,...]\n");
		return;
	}

	ConAutoCompletion_t* newItem = new ConAutoCompletion_t;
	newItem->cmd_name = CMD_ARGV(0);

	xstrsplit(CMD_ARGV(1).c_str(),",",newItem->args);

	g_consoleInput->AddAutoCompletion(newItem);
}

DECLARE_CMD(help,"Display the help", 0)
{
	MsgInfo("Type \"cvarlist\" to show available console variables\nType \"cmdlist\" to show aviable console commands\nAlso you can press 'Tab' key to find commands by your typing\n");
	Msg("Hold Shift and use arrows for selection\n");
	Msg("Ctrl + C - Copy selection to clipboard\n");
	Msg("Ctrl + X - Cut selection to clipboard\n");
	Msg("Ctrl + V - Paste\n");
	Msg("Ctrl + A - Select All\n");
	Msg(" \nUse 'PageUp', 'PageDown', 'Home' and 'End' keys to navigate in console\n");
}

bool IsInRectangle(int posX, int posY,int rectX,int rectY,int rectW,int rectH)
{
	return ((posX >= rectX) && (posX <= rectX + rectW) && (posY >= rectY) && (posY <= rectY + rectH));
}

struct conSpewText_t
{
	~conSpewText_t()
	{
	}

	SpewType_t	type;
	EqString	text;
};

static DkList<conSpewText_t*> s_spewMessages;

void CEqConsoleInput::SpewFunc(SpewType_t type, const char* pMsg)
{
	// print out to std console
	printf("%s", pMsg );

#ifdef _WIN32
	// debug print only for Windows
	OutputDebugString(pMsg);
#endif // _WIN32

	char* pc = (char*)pMsg;
	char* lineStart = pc;

	conSpewText_t* currentSpewLine = NULL;

	for(;;pc++)
	{
		if(!(*pc == '\n' || *pc == '\0'))
			continue;

		int length = pc-lineStart;

		if(length > 0 || *pc == '\n')	// print non empty text and newlines
		{
			currentSpewLine = new conSpewText_t;
			currentSpewLine->type = type;
			currentSpewLine->text.Assign(lineStart, length);

			// output to debugoverlay if enabled
			if(con_minicon.GetBool() && debugoverlay != NULL)
				debugoverlay->TextFadeOut(0, s_spewColors[type], CON_MINICON_TIME, currentSpewLine->text.c_str());

			s_spewMessages.append(currentSpewLine);
		}

		// if not a zero, there is line start
		if(*pc != '\0')
			lineStart = pc+1;
		else
			break;
	}
}

void CEqConsoleInput::SpewClear()
{
	for(int i = 0; i < s_spewMessages.numElem(); i++)
		delete s_spewMessages[i];

	s_spewMessages.clear();
}

void CEqConsoleInput::SpewInit()
{
	SetSpewFunction(CEqConsoleInput::SpewFunc);
}

void CEqConsoleInput::SpewUninstall()
{
	SpewClear();
	SetSpewFunction(NULL);
}

//-------------------------------------------------------------------------

CEqConsoleInput::CEqConsoleInput()
{
	// release build needs false
	m_visible = false;

	m_logVisible = false;

	m_cursorTime = 0.0f;
	m_maxLines = 10;

	m_font = NULL;

	m_shiftModifier = false;
	m_ctrlModifier = false;

	m_width = 512;
	m_height = 512;
	m_enabled = false;

	m_cursorPos = 0;
	m_startCursorPos = 0;

	m_logScrollPosition = 0;
	m_logScrollDelay = LOG_SCROLL_DELAY_START;
	m_logScrollDir = 0;
	m_logScrollPower = 1.0f;

	m_histIndex = 0;
	m_fastfind_cmdbase = NULL;
	m_variantSelection = -1;

	m_alternateHandler = NULL;
}

void CEqConsoleInput::Initialize()
{
	kvkeybase_t* consoleSettings = GetCore()->GetConfig()->FindKeyBase("Console");

	const char* consoleFontName = KV_GetValueString(consoleSettings ? consoleSettings->FindKeyBase("Font") : NULL, 0, "console");

	m_font = g_fontCache->GetFont(consoleFontName, 16);
	m_enabled = KV_GetValueBool(consoleSettings ? consoleSettings->FindKeyBase("Enable") : NULL);
}

void CEqConsoleInput::AddAutoCompletion(ConAutoCompletion_t* newItem)
{
	for(int i = 0; i < m_customAutocompletion.numElem();i++)
	{
		if(!m_customAutocompletion[i]->cmd_name.CompareCaseIns(newItem->cmd_name.c_str()))
		{
			delete newItem;
			return;
		}
	}

	m_customAutocompletion.append(newItem);
}

void CEqConsoleInput::consoleRemTextInRange(int start,int len)
{
	if(uint(start+len) > m_inputText.Length())
		return;

	m_inputText.Remove(start,len);
	OnTextUpdate();
}

void CEqConsoleInput::consoleInsText(char* text,int pos)
{
	m_inputText.Insert(text, pos);
	OnTextUpdate();
}

void DrawAlphaFilledRectangle(const Rectangle_t &rect, const ColorRGBA &color1, const ColorRGBA &color2)
{
	BlendStateParam_t blending;
	blending.srcFactor = BLENDFACTOR_SRC_ALPHA;
	blending.dstFactor = BLENDFACTOR_ONE_MINUS_SRC_ALPHA;

	g_pShaderAPI->SetTexture(NULL,0,0);
	materials->SetBlendingStates(blending);
	materials->SetRasterizerStates(CULL_FRONT, FILL_SOLID);
	materials->SetDepthStates(false,false);

	materials->BindMaterial(materials->GetDefaultMaterial());

	Vector2D r0[] = { MAKEQUAD(rect.vleftTop.x, rect.vleftTop.y,rect.vleftTop.x, rect.vrightBottom.y, -1) };
	Vector2D r1[] = { MAKEQUAD(rect.vrightBottom.x, rect.vleftTop.y,rect.vrightBottom.x, rect.vrightBottom.y, -1) };
	Vector2D r2[] = { MAKEQUAD(rect.vleftTop.x, rect.vrightBottom.y,rect.vrightBottom.x, rect.vrightBottom.y, -1) };
	Vector2D r3[] = { MAKEQUAD(rect.vleftTop.x, rect.vleftTop.y,rect.vrightBottom.x, rect.vleftTop.y, -1) };

	// draw all rectangles with just single draw call
	CMeshBuilder meshBuilder(materials->GetDynamicMesh());
	meshBuilder.Begin(PRIM_TRIANGLE_STRIP);
		// put main rectangle
		meshBuilder.Color4fv(color1);
		meshBuilder.Quad2(rect.GetLeftBottom(), rect.GetRightBottom(), rect.GetLeftTop(), rect.GetRightTop());

		// put borders
		meshBuilder.Color4fv(color2);
		meshBuilder.Quad2(r0[0], r0[1], r0[2], r0[3]);
		meshBuilder.Quad2(r1[0], r1[1], r1[2], r1[3]);
		meshBuilder.Quad2(r2[0], r2[1], r2[2], r2[3]);
		meshBuilder.Quad2(r3[0], r3[1], r3[2], r3[3]);
	meshBuilder.End();
}

void CEqConsoleInput::DrawFastFind(float x, float y, float w)
{
	BlendStateParam_t blending;
	blending.srcFactor = BLENDFACTOR_SRC_ALPHA;
	blending.dstFactor = BLENDFACTOR_ONE_MINUS_SRC_ALPHA;

	eqFontStyleParam_t helpTextParams;
	helpTextParams.textColor = s_conHelpTextColor;
	helpTextParams.styleFlag = TEXT_STYLE_FROM_CAP;

	if(m_histIndex != -1 && m_commandHistory.numElem())
	{
		int max_string_length = 35;

		for(int i = 0; i < m_commandHistory.numElem(); i++)
			max_string_length = max((uint)max_string_length, m_commandHistory[i].Length());

		// draw as autocompletion
		int linesToDraw = m_commandHistory.numElem()+1;
		Rectangle_t rect(x,y,w,y + linesToDraw*m_font->GetLineHeight(helpTextParams) + 2);
		DrawAlphaFilledRectangle(rect, s_conBackColor, s_conBorderColor);

		m_font->RenderText("Last executed command: (cycle: Up/Down, press Enter to repeat)", rect.GetLeftTop() + Vector2D(5,4), helpTextParams);

		int startDraw = m_commandHistory.numElem()-CON_SUGGESTIONS_MAX;
		startDraw = max(0,startDraw);

		//for(int order = 0, i = m_commandHistory.numElem()-1; i >= startDraw;i--,order++)
		for(int i = startDraw; i < m_commandHistory.numElem(); i++)
		{
			eqFontStyleParam_t historyTextParams;
			historyTextParams.textColor = Vector4D(0.7f,0.7f,0,1);
			historyTextParams.styleFlag = TEXT_STYLE_FROM_CAP;

			int textYPos = m_font->GetLineHeight(helpTextParams) * (i + 1);

			if(m_histIndex == i)
			{
				g_pShaderAPI->Reset(STATE_RESET_TEX);
				Vertex2D_t selrect[] = { MAKETEXQUAD(x, rect.GetLeftTop().y+textYPos, w, rect.GetLeftTop().y+textYPos + 15 , 0) };

				materials->DrawPrimitives2DFFP(PRIM_TRIANGLE_STRIP,selrect,elementsOf(selrect), NULL, ColorRGBA(1.0f, 1.0f, 1.0f, 0.8f), &blending);
				g_pShaderAPI->Apply();

				historyTextParams.textColor = Vector4D(0.1f,0.1f,0,1);
			}

			m_font->RenderText(m_commandHistory[i].c_str(), rect.GetLeftTop() + Vector2D(5,4+textYPos), historyTextParams);
		}

		return;
	}

	if(con_suggest.GetBool())
	{
		int commandinfo_size = 0;

		// show command info
		if(m_fastfind_cmdbase)
		{
			int numLines = 1;
			const char* desc_str = m_fastfind_cmdbase->GetDesc();
			char c = desc_str[0];

			do
			{
				if(c == '\0')
					break;

				if(c == '\n')
					numLines++;
			}while(c = *desc_str++);

			EqString string_to_draw(_Es(m_fastfind_cmdbase->GetName()) + _Es(" - ") + m_fastfind_cmdbase->GetDesc());

			if(m_fastfind_cmdbase->IsConVar())
			{
				ConVar* pVar = (ConVar*)m_fastfind_cmdbase;
				if(pVar->HasClamp())
				{
					string_to_draw.Append(varargs(" \n\nValue in range [%g..%g]\n", pVar->GetMinClamp(), pVar->GetMaxClamp()));

					numLines += 2;
				}
			}

			// draw as autocompletion
			Rectangle_t rect(x,y,w,y + numLines * m_font->GetLineHeight(helpTextParams) + 2);
			DrawAlphaFilledRectangle(rect, s_conBackFastFind, s_conBorderColor);

			m_font->RenderText(string_to_draw.GetData(), rect.GetLeftTop() + Vector2D(5,4), helpTextParams);

			// draw autocompletion if available
			commandinfo_size += DrawAutoCompletion(x, rect.vrightBottom.y, w);
			commandinfo_size += numLines+1;
		}

		if(m_foundCmdList.numElem() >= CON_SUGGESTIONS_MAX)
		{
			Rectangle_t rect(x,y,w,y + m_font->GetLineHeight(helpTextParams) + 2 );
			DrawAlphaFilledRectangle(rect, s_conBackFastFind, s_conBorderColor);

			// Set color
			char* str = varargs("%d commands found (too many to show here)", m_foundCmdList.numElem());
			m_font->RenderText(str, rect.GetLeftTop() + Vector2D(5,4), helpTextParams);
		}
		else if(m_foundCmdList.numElem() > 0)
		{
			eqFontStyleParam_t variantsTextParams;
			variantsTextParams.styleFlag = TEXT_STYLE_FROM_CAP;

			int max_string_length = 35;

			for(int i = 0; i < m_foundCmdList.numElem(); i++)
				max_string_length = max((uint)max_string_length, strlen(m_foundCmdList[i]->GetName()));

			int numElemsToDraw = m_foundCmdList.numElem();

			float lineHeight = m_font->GetLineHeight(variantsTextParams);

			Rectangle_t rect(x,y,x+max_string_length*CMDLIST_SYMBOL_SIZE,y+numElemsToDraw*m_font->GetLineHeight(helpTextParams)+2);
			DrawAlphaFilledRectangle(rect, s_conBackFastFind, s_conBorderColor);

			for(int i = 0; i < m_foundCmdList.numElem();i++)
			{
				ConCommandBase* cmdBase = m_foundCmdList[i];

				bool bHasAutocompletion = cmdBase->HasVariants();

				// find autocompletion
				if(!bHasAutocompletion)
				{
					for(int j = 0;j < m_customAutocompletion.numElem();j++)
					{
						if(!stricmp(cmdBase->GetName(), m_customAutocompletion[j]->cmd_name.GetData()))
						{
							bHasAutocompletion = true;
							break;
						}
					}
				}

				bool bSelected = false;

				int clen = strlen(cmdBase->GetName());

				float textYPos = (y + i * m_font->GetLineHeight(helpTextParams)) + 4;

				if(IsInRectangle(m_mousePosition.x,m_mousePosition.y,x,textYPos+2, rect.vrightBottom.x-rect.vleftTop.x,12) || m_cmdSelection == i)
				{
					g_pShaderAPI->Reset(STATE_RESET_TEX);
					Vertex2D_t selrect[] = { MAKETEXQUAD(x, textYPos, x+max_string_length*CMDLIST_SYMBOL_SIZE, textYPos + 15 , 0) };

					materials->DrawPrimitives2DFFP(PRIM_TRIANGLE_STRIP,selrect,elementsOf(selrect), NULL, ColorRGBA(1.0f, 1.0f, 1.0f, 0.8f), &blending);
					g_pShaderAPI->Apply();

					m_cmdSelection = i;

					bSelected = true;
				}

				ColorRGBA text_color = Vector4D(0.7f,0.7f,0,1);
				if(cmdBase->IsConCommand())
					text_color = Vector4D(0.7f,0.7f,0.8f,1);

				Vector4D selTextColor = bSelected ? Vector4D(0.1f,0.1f,0,1) : text_color;
				variantsTextParams.textColor = selTextColor;

				char* str = NULL;

				if(cmdBase->IsConVar())
				{
					ConVar *cv = (ConVar*)cmdBase;

					if(bHasAutocompletion)
						str = varargs("%s [%s] ->", cv->GetName(), cv->GetString());
					else
						str = varargs("%s [%s]", cv->GetName(), cv->GetString());
				}
				else
				{
					if(bHasAutocompletion)
						str = varargs("%s () ->",cmdBase->GetName());
					else
						str = varargs("%s ()",cmdBase->GetName());
				}

				m_font->RenderText( str, Vector2D(x+5, textYPos),variantsTextParams);

				const char* cstr = xstristr(cmdBase->GetName(), m_inputText.GetData());

				if(cstr)
				{
					int ofs = cstr - cmdBase->GetName();
					int len = m_inputText.Length();

					float lookupStrStart = m_font->GetStringWidth(cmdBase->GetName(), variantsTextParams, ofs);
					float lookupStrEnd = lookupStrStart + m_font->GetStringWidth(cmdBase->GetName()+ofs, variantsTextParams, len);

					Vertex2D_t rect[] = { MAKETEXQUAD(x+5 + lookupStrStart, textYPos-2, x+5 + lookupStrEnd, textYPos+12, 0) };

					// Cancel textures
					g_pShaderAPI->Reset();

					materials->DrawPrimitives2DFFP(PRIM_TRIANGLE_STRIP,rect,elementsOf(rect), NULL, ColorRGBA(1.0f, 1.0f, 1.0f, 0.3f), &blending);
				}
			}
		}
	}
}

void CEqConsoleInput::ResetLogScroll()
{
	m_logScrollDelay = LOG_SCROLL_DELAY_START;
	m_logScrollPower = 1.0f;
	m_logScrollNextTime = 0.0f;
	m_logScrollDir = 0;
}

void CEqConsoleInput::OnTextUpdate()
{
	m_histIndex = -1;
	
	EqString inputText;
	int currentStatementStart = GetCurrentInputText(inputText);
	
	// update command variants
	UpdateCommandAutocompletionList( inputText.c_str() );

	// update argument variants
	int spaceIdx = inputText.Find(" ");
	if(spaceIdx != -1)
	{
		EqString firstArguemnt( inputText.Left(spaceIdx) );
		m_fastfind_cmdbase = (ConCommandBase*)g_sysConsole->FindBase( firstArguemnt.c_str() );

		if(m_fastfind_cmdbase)
		{
			EqString nextArguemnt( inputText.Mid(spaceIdx+1, inputText.Length()) );
			UpdateVariantsList( nextArguemnt );
		}
		else
		{
			m_variantSelection = -1;
			m_variantList.clear(false);
		}
	}
	else
	{
		m_fastfind_cmdbase = NULL;
	}
}

int CEqConsoleInput::GetCurrentInputText(EqString& str)
{
	int currentStatementStart = 0;

	while(true)
	{
		int nextStatementIdx = m_inputText.Find(";", false, currentStatementStart);

		if(nextStatementIdx == -1)
			break;

		currentStatementStart = nextStatementIdx+1;
	};
	
	str = m_inputText.Mid(currentStatementStart, m_inputText.Length());

	return currentStatementStart;
}

bool CEqConsoleInput::AutoCompleteSelectVariant()
{
	EqString inputText;
	int currentStatementStart = GetCurrentInputText(inputText);

	if(m_fastfind_cmdbase && m_variantSelection != -1)
	{
		// remove from this position
		m_inputText.Remove(currentStatementStart, inputText.Length());
		m_inputText.Append( (m_fastfind_cmdbase->GetName() + _Es(" \"") + m_variantList[m_variantSelection] + _Es("\"")).c_str() );

		m_cursorPos = m_startCursorPos = m_inputText.Length();
		OnTextUpdate();
		return true;
	}
	else if(m_foundCmdList.numElem() && m_cmdSelection != -1)
	{
		// remove from this position
		m_inputText.Remove(currentStatementStart, inputText.Length());
		m_inputText.Append( (m_foundCmdList[m_cmdSelection]->GetName() + _Es(" ")).c_str() );

		m_cursorPos = m_startCursorPos = m_inputText.Length();
		OnTextUpdate();
		return true;
	}

	return false;
}

void CEqConsoleInput::AutoCompleteSuggestion()
{
	EqString inputText;
	int currentStatementStart = GetCurrentInputText(inputText);

	int char_index = 0;

	int max_match_chars = -1;
	EqString matching_str;

	if(m_fastfind_cmdbase != NULL)
	{
		if(m_variantList.numElem() == 1)
		{
			// remove from this position
			m_inputText.Remove(currentStatementStart, inputText.Length());
			m_inputText.Append( (m_fastfind_cmdbase->GetName() + _Es(" ") + m_variantList[0]).c_str() );

			m_cursorPos = m_startCursorPos = m_inputText.Length();
			OnTextUpdate();
			return;
		}

		EqString queryStr;

		int spaceIdx = inputText.Find(" ");
		if(spaceIdx != -1)
			queryStr = inputText.c_str() + spaceIdx + 1;

		for(int i = 0; i < m_variantList.numElem(); i++)
		{
			char_index = m_variantList[i].Find(queryStr.c_str(), true);

			if(char_index != -1)
			{
				if(max_match_chars == -1)
				{
					matching_str = m_variantList[i];
					max_match_chars = matching_str.Length();
				}
				else
				{
					int cmpLen = matching_str.GetMathingChars( m_variantList[i] );

					if(cmpLen < max_match_chars)
					{
						matching_str = matching_str.Left(cmpLen);
						max_match_chars = cmpLen;
					}
				}
			}
		}

		if (max_match_chars > queryStr.Length() && queryStr.CompareCaseIns(matching_str))
		{
			m_inputText.Remove(spaceIdx+1, inputText.Length());
			m_inputText.Append(matching_str.c_str());
			m_cursorPos = m_startCursorPos = m_inputText.Length();

			OnTextUpdate();
			m_variantSelection = -1;
		}

		// multiple variants are trying to match string beginning
		bool anyFound = false;

		for (int i = m_variantSelection + 1; i < m_variantList.numElem(); i++)
		{
			EqString& variant = m_variantList[i];

			char_index = variant.Find(queryStr.c_str());

			if (char_index == 0)
			{
				m_variantSelection = i;
				anyFound = true;
				break;
			}
		}

		if (!anyFound)
			m_variantSelection = -1;
	}
	else
	{
		// single variant just autocompletes
		if (m_foundCmdList.numElem() == 1)
		{
			// remove from this position
			m_inputText.Remove(currentStatementStart, inputText.Length());
			m_inputText.Append(varargs("%s ", m_foundCmdList[0]->GetName()));

			m_cursorPos = m_startCursorPos = m_inputText.Length();
			OnTextUpdate();
			return;
		}

		// extending input string
		for (int i = 0; i < m_foundCmdList.numElem(); i++)
		{
			ConCommandBase* cmdBase = m_foundCmdList[i];
			EqString conVarName(cmdBase->GetName());

			if (max_match_chars == -1)
			{
				matching_str = conVarName;
				max_match_chars = matching_str.Length();
			}
			else
			{
				int cmpLen = matching_str.GetMathingChars(conVarName);

				if (cmpLen < max_match_chars)
				{
					matching_str = matching_str.Left(cmpLen);
					max_match_chars = cmpLen;
				}
			}
		}

		// maximize text input
		if (max_match_chars > m_inputText.Length() && m_inputText.CompareCaseIns(matching_str))
		{
			m_inputText.Assign(matching_str.c_str());
			m_cursorPos = m_startCursorPos = m_inputText.Length();
			
			OnTextUpdate();
			m_cmdSelection = -1;
		}

		// multiple variants are trying to match string beginning
		bool anyFound = false;

		for (int i = m_cmdSelection + 1; i < m_foundCmdList.numElem(); i++)
		{
			ConCommandBase* cmdBase = m_foundCmdList[i];

			EqString conVarName(cmdBase->GetName());
			char_index = conVarName.Find(inputText.c_str());

			if (char_index == 0)
			{
				m_cmdSelection = i;
				anyFound = true;
				break;
			}
		}

		if (!anyFound)
			m_cmdSelection = -1;
	}
}

void CEqConsoleInput::ExecuteCurrentInput()
{
	MsgInfo("> %s\n",m_inputText.GetData());

	g_sysConsole->ResetCounter();
	g_sysConsole->SetCommandBuffer(m_inputText.GetData());

	bool execStatus = g_sysConsole->ExecuteCommandBuffer(-1, m_alternateHandler != NULL);

	DkList<EqString>& failedCmds = g_sysConsole->GetFailedCommands();

	bool hasFailed = failedCmds.numElem() > 0;

	if( (execStatus == false || hasFailed) && m_alternateHandler != NULL)
	{
		hasFailed = !(*m_alternateHandler)(m_inputText.c_str());
		execStatus = true;
	}

	if(!execStatus)
		MsgError("Failed to execute '%s'\n",m_inputText.GetData());

	if(hasFailed)
	{
		for(int i = 0; i < failedCmds.numElem(); i++)
		{
			MsgError( "Unknown command or variable: '%s'\n", failedCmds[i].c_str() );
		}
	}

	// Compare the last command with current and add history if needs
	if(m_commandHistory.numElem() > 0)
	{
		if(stricmp(m_commandHistory[m_commandHistory.numElem()-1].GetData(),m_inputText.GetData()))
			m_commandHistory.append(m_inputText);
	}
	else
		m_commandHistory.append(m_inputText);
}

void CEqConsoleInput::UpdateCommandAutocompletionList(const EqString& queryStr)
{
	m_foundCmdList.clear();
	m_cmdSelection = -1;

	const DkList<ConCommandBase*>* baseList = g_sysConsole->GetAllCommands();

	for(int i = 0; i < baseList->numElem();i++)
	{
		ConCommandBase* cmdBase = baseList->ptr()[i];

		EqString conVarName(cmdBase->GetName());

		// find by input text
		if(conVarName.Find(queryStr.c_str()) != -1)
		{
			if(cmdBase->GetFlags() & CV_INVISIBLE)
				continue;

			m_foundCmdList.append(cmdBase);
		}
	}
}

void CEqConsoleInput::UpdateVariantsList( const EqString& queryStr )
{
	if(!con_suggest.GetBool())
		return;

	m_variantSelection = -1;
	m_variantList.clear(false);

	DkList<EqString> allAutoCompletion;

	for(int i = 0; i < m_customAutocompletion.numElem(); i++)
	{
		ConAutoCompletion_t* item = m_customAutocompletion[i];

		if(stricmp(m_fastfind_cmdbase->GetName(), item->cmd_name.c_str()))
			continue;

		for(int j = 0; j < item->args.numElem(); j++)
			allAutoCompletion.append( item->args[j] );

		break;
	}

	if( m_fastfind_cmdbase->HasVariants() )
		m_fastfind_cmdbase->GetVariants(allAutoCompletion, queryStr.c_str());

	// filter out the list
	for(int i = 0; i < allAutoCompletion.numElem(); i++)
	{
		if(queryStr.Length() == 0 || allAutoCompletion[i].Find(queryStr.c_str()) != -1)
			m_variantList.append(allAutoCompletion[i]);
	}
}

int CEqConsoleInput::DrawAutoCompletion(float x, float y, float w)
{
	if(!(con_suggest.GetBool() && m_fastfind_cmdbase))
		return 0;

	int max_string_length = 35;

	for(int i = 0; i < m_variantList.numElem(); i++)
		max_string_length = max((uint)max_string_length, m_variantList[i].Length());

	int displayEnd = CON_SUGGESTIONS_MAX;
	int displayStart = 0;

	while(m_variantSelection-displayStart > CON_SUGGESTIONS_MAX-1)
	{
		displayStart += CON_SUGGESTIONS_MAX;
		displayEnd += CON_SUGGESTIONS_MAX;
	}

	displayEnd = min(displayEnd, m_variantList.numElem());

	if(displayEnd <= 0)
		return 0;

	eqFontStyleParam_t variantsTextParams;
	variantsTextParams.textColor = s_conInputTextColor;
	variantsTextParams.styleFlag = TEXT_STYLE_FROM_CAP;

	int linesToDraw = displayEnd-displayStart+1;

	// draw as autocompletion
	Rectangle_t rect(x,y,x+max_string_length*CMDLIST_SYMBOL_SIZE,y+linesToDraw*m_font->GetLineHeight(variantsTextParams));
	DrawAlphaFilledRectangle(rect, s_conBackFastFind, s_conBorderColor);

	m_font->RenderText("Possible variants: ", Vector2D(x+5,y+2), variantsTextParams);

	for(int line = 1, i = displayStart; i < displayEnd; i++)
	{
		float textYPos = (y + line*m_font->GetLineHeight(variantsTextParams));
		line++;

		bool bSelected = false;

		if(IsInRectangle(m_mousePosition.x,m_mousePosition.y,
						x,textYPos, rect.vrightBottom.x - rect.vleftTop.x,12) || m_variantSelection == i)
		{
			Vertex2D_t selrect[] = { MAKETEXQUAD(x, textYPos-4,x+max_string_length*CMDLIST_SYMBOL_SIZE, textYPos + 14 , 1) };

			// Cancel textures
			g_pShaderAPI->Reset(STATE_RESET_TEX);

			// Draw the rectangle
			materials->DrawPrimitives2DFFP(PRIM_TRIANGLE_STRIP,selrect,elementsOf(selrect));

			bSelected = true;

			m_variantSelection = i;
		}

		Vector4D selTextColor = bSelected ? s_conSelectedTextColor : s_conTextColor;

		variantsTextParams.textColor = selTextColor;
		m_font->RenderText(m_variantList[i].c_str(), Vector2D(x+5,textYPos), variantsTextParams);
	}

	return linesToDraw;
}

void CEqConsoleInput::SetLastLine()
{
	int totalLines = s_spewMessages.numElem();

	m_logScrollPosition = totalLines - m_maxLines;
	m_logScrollPosition = max(0,m_logScrollPosition);
}

void CEqConsoleInput::AddToLinePos(int num)
{
	int totalLines = s_spewMessages.numElem();
	m_logScrollPosition += num;
	m_logScrollPosition = min(totalLines,m_logScrollPosition);
}

void CEqConsoleInput::SetText( const char* text, bool quiet /*= false*/ )
{
	m_inputText = text;
	m_cursorPos = m_startCursorPos = m_inputText.Length();

	if(!quiet)
		OnTextUpdate();
}

void CEqConsoleInput::DrawSelf(int width,int height, float frameTime)
{
	m_cursorTime -= frameTime;

	if(m_cursorTime < 0.0f)
	{
		m_cursorVisible = !m_cursorVisible;
		m_cursorTime = CURSOR_BLINK_TIME;
	}

	if (!m_logVisible)
		ResetLogScroll();

	if(m_logScrollDir != 0)
	{
		int maxScroll = s_spewMessages.numElem()-1;

		m_logScrollNextTime -= frameTime;
		if(m_logScrollNextTime <= 0.0f)
		{
			m_logScrollNextTime = m_logScrollDelay;

			m_logScrollPosition += m_logScrollDir*floor(m_logScrollPower);

			m_logScrollPosition = min(m_logScrollPosition, maxScroll);
			m_logScrollPosition = max(m_logScrollPosition, 0);
		}
	}

	BlendStateParam_t blending;
	blending.srcFactor = BLENDFACTOR_SRC_ALPHA;
	blending.dstFactor = BLENDFACTOR_ONE_MINUS_SRC_ALPHA;

	eqFontStyleParam_t fontStyle;

	int drawstart = m_logScrollPosition;

	if(!m_visible)
		return;

	m_maxLines = floor(height / m_font->GetLineHeight(fontStyle)) - 2;

	if(m_maxLines < -4)
		return;

	int drawending = (-drawstart*m_font->GetLineHeight(fontStyle)) + m_font->GetLineHeight(fontStyle) * (m_maxLines+drawstart);

	int draws = 0;
	
	Rectangle_t inputTextEntryRect(64, 26, width-64,46);

	Rectangle_t con_outputRectangle(64.0f,inputTextEntryRect.vrightBottom.y+26, width - 64.0f, height-inputTextEntryRect.vleftTop.y);

	if(m_logVisible)
	{
		DrawAlphaFilledRectangle(con_outputRectangle, s_conBackColor, s_conBorderColor);

		int cnumLines = 0;

		int numRenderLines = s_spewMessages.numElem();

		m_maxLines = (con_outputRectangle.GetSize().y / m_font->GetLineHeight(fontStyle))-1;

		int numDrawn = 0;

		eqFontStyleParam_t outputTextStyle;

		eqFontStyleParam_t hasLinesStyle;
		hasLinesStyle.textColor = ColorRGBA(0.5f,0.5f,1.0f,1.0f);

		con_outputRectangle.vleftTop.x += 5.0f;
		con_outputRectangle.vleftTop.y += m_font->GetLineHeight(fontStyle);

		static CRectangleTextLayoutBuilder rectLayout;
		rectLayout.SetRectangle( con_outputRectangle );

		outputTextStyle.layoutBuilder = &rectLayout;

		for(int i = drawstart; i < numRenderLines; i++, numDrawn++)
		{
			outputTextStyle.textColor = s_spewColors[s_spewMessages[i]->type];

			m_font->RenderText(s_spewMessages[i]->text.c_str(), con_outputRectangle.vleftTop, outputTextStyle);

			con_outputRectangle.vleftTop.y += m_font->GetLineHeight(fontStyle)*rectLayout.GetProducedLines();//cnumLines;

			if(rectLayout.HasNotDrawnLines() || i-drawstart >= m_maxLines)
			{
				m_font->RenderText("^ ^ ^ ^ ^ ^", Vector2D(con_outputRectangle.vleftTop.x, con_outputRectangle.vrightBottom.y), hasLinesStyle);

				break;
			}
		}
	}

	DrawFastFind( 128, inputTextEntryRect.vleftTop.y+25, width-128 );

	EqString conInputStr(CONSOLE_INPUT_STARTSTR);
	conInputStr.Append(m_inputText);

	DrawAlphaFilledRectangle(inputTextEntryRect, s_conInputBackColor, s_conBorderColor);

	eqFontStyleParam_t inputTextStyle;
	inputTextStyle.textColor = s_conInputTextColor;

	Vector2D inputTextPos(inputTextEntryRect.vleftTop.x+4, inputTextEntryRect.vrightBottom.y-6);

	// render input text
	m_font->RenderText(conInputStr.c_str(), inputTextPos, inputTextStyle);

	float inputGfxOfs = m_font->GetStringWidth(CONSOLE_INPUT_STARTSTR, inputTextStyle);
	float cursorPosition = inputGfxOfs + m_font->GetStringWidth(m_inputText.c_str(), inputTextStyle, m_cursorPos);

	// render selection
	if(m_startCursorPos != m_cursorPos)
	{
		float selStartPosition = inputGfxOfs + m_font->GetStringWidth(m_inputText.c_str(), inputTextStyle, m_startCursorPos);

		Vertex2D_t rect[] = { MAKETEXQUAD(	inputTextPos.x + selStartPosition,
											inputTextPos.y - 10,
											inputTextPos.x + cursorPosition,
											inputTextPos.y + 4, 0) };
		// Cancel textures
		g_pShaderAPI->Reset();

		materials->DrawPrimitives2DFFP(PRIM_TRIANGLE_STRIP,rect,elementsOf(rect), NULL, ColorRGBA(1.0f, 1.0f, 1.0f, 0.3f), &blending);
	}

	// render cursor
	if(m_cursorVisible)
	{
		Vertex2D_t rect[] = { MAKETEXQUAD(	inputTextPos.x + cursorPosition,
											inputTextPos.y - 10,
											inputTextPos.x + cursorPosition + 1,
											inputTextPos.y + 4, 0) };

		// Cancel textures
		g_pShaderAPI->Reset();

		materials->DrawPrimitives2DFFP(PRIM_TRIANGLE_STRIP,rect,elementsOf(rect), NULL, ColorRGBA(1.0f), &blending);
	}

	eqFontStyleParam_t versionTextStl;
	versionTextStl.styleFlag = TEXT_STYLE_SHADOW | TEXT_STYLE_FROM_CAP;
	versionTextStl.textColor = ColorRGBA(1,1,1,0.5f);

	m_font->RenderText( CONSOLE_ENGINEVERSION_STR, Vector2D(5,5), versionTextStl);
}

void CEqConsoleInput::MousePos(const Vector2D &pos)
{
	m_mousePosition = pos;
}

bool CEqConsoleInput::KeyChar(int ch)
{
	if(!m_visible)
		return false;

	// Font is not loaded, skip
	if(m_font == NULL)
		return false;

	if(ch < 32 || ch == '~' || ch == '`')
		return false;

	if(m_font->GetFontCharById(ch).advX == 0.0f)
		return false;

	char text[2];
	text[0] = ch;
	text[1] = 0;

	m_inputText.Insert( text, m_cursorPos);

	m_cursorPos += 1;
	m_startCursorPos = m_cursorPos;

	OnTextUpdate();

	return true;
}

bool CEqConsoleInput::MouseEvent(const Vector2D &pos, int Button,bool pressed)
{
	if(!m_visible)
		return false;

	if (pressed)
	{
		if (Button == MOU_B1)
		{
			AutoCompleteSelectVariant();
		}
	}

	return true;
}

void CEqConsoleInput::SetLogVisible(bool bVisible)
{
	m_logVisible = bVisible;
	ResetLogScroll();
}

void CEqConsoleInput::SetVisible(bool bVisible)
{
	if(!m_enabled)
	{
		m_visible = false;
		return;
	}

	ResetLogScroll();

	m_visible = bVisible;

	if(!m_visible)
	{
		m_histIndex = -1;
	}
}

bool CEqConsoleInput::KeyPress(int key, bool pressed)
{
	if (pressed) // catch "DOWN" event
	{
		in_binding_t* tgBind = g_inputCommandBinder->FindBindingByCommand(&cmd_toggleconsole);

		if (tgBind && s_keyMapList[tgBind->key_index].keynum == key)
		{
			if (IsVisible() && IsShiftPressed())
			{
				SetLogVisible(!IsLogVisible());
				return false;
			}

			SetVisible(!IsVisible());
			return false;
		}
	}

	if(!m_visible)
		return false;

	if(pressed)
	{
		switch (key)
		{
			case KEY_BACKSPACE:
			case KEY_DELETE:
			{
				if(m_startCursorPos != m_cursorPos)
				{
					int selStart = min(m_startCursorPos, m_cursorPos);
					int selEnd = max(m_startCursorPos, m_cursorPos);

					consoleRemTextInRange(selStart, selEnd - selStart);

					m_cursorPos = selStart;
					m_startCursorPos = m_cursorPos;

					m_histIndex = -1;
					break;
				}

				if(key == KEY_BACKSPACE)
				{
					if(m_cursorPos > 0)
					{
						m_cursorPos--;
						m_startCursorPos = m_cursorPos;

						consoleRemTextInRange(m_cursorPos, 1);
						m_histIndex = -1;
					}
				}
				else
				{
					consoleRemTextInRange(m_cursorPos, 1);
					m_histIndex = -1;
				}

				break;
			}
			case KEY_SHIFT:
			{
				m_shiftModifier = true;

				break;
			}
			case KEY_CTRL:
			{
				m_ctrlModifier = true;

				break;
			}
			case KEY_LEFT:
			{
				m_cursorPos--;
				m_cursorPos = max(m_cursorPos, 0);

				if(!m_shiftModifier)	// drop secondary cursor position
					m_startCursorPos = m_cursorPos;

				break;
			}
			case KEY_RIGHT:
			{
				m_cursorPos++;
				m_cursorPos = min(m_cursorPos, m_inputText.Length());

				if(!m_shiftModifier)	// drop secondary cursor position
					m_startCursorPos = m_cursorPos;

				break;
			}
			case KEY_HOME:
				m_logScrollPosition = 0;
				break;
			case KEY_END:
				SetLastLine();
				break;
			case KEY_A:
				if(m_ctrlModifier)
				{
					m_cursorPos = m_inputText.Length();
					m_startCursorPos = 0;
				}
				break;
			case KEY_C:
				if(m_ctrlModifier)
				{
					if(m_startCursorPos != m_cursorPos)
					{
						int selStart = min(m_startCursorPos, m_cursorPos);
						int selEnd = max(m_startCursorPos, m_cursorPos);

						EqString tmpString(m_inputText.Mid(selStart, selEnd-selStart));

#ifdef PLAT_SDL
						// simple, yea
						SDL_SetClipboardText(tmpString.c_str());
#elif PLAT_WIN
						if(OpenClipboard( m_hwnd ))
						{
							HGLOBAL hgBuffer;

							char* chBuffer;
							EmptyClipboard();
							hgBuffer= GlobalAlloc(GMEM_DDESHARE, tmpString.Length()+1);
							chBuffer= (char*)GlobalLock(hgBuffer);
							strcpy(chBuffer, tmpString.GetData());
							GlobalUnlock(hgBuffer);
							SetClipboardData(CF_TEXT,hgBuffer);
							CloseClipboard();
						}
#endif // PLAT_SDL
					}
				}
				break;
			case KEY_X:
				if(m_ctrlModifier)
				{
					if(m_startCursorPos != m_cursorPos)
					{
						int selStart = min(m_startCursorPos, m_cursorPos);
						int selEnd = max(m_startCursorPos, m_cursorPos);

						EqString tmpString(m_inputText.Mid(selStart, selEnd-selStart));
#ifdef PLAT_SDL
						SDL_SetClipboardText(tmpString.c_str());
#elif PLAT_WIN
						if(OpenClipboard( m_hwnd ))
						{
							HGLOBAL hgBuffer;

							char* chBuffer;
							EmptyClipboard();
							hgBuffer= GlobalAlloc(GMEM_DDESHARE, tmpString.Length()+1);
							chBuffer= (char*)GlobalLock(hgBuffer);
							strcpy(chBuffer, tmpString.GetData());
							GlobalUnlock(hgBuffer);
							SetClipboardData(CF_TEXT,hgBuffer);
							CloseClipboard();
						}
#endif // PLAT_SDL

						consoleRemTextInRange(selStart, selEnd - selStart);

						m_cursorPos = selStart;
						m_startCursorPos = m_cursorPos;
					}
				}
				break;
			case KEY_V:
				if(m_ctrlModifier)
				{
#ifdef PLAT_SDL
					{
						EqString tmpString = SDL_GetClipboardText();
#elif PLAT_WIN
					if(OpenClipboard(m_hwnd))

					{
						HANDLE hData = GetClipboardData(CF_TEXT);
						EqString tmpString;
						char* chBuffer = (char*)GlobalLock(hData);
						if(chBuffer == NULL)
						{
							GlobalUnlock(hData);
							CloseClipboard();
							break;
						}

						tmpString = chBuffer;
						GlobalUnlock(hData);
						CloseClipboard();
#endif // PLAT_SDL

						int selStart = min(m_startCursorPos, m_cursorPos);

						if(m_startCursorPos != m_cursorPos)
						{
							int selEnd = max(m_startCursorPos, m_cursorPos);

							consoleRemTextInRange(selStart, selEnd - selStart);
						}

						consoleInsText((char*)tmpString.GetData(), selStart);
						m_cursorPos = selStart;

						m_cursorPos += tmpString.Length();
						m_startCursorPos = m_cursorPos;
					}
				}
				break;
			case KEY_ENTER:
				if (m_inputText.Length() > 0)
				{
					if(AutoCompleteSelectVariant())
						break;

					ExecuteCurrentInput();
				}
				else
				{
					if(m_histIndex != -1 && m_commandHistory.numElem())
					{
						SetText(m_commandHistory[m_histIndex].c_str());
						m_histIndex = -1;
						break;
					}

					Msg("]\n");
				}

				SetText("");

				break;

			case KEY_TAB:
				{
					AutoCompleteSuggestion();
				}
				break;

			case KEY_PGUP:
				m_logScrollDir = -1;

				m_logScrollDelay -= LOG_SCROLL_DELAY_STEP;
				m_logScrollPower += LOG_SCROLL_POWER_INC;
				m_logScrollDelay = max(m_logScrollDelay, LOG_SCROLL_DELAY_END);

				break;
			case KEY_PGDN:
				m_logScrollDir = 1;

				m_logScrollDelay -= LOG_SCROLL_DELAY_STEP;
				m_logScrollPower += LOG_SCROLL_POWER_INC;
				m_logScrollDelay = max(m_logScrollDelay, LOG_SCROLL_DELAY_END);

				break;

			case KEY_DOWN: // FIXME: invalid indices

				if(m_fastfind_cmdbase && m_variantList.numElem())
				{
					m_variantSelection++;
					m_variantSelection = min(m_variantSelection, m_variantList.numElem()-1);
					break;
				}
				else
				{
					if(m_foundCmdList.numElem())
					{
						m_cmdSelection++;
						m_cmdSelection = min(m_cmdSelection, m_foundCmdList.numElem()-1);
						break;
					}
				}

				if(!m_commandHistory.numElem())
					break;

				m_histIndex++;
				m_histIndex = min(m_histIndex, m_commandHistory.numElem()-1);

				if(m_commandHistory.numElem() > 0)
					SetText(m_commandHistory[m_histIndex].c_str(), true);

				break;
			case KEY_UP:

				if(m_fastfind_cmdbase && m_variantList.numElem())
				{
					m_variantSelection--;
					m_variantSelection = max(m_variantSelection, 0);
					break;
				}
				else
				{
					if(m_foundCmdList.numElem())
					{
						m_cmdSelection--;
						m_cmdSelection = max(m_cmdSelection, 0);
						break;
					}
				}

				if(!m_commandHistory.numElem())
					break;

				if(m_histIndex == -1)
					m_histIndex = m_commandHistory.numElem();

				m_histIndex--;
				m_histIndex = max(m_histIndex, 0);

				if(m_commandHistory.numElem() > 0)
					SetText(m_commandHistory[m_histIndex].c_str(), true);

				break;

			case KEY_ESCAPE:
				if(m_histIndex != -1)
					SetText("", true);

			default:

				if(m_startCursorPos != m_cursorPos)
				{
					int selStart = min(m_startCursorPos, m_cursorPos);
					int selEnd = max(m_startCursorPos, m_cursorPos);

					m_inputText.Remove(selStart, selEnd - selStart);

					m_cursorPos = selStart;
					m_startCursorPos = m_cursorPos;
				}

				m_histIndex = -1; // others and escape
				m_startCursorPos = m_cursorPos;
				break;
		}
	}
	else
	{
		switch (key)
		{
			case KEY_SHIFT:
				m_shiftModifier = false;
				break;
			case KEY_CTRL:
				m_ctrlModifier = false;
				break;
			case KEY_PGUP:
			case KEY_PGDN:
				ResetLogScroll();
				break;
		}
	}

	return true;
}