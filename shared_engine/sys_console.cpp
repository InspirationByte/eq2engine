//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Provides base console interface
//////////////////////////////////////////////////////////////////////////////////

#include "sys_console.h"

#include <stdio.h>

#include "FontLayoutBuilders.h"

#include "EngineSpew.h"
#include "EngineVersion.h"
#include "FontCache.h"
#include "KeyBinding/Keys.h"

#ifdef _DEBUG
#define CONSOLE_ENGINEVERSION_STR varargs(ENGINE_NAME " Engine " ENGINE_VERSION " DEBUG build %d (" COMPILE_DATE ")", BUILD_NUMBER_ENGINE)
#else
#define CONSOLE_ENGINEVERSION_STR varargs(ENGINE_NAME " Engine " ENGINE_VERSION " build %d (" COMPILE_DATE ")", BUILD_NUMBER_ENGINE)
#endif // _DEBUG

#define CONSOLE_INPUT_STARTSTR ("> ")

#define CURSOR_BLINK_TIME (0.2f)

// dummy command
DECLARE_CMD(toggleconsole, "Toggles console", 0)
{
}

// shows console
DECLARE_CMD(con_show, "Show console", 0)
{
	g_pSysConsole->SetVisible(true);
}

ConVar con_enable("con_enable","1","Enable console", CV_CHEAT);
ConVar con_fastfind("con_fastfind","1","Show FastFind popup in console",CV_ARCHIVE);
ConVar con_fastfind_count("con_fastfind_count","35","FastFind listed vars count",CV_ARCHIVE);
ConVar con_autocompletion_enable("con_autocompletion_enable","1","Enable autocompletion for console\n See file <game dir>/common/autocompletion.cfg",CV_ARCHIVE);

static CEqSysConsole s_SysConsole;
CEqSysConsole* g_pSysConsole = &s_SysConsole;

#define FONT_WIDE 10 //9
#define FONT_TALL 18 //14

#define FONT_DIMS FONT_WIDE, FONT_TALL

static ColorRGBA s_conBackColor = ColorRGBA(0.15f, 0.25f, 0.25f, 0.85f);
static ColorRGBA s_conInputBackColor = ColorRGBA(0.15f, 0.25f, 0.25f, 0.85f);
static ColorRGBA s_conBorderColor = ColorRGBA(0.05f, 0.05f, 0.1f, 1.0f);

static ColorRGBA s_conBackFastFind = ColorRGBA(s_conBackColor.xyz(), 0.8f);

static ColorRGBA s_conTextColor = ColorRGBA(0.7f,0.7f,0.8f,1);
static ColorRGBA s_conSelectedTextColor = ColorRGBA(0.2f,0.2f,0.2f,1);
static ColorRGBA s_conInputTextColor = ColorRGBA(0.7f,0.7f,0.6f,1);
static ColorRGBA s_conHelpTextColor = ColorRGBA(0.7f,0.7f,0.8f,1);

void AutoCompletionCheckExtensionDir(const char* extension, const char *dir, bool stripExtensions, DkList<EqString>* strings)
{
	EqString dirname = dir + _Es(extension);

#ifdef _WIN32

	WIN32_FIND_DATA wfd;
	HANDLE hFile;

	hFile = FindFirstFile(dirname.GetData(), &wfd);
	if(hFile != NULL)
	{
		while(1)
		{
			if(!FindNextFile(hFile, &wfd))
				break;

			EqString filename = wfd.cFileName;
			if(filename.GetLength() > 1 && filename != ".." && filename != "." && !(wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
			{
				if(stripExtensions)
					strings->append(filename.Path_Strip_Ext());
				else
					strings->append(filename);
			}
		}

		FindClose(hFile);
	}
#endif // _WIN32
}

void CC_AutoCompletinon_AddCommandBase(DkList<EqString> *args)
{
	if(!args)
	{
		Msg("Usage: autocompletion_addcommandbase <console command/variable> <arguments>\n use $cvarlist$ or $cmdlist$ as arguments");
		return;
	}

	if(args->numElem() > 1)
	{
		AutoCompletionNode_s* pNode = new AutoCompletionNode_s;
		pNode->cmd_name = args->ptr()[0];

		bool isGeneric = true;

		if(!stricmp(args->ptr()[1].GetData(),ARG_TYPE_CMDLIST_STRING))
		{
			pNode->argumentType = ARG_TYPE_CMDLIST;
			isGeneric = false;
		}

		if(!stricmp(args->ptr()[1].GetData(), ARG_TYPE_CVARLIST_STRING))
		{
			pNode->argumentType = ARG_TYPE_CVARLIST;
			isGeneric = false;
		}

#ifdef _WIN32

		if(!stricmp(args->ptr()[1].GetData(), ARG_TYPE_MAPLIST_STRING))
		{
			pNode->argumentType = ARG_TYPE_MAPLIST;
			isGeneric = false;

			EqString dirname = g_fileSystem->GetCurrentGameDirectory() + _Es("/maps/");
			//AutoCompletionCheckExtensionDir("*.eqbsp",dirname.getData(),true,&pNode->args);

			EqString level_dir(g_fileSystem->GetCurrentGameDirectory());
			level_dir = level_dir + _Es("/Worlds/*.*");

			WIN32_FIND_DATA wfd;
			HANDLE hFile;

			hFile = FindFirstFile(level_dir.GetData(), &wfd);
			if(hFile != NULL)
			{
				while(1)
				{
					if(!FindNextFile(hFile, &wfd))
						break;

					EqString filename = wfd.cFileName;

					if((wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && filename != ".." && filename != ".")
					{
						// check level compilation
						if(!g_fileSystem->FileExist(("worlds/" + filename + "/world.build").GetData()))
							continue;

						pNode->args.append(filename);

					}
				}

				FindClose(hFile);
			}
		}
#endif // _WIN32

		if(!stricmp(args->ptr()[1].GetData(), ARG_TYPE_CFGLIST_STRING))
		{
			pNode->argumentType = ARG_TYPE_CFGLIST;
			isGeneric = false;

			EqString dirname = g_fileSystem->GetCurrentGameDirectory() + _Es("/cfg/");
			AutoCompletionCheckExtensionDir("*.cfg",dirname.GetData(), false, &pNode->args);

			dirname = g_fileSystem->GetCurrentDataDirectory() + _Es("/cfg/");
			AutoCompletionCheckExtensionDir("*.cfg",dirname.GetData(), false, &pNode->args);

			dirname = "cfg/";
			AutoCompletionCheckExtensionDir("*.cfg",dirname.GetData(), false, &pNode->args);
		}

		if(isGeneric)
		{
			xstrsplit(args->ptr()[1].GetData(),",",pNode->args);
		}

		g_pSysConsole->AddAutoCompletionNode(pNode);
	}
}
ConCommand cc_autocompletion_addbase("autocompletion_addcommandbase",CC_AutoCompletinon_AddCommandBase,"Add autocompletion descriptor to cvar/command",CV_INVISIBLE);

void CC_Help_F(DkList<EqString> *args)
{
	MsgInfo("Type \"cvarlist\" to show available console variables\nType \"cmdlist\" to show aviable console commands\nAlso you can press 'Tab' key to find commands by your typing\n");
	Msg("Hold Shift and use arrows for selection\n");
	Msg("Ctrl + C - Copy selection to clipboard\n");
	Msg("Ctrl + X - Cut selection to clipboard\n");
	Msg("Ctrl + V - Paste\n");
	Msg("Ctrl + A - Select All\n");
	Msg(" \nUse 'PageUp', 'PageDown', 'Home' and 'End' keys to navigate in console\n");
}
ConCommand cc_help("help",CC_Help_F,"Display the help");

bool IsInRectangle(int posX, int posY,int rectX,int rectY,int rectW,int rectH)
{
	return ((posX >= rectX) && (posX <= rectX + rectW) && (posY >= rectY) && (posY <= rectY + rectH));
}

CEqSysConsole::CEqSysConsole()
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
	fullscreen = false;

	m_cursorPos = 0;
	m_startCursorPos = -1;
	m_logScrollPosition = 0;
	con_histIndex = 0;
	con_valueindex = 0;
	con_fastfind_selection_index = -1;
	con_fastfind_selection_autocompletion_index = -1;
	con_fastfind_selection_autocompletion_val_index = -1;
	con_fastfind_isbeingselected = false;
	con_fastfind_isbeingselected_autoc = false;
}

void CEqSysConsole::Initialize()
{
	m_font = g_fontCache->GetFont("console", 16);
}

void CEqSysConsole::AddAutoCompletionNode(AutoCompletionNode_s *pNode)
{
	for(int i = 0; i < m_hAutoCompletionNodes.numElem();i++)
	{
		if(!m_hAutoCompletionNodes[i]->cmd_name.CompareCaseIns(pNode->cmd_name.GetData()))
		{
			delete pNode;
			return;
		}
	}

	m_hAutoCompletionNodes.append(pNode);
}

void CEqSysConsole::consoleRemTextInRange(int start,int len)
{
	if(uint(start+len) > con_Text.GetLength())
		return;

	con_Text.Remove(start,len);
}

void CEqSysConsole::consoleInsText(char* text,int pos)
{
	con_Text.Insert(text, pos);
}

void DrawAlphaFilledRectangle(Rectangle_t &rect, ColorRGBA &color1, ColorRGBA &color2)
{
	Vertex2D_t tmprect[] = { MAKETEXQUAD(rect.vleftTop.x, rect.vleftTop.y,rect.vrightBottom.x, rect.vrightBottom.y, 0) };

	// Cancel textures
	g_pShaderAPI->Reset(STATE_RESET_TEX);

	BlendStateParam_t blending;
	blending.srcFactor = BLENDFACTOR_SRC_ALPHA;
	blending.dstFactor = BLENDFACTOR_ONE_MINUS_SRC_ALPHA;

	materials->DrawPrimitives2DFFP(PRIM_TRIANGLE_STRIP,tmprect,elementsOf(tmprect), NULL, color1, &blending);

	// Set color

	// Draw 4 solid rectangles
	Vertex2D_t r0[] = { MAKETEXQUAD(rect.vleftTop.x, rect.vleftTop.y,rect.vleftTop.x, rect.vrightBottom.y, -1) };
	Vertex2D_t r1[] = { MAKETEXQUAD(rect.vrightBottom.x, rect.vleftTop.y,rect.vrightBottom.x, rect.vrightBottom.y, -1) };
	Vertex2D_t r2[] = { MAKETEXQUAD(rect.vleftTop.x, rect.vrightBottom.y,rect.vrightBottom.x, rect.vrightBottom.y, -1) };
	Vertex2D_t r3[] = { MAKETEXQUAD(rect.vleftTop.x, rect.vleftTop.y,rect.vrightBottom.x, rect.vleftTop.y, -1) };

	// Set alpha,rasterizer and depth parameters
	//g_pShaderAPI->SetBlendingStateFromParams(NULL);
	//g_pShaderAPI->ChangeRasterStateEx(CULL_FRONT,FILL_SOLID);



	materials->DrawPrimitives2DFFP(PRIM_TRIANGLE_STRIP,r0,elementsOf(r0), NULL, color2, &blending);
	materials->DrawPrimitives2DFFP(PRIM_TRIANGLE_STRIP,r1,elementsOf(r1), NULL, color2, &blending);
	materials->DrawPrimitives2DFFP(PRIM_TRIANGLE_STRIP,r2,elementsOf(r2), NULL, color2, &blending);
	materials->DrawPrimitives2DFFP(PRIM_TRIANGLE_STRIP,r3,elementsOf(r3), NULL, color2, &blending);
}

void CEqSysConsole::DrawFastFind(float x, float y, float w)
{
	con_fastfind_isbeingselected = false;
	con_fastfind_selection_index = -1;

	BlendStateParam_t blending;
	blending.srcFactor = BLENDFACTOR_SRC_ALPHA;
	blending.dstFactor = BLENDFACTOR_ONE_MINUS_SRC_ALPHA;

	eqFontStyleParam_t helpTextParams;
	helpTextParams.textColor = s_conHelpTextColor;
	helpTextParams.styleFlag = TEXT_STYLE_FROM_CAP;

	if(con_Text.GetLength() > 0 && con_fastfind.GetBool())
	{
		const DkList<ConCommandBase*> *base = g_sysConsole->GetAllCommands();

		int enumcount = 0;
		int ff_numelems = 0;
		int enumcount2 = 0;

		for(int i = 0; i < base->numElem();i++)
		{
			EqString conVarName(base->ptr()[i]->GetName());

			if(conVarName.Find(con_Text.c_str()) != -1)
			{
				//if(enumcount < con_fastfind_count.GetInt())
					ff_numelems++;

				if(base->ptr()[i]->GetFlags() & CV_INVISIBLE)
					continue;

				enumcount++;
			}
		}

		EqString name(con_Text);

		if(isspace(name.GetData()[name.GetLength()-1]))
			name = name.Left(name.GetLength()-1);

		ConCommandBase* pCommand = (ConCommandBase*)g_sysConsole->FindBase( name.GetData() );

		int commandinfo_size = 0;

		// show command info
		if(pCommand)
		{
			int numLines = 1;
			const char* desc_str = pCommand->GetDesc();
			char c = desc_str[0];

			do
			{
				if(c == '\0')
					break;

				if(c == '\n')
					numLines++;
			}while(c = *desc_str++);

			EqString string_to_draw(_Es(pCommand->GetName()) + _Es(" - ") + pCommand->GetDesc());

			if(pCommand->IsConVar())
			{
				ConVar* pVar = (ConVar*)pCommand;
				if(pVar->HasClamp())
				{
					string_to_draw.Append(varargs(" \n\nValue in range [%g..%g]\n", pVar->GetMinClamp(), pVar->GetMaxClamp()));

					numLines += 2;
				}
			}

			// draw as autocompletion
			Rectangle_t rect(x,y,w,y+numLines*m_font->GetLineHeight()+2);
			DrawAlphaFilledRectangle(rect, s_conBackFastFind, s_conBorderColor);

			m_font->RenderText(string_to_draw.GetData(), rect.GetLeftTop() + Vector2D(5,4), helpTextParams);

			// draw autocompletion if available
			commandinfo_size += DrawAutoCompletion(x, rect.vrightBottom.y, w);
			commandinfo_size += numLines+1;

			if(enumcount <= 1)
				return;
		}

		if(ff_numelems >= con_fastfind_count.GetInt()-1)
			enumcount = 1;

		if(ff_numelems <= 0)
			return;

		y += m_font->GetLineHeight()*commandinfo_size;

		Rectangle_t rect(x,y,w,y+enumcount*m_font->GetLineHeight()+2);
		DrawAlphaFilledRectangle(rect, s_conBackFastFind, s_conBorderColor);

		if(ff_numelems >= con_fastfind_count.GetInt()-1)
		{
			// Set color
			char* str = varargs("%d commands found (too many to show here)", ff_numelems);
			m_font->RenderText(str, rect.GetLeftTop() + Vector2D(5,4), helpTextParams);

			return;
		}

		for(int i = 0; i < base->numElem();i++)
		{
			EqString conVarName = base->ptr()[i]->GetName();

			if(conVarName.Find( con_Text.GetData() ) != -1)
			{
				bool bHasAutocompletion = false;
				for(int j = 0;j < m_hAutoCompletionNodes.numElem();j++)
				{
					if(!stricmp(base->ptr()[i]->GetName(),m_hAutoCompletionNodes[j]->cmd_name.GetData()))
					{
						bHasAutocompletion = true;
						break;
					}
				}

				if(enumcount2 >= con_fastfind_count.GetInt())
					break;

				if(base->ptr()[i]->GetFlags() & CV_INVISIBLE)
					continue;

				float textYPos = (y + enumcount2 * m_font->GetLineHeight()) + 4;

				enumcount2++;

				bool bSelected = false;

				int clen = strlen(base->ptr()[i]->GetName());

				if(IsInRectangle(m_mousePosition.x,m_mousePosition.y,x,textYPos+2,w-x,12))
				{
					g_pShaderAPI->Reset(STATE_RESET_TEX);
					Vertex2D_t selrect[] = { MAKETEXQUAD(x, textYPos, w, textYPos + 15 , 0) };

					materials->DrawPrimitives2DFFP(PRIM_TRIANGLE_STRIP,selrect,elementsOf(selrect), NULL, ColorRGBA(1.0f, 1.0f, 1.0f, 0.8f), &blending);

					g_pShaderAPI->Apply();

					bSelected = true;

					con_fastfind_isbeingselected = true;
					con_fastfind_isbeingselected_autoc = false;

					con_fastfind_selection_index = i;
				}

				ColorRGBA text_color = Vector4D(0.7f,0.7f,0,1);
				if(base->ptr()[i]->IsConCommand())
					text_color = Vector4D(0.7f,0.7f,0.8f,1);

				Vector4D selTextColor = bSelected ? Vector4D(0.1f,0.1f,0,1) : text_color;

				eqFontStyleParam_t variantsTextParams;
				variantsTextParams.textColor = selTextColor;
				variantsTextParams.styleFlag = TEXT_STYLE_FROM_CAP;

				if(base->ptr()[i]->IsConVar())
				{
					ConVar *cv = (ConVar*)base->ptr()[i];
					char* str = NULL;

					if(bHasAutocompletion)
						str = varargs("%s [%s] ->", cv->GetName(), cv->GetString());
					else
						str = varargs("%s [%s]",cv->GetName(), cv->GetString());

					m_font->RenderText( str, Vector2D(x+5, textYPos),variantsTextParams);
				}
				else
				{
					if(!bHasAutocompletion)
					{
						char* str = varargs("%s ()",base->ptr()[i]->GetName());

						m_font->RenderText( str, Vector2D(x+5, textYPos),variantsTextParams);
					}
					else
					{
						char* str = varargs("%s ()",base->ptr()[i]->GetName());

						m_font->RenderText( str, Vector2D(x+5, textYPos),variantsTextParams);
					}
				}

				const char* cvarName = base->ptr()[i]->GetName();

				const char* cstr = xstristr(cvarName, con_Text.GetData());

				if(cstr)
				{
					int ofs = cstr - base->ptr()[i]->GetName();
					int len = con_Text.GetLength();

					float lookupStrStart = m_font->GetStringWidth(cvarName, variantsTextParams.styleFlag, ofs);
					float lookupStrEnd = lookupStrStart + m_font->GetStringWidth(cvarName+ofs, variantsTextParams.styleFlag, len);

					Vertex2D_t rect[] = { MAKETEXQUAD(x+5 + lookupStrStart, textYPos-2, x+5 + lookupStrEnd, textYPos+12, 0) };

					// Cancel textures
					g_pShaderAPI->Reset();

					materials->DrawPrimitives2DFFP(PRIM_TRIANGLE_STRIP,rect,elementsOf(rect), NULL, ColorRGBA(1.0f, 1.0f, 1.0f, 0.3f), &blending);
				}
			}
		}
	}
}

int CEqSysConsole::DrawAutoCompletion(float x, float y, float w)
{
	con_fastfind_isbeingselected_autoc = false;
	con_fastfind_selection_autocompletion_index = -1;
	con_fastfind_selection_autocompletion_val_index = -1;

	EqString name(con_Text);
	if(isspace(name.GetData()[name.GetLength()-1]))
	{
		name = name.Left(name.GetLength()-1);
	}

	int enumcount = 0;
	int enumcount2 = 1;

	if(name.GetLength() > 0 && con_autocompletion_enable.GetBool())
	{
		int max_string_length = 35;

		const DkList<ConCommandBase*> *base = g_sysConsole->GetAllCommands();

		for(int i = 0; i < base->numElem();i++)
		{
			if(!stricmp((char*)base->ptr()[i]->GetName(),name.GetData()))
			{
				if(enumcount >= con_fastfind_count.GetInt())
					break;

				if(base->ptr()[i]->GetFlags() & CV_INVISIBLE)
					continue;

				for(int j = 0;j < m_hAutoCompletionNodes.numElem();j++)
				{
					if(stricmp(base->ptr()[i]->GetName(),m_hAutoCompletionNodes[j]->cmd_name.GetData()))
						continue;

					for(int k = 0;k < m_hAutoCompletionNodes[j]->args.numElem();k++)
					{
						enumcount++;
						int str_l = strlen(m_hAutoCompletionNodes[j]->args[k].GetData());

						if(max_string_length < str_l)
							max_string_length = str_l;
					}
				}
			}
		}

		if(enumcount <= 0)
			return 0;

		eqFontStyleParam_t variantsTextParams;
		variantsTextParams.textColor = s_conInputTextColor;
		variantsTextParams.styleFlag = TEXT_STYLE_FROM_CAP;

		enumcount++;

		// draw as autocompletion
		Rectangle_t rect(x,y,x+max_string_length*FONT_WIDE,y+enumcount*m_font->GetLineHeight());
		DrawAlphaFilledRectangle(rect, s_conBackFastFind, s_conBorderColor);

		m_font->RenderText("Possible variants: ", Vector2D(x+5,y+2), variantsTextParams);



		for(int i = 0; i < base->numElem();i++)
		{
			if(!stricmp((char*)base->ptr()[i]->GetName(),name.GetData()))
			{
				if(enumcount2 >= con_fastfind_count.GetInt())
					break;

				if(base->ptr()[i]->GetFlags() & CV_INVISIBLE)
					continue;

				for(int j = 0;j < m_hAutoCompletionNodes.numElem();j++)
				{
					if(stricmp(base->ptr()[i]->GetName(),m_hAutoCompletionNodes[j]->cmd_name.GetData()))
						continue;

					for(int k = 0;k < m_hAutoCompletionNodes[j]->args.numElem();k++)
					{
						float textYPos = (y + enumcount2*m_font->GetLineHeight());

						enumcount2++;

						bool bSelected = false;

						if(IsInRectangle(m_mousePosition.x,m_mousePosition.y,
										x,textYPos,(max_string_length*FONT_WIDE)-x,12))
						{
							Vertex2D_t selrect[] = { MAKETEXQUAD(x, textYPos-4,x+max_string_length*FONT_WIDE, textYPos + 14 , 1) };

							// Cancel textures
							g_pShaderAPI->Reset(STATE_RESET_TEX);

							// Draw the rectangle
							materials->DrawPrimitives2DFFP(PRIM_TRIANGLE_STRIP,selrect,elementsOf(selrect));

							bSelected = true;

							con_fastfind_isbeingselected_autoc = true;
							con_fastfind_selection_autocompletion_index = j;
							con_fastfind_selection_autocompletion_val_index = k;
						}

						Vector4D selTextColor = bSelected ? s_conSelectedTextColor : s_conTextColor;

						variantsTextParams.textColor = selTextColor;
						m_font->RenderText(m_hAutoCompletionNodes[j]->args[k].GetData(), Vector2D(x+5,textYPos), variantsTextParams);
					}
				}
			}
		}
	}

	return enumcount2;
}

void CEqSysConsole::SetLastLine()
{
	m_logScrollPosition = GetAllMessages()->numElem() - m_maxLines;
}

void CEqSysConsole::AddToLinePos(int num)
{
	m_logScrollPosition += num;
}

void CEqSysConsole::DrawSelf(bool transparent,int width,int height, float curTime)
{
	BlendStateParam_t blending;
	blending.srcFactor = BLENDFACTOR_SRC_ALPHA;
	blending.dstFactor = BLENDFACTOR_ONE_MINUS_SRC_ALPHA;

	fullscreen = transparent;

	int drawstart = m_logScrollPosition;

	if(m_logScrollPosition > GetAllMessages()->numElem())
		m_logScrollPosition = GetAllMessages()->numElem();

	if(!m_visible)
		return;

	m_maxLines = floor(height / m_font->GetLineHeight()) - 2;

	if(m_maxLines < -4)
		return;

	int drawending = (-drawstart*m_font->GetLineHeight()) + m_font->GetLineHeight() * (m_maxLines+drawstart);

	int draws = 0;

	Vector4D color(0.75f, 0.75f, 0.75f, 1);

	static ITexture* pConsoleFrame = g_pShaderAPI->LoadTexture("ui/ui_console", TEXFILTER_TRILINEAR_ANISO,ADDRESSMODE_WRAP, TEXFLAG_NOQUALITYLOD);

	if(!transparent)
	{
		Vertex2D_t tmprect[] = { MAKETEXQUAD(0, 0,(float) width, (float)height, 0) };

		// Cancel textures
		g_pShaderAPI->Reset();

		// Set alpha,rasterizer and depth parameters
		materials->SetBlendingStates(BLENDFACTOR_ONE, BLENDFACTOR_ZERO);
		materials->SetRasterizerStates(CULL_FRONT,FILL_SOLID);
		materials->SetDepthStates(false,false);
		g_pShaderAPI->SetTexture( pConsoleFrame );

		materials->DrawPrimitives2DFFP(PRIM_TRIANGLE_STRIP,tmprect,elementsOf(tmprect), pConsoleFrame, color, &blending);

		// render slider
	}

	Rectangle_t inputTextEntryRect(64, 26, width-64,46);

	Rectangle_t con_outputRectangle(64,inputTextEntryRect.vrightBottom.y+26, width - 64, height-inputTextEntryRect.vleftTop.y);

	if(m_logVisible)
	{
		DrawAlphaFilledRectangle(con_outputRectangle, s_conBackColor, s_conBorderColor);

		int cnumLines = 0;

		int numRenderLines = GetAllMessages()->numElem();

		m_maxLines = (con_outputRectangle.GetSize().y / m_font->GetLineHeight())-1;

		int numDrawn = 0;

		eqFontStyleParam_t outputTextStyle;

		eqFontStyleParam_t hasLinesStyle;
		hasLinesStyle.textColor = ColorRGBA(0.5f,0.5f,1.0f,1.0f);

		con_outputRectangle.vleftTop.x += 5.0f;
		con_outputRectangle.vleftTop.y += m_font->GetLineHeight();

		static CRectangleTextLayoutBuilder rectLayout;
		rectLayout.SetRectangle( con_outputRectangle );

		outputTextStyle.layoutBuilder = &rectLayout;

		for(int i = drawstart; i < numRenderLines; i++, numDrawn++)
		{
			outputTextStyle.textColor = GetAllMessages()->ptr()[i]->color;

			m_font->RenderText(GetAllMessages()->ptr()[i]->text, con_outputRectangle.vleftTop, outputTextStyle);

			con_outputRectangle.vleftTop.y += m_font->GetLineHeight()*rectLayout.GetProducedLines();//cnumLines;

			if(rectLayout.HasNotDrawnLines() || i-drawstart >= m_maxLines)
			{
				m_font->RenderText("^ ^ ^ ^ ^ ^", Vector2D(con_outputRectangle.vleftTop.x, con_outputRectangle.vrightBottom.y), hasLinesStyle);

				break;
			}
		}
	}

	DrawFastFind( 128, inputTextEntryRect.vleftTop.y+25, width-128 );

	EqString conInputStr(CONSOLE_INPUT_STARTSTR);
	conInputStr.Append(con_Text);

	if(m_cursorTime < curTime)
	{
		m_cursorVisible = !m_cursorVisible;
		m_cursorTime = curTime + CURSOR_BLINK_TIME;
	}

	DrawAlphaFilledRectangle(inputTextEntryRect, s_conInputBackColor, s_conBorderColor);

	eqFontStyleParam_t inputTextStyle;
	inputTextStyle.textColor = s_conInputTextColor;

	Vector2D inputTextPos(inputTextEntryRect.vleftTop.x+4, inputTextEntryRect.vrightBottom.y-6);

	// render input text
	m_font->RenderText(conInputStr.c_str(), inputTextPos, inputTextStyle);

	float inputGfxOfs = m_font->GetStringWidth(CONSOLE_INPUT_STARTSTR, inputTextStyle.styleFlag);
	float cursorPosition = inputGfxOfs + m_font->GetStringWidth(con_Text.c_str(), inputTextStyle.styleFlag, m_cursorPos);

	// render selection
	if(m_startCursorPos != -1)
	{
		float selStartPosition = inputGfxOfs + m_font->GetStringWidth(con_Text.c_str(), inputTextStyle.styleFlag, m_startCursorPos);

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

void CEqSysConsole::MousePos(const Vector2D &pos)
{
	m_mousePosition = pos;
}

bool CEqSysConsole::KeyChar(int ch)
{
	if(!m_visible)
		return false;

	// Font is not loaded, skip
	if(m_font == NULL)
		return false;

	if(ch == '~')
		return false;

	CFont* cFont = (CFont*)m_font;

	if(ch )

	// THis is a weird thing
	if(cFont->GetFontCharById(ch).advX > 0.0f && ch != '`')
	{
		char text[2];
		text[0] = ch;
		text[1] = 0;

		con_Text.Insert( text, m_cursorPos);
		m_cursorPos += 1;
	}
}

bool CEqSysConsole::MouseEvent(const Vector2D &pos, int Button,bool pressed)
{
	if(!m_visible)
		return false;

	if (pressed)
	{
		if (Button == MOU_B1)
		{
			if(m_visible)
			{
				if(con_fastfind_isbeingselected && con_fastfind_selection_index != -1)
				{
					const DkList<ConCommandBase*> *base = g_sysConsole->GetAllCommands();
					if(con_fastfind_selection_index < base->numElem())
					{
						bool bHasAutocompletion = false;
						for(int j = 0;j < m_hAutoCompletionNodes.numElem();j++)
						{
							if(!stricmp(base->ptr()[con_fastfind_selection_index]->GetName(),m_hAutoCompletionNodes[j]->cmd_name.GetData()))
							{
								bHasAutocompletion = true;
								break;
							}
						}

						con_Text = base->ptr()[con_fastfind_selection_index]->GetName();

						if(!bHasAutocompletion)
							con_Text = con_Text + _Es(" ");

						m_cursorPos = con_Text.GetLength();
					}
				}

				if(con_fastfind_isbeingselected_autoc)
				{
					if(con_fastfind_selection_autocompletion_index < m_hAutoCompletionNodes.numElem())
					{
						int ac_ind = con_fastfind_selection_autocompletion_index;
						int val_ind = con_fastfind_selection_autocompletion_val_index;

						con_Text = m_hAutoCompletionNodes[ac_ind]->cmd_name + _Es(" \"") + m_hAutoCompletionNodes[ac_ind]->args[val_ind] + _Es("\"");
						m_cursorPos = con_Text.GetLength();
					}
				}
			}
		}
	}

	return true;
}

bool CEqSysConsole::KeyPress(int key, bool pressed)
{
	if( pressed ) // catch "DOWN" event
	{
		int action_index = GetKeyBindings()->GetBindingIndexByKey(key);

		if(action_index != -1)
		{
			if(!stricmp(GetKeyBindings()->GetBindingList()->ptr()[action_index]->commandString.GetData(), "toggleconsole"))
			{
				if(g_pSysConsole->IsVisible() && g_pSysConsole->IsShiftPressed())
				{
					g_pSysConsole->SetLogVisible( !g_pSysConsole->IsLogVisible() );
					return false;
				}

				SetVisible(!IsVisible());
				return false;
			}
		}
	}



	if(!m_visible)
		return false;

	if(pressed)
	{
		switch (key)
		{
			case KEY_BACKSPACE:
				if(m_startCursorPos != -1 && m_startCursorPos != m_cursorPos)
				{
					if(m_cursorPos > m_startCursorPos)
					{
						consoleRemTextInRange(m_startCursorPos, m_cursorPos - m_startCursorPos);
						m_cursorPos = m_startCursorPos;
					}
					else
					{
						consoleRemTextInRange(m_cursorPos, m_startCursorPos - m_cursorPos);
					}

					m_startCursorPos = -1; //m_cursorPos;
					con_histIndex = commandHistory.numElem()-1;
					return true;
				}

				if (m_cursorPos > 0)
				{
					m_cursorPos--;
					consoleRemTextInRange(m_cursorPos, 1);
					con_histIndex = commandHistory.numElem()-1;
				}
				return true;
			case KEY_DELETE:
				if (m_cursorPos <= con_Text.GetLength())
				{
					if(m_startCursorPos != -1 && m_startCursorPos != m_cursorPos)
					{
						if(m_cursorPos > m_startCursorPos)
						{
							consoleRemTextInRange(m_startCursorPos, m_cursorPos - m_startCursorPos);
							m_cursorPos = m_startCursorPos;
						}
						else
						{
							consoleRemTextInRange(m_cursorPos, m_startCursorPos - m_cursorPos);
						}

						m_startCursorPos = m_cursorPos;
						con_histIndex = commandHistory.numElem()-1;
						return true;
					}
					consoleRemTextInRange(m_cursorPos, 1);
					con_histIndex = commandHistory.numElem()-1;
				}
				return true;
			case KEY_SHIFT:
				if(!m_shiftModifier)
				{
					m_startCursorPos = m_cursorPos;
					m_shiftModifier = true;
				}
				return true;
			case KEY_CTRL:
				m_ctrlModifier = true;

				return true;
			case KEY_HOME:
				m_logScrollPosition = 0;
				return true;
			case KEY_END:
				m_logScrollPosition = clamp(GetAllMessages()->numElem() - m_maxLines, 0, GetAllMessages()->numElem()-1);
				return true;
			case KEY_A:
				if(m_ctrlModifier)
				{
					m_cursorPos = con_Text.GetLength();
					m_startCursorPos = 0;
				}
			case KEY_C:
				if(m_ctrlModifier)
				{
					if(m_startCursorPos != -1 && m_startCursorPos != m_cursorPos)
					{
						bool bInverseSelection = m_cursorPos > m_startCursorPos;
						int cpystartpos = bInverseSelection ? m_startCursorPos : m_cursorPos;
						int cpylength = bInverseSelection ? (m_cursorPos - m_startCursorPos) : (m_startCursorPos - m_cursorPos);

						EqString tmpString(con_Text.Mid(cpystartpos,cpylength));

#ifdef PLAT_SDL
						// simple, yea
						SDL_SetClipboardText(tmpString.c_str());
#elif PLAT_WIN
						if(OpenClipboard( m_hwnd ))
						{
							HGLOBAL hgBuffer;

							char* chBuffer;
							EmptyClipboard();
							hgBuffer= GlobalAlloc(GMEM_DDESHARE, tmpString.GetLength()+1);
							chBuffer= (char*)GlobalLock(hgBuffer);
							strcpy(chBuffer, tmpString.GetData());
							GlobalUnlock(hgBuffer);
							SetClipboardData(CF_TEXT,hgBuffer);
							CloseClipboard();
						}
#endif // PLAT_SDL
					}
				}
				return true;
			case KEY_X:
				if(m_ctrlModifier)
				{
					if(m_startCursorPos != -1 && m_startCursorPos != m_cursorPos)
					{
						bool bInverseSelection = m_cursorPos > m_startCursorPos;
						int cpystartpos = bInverseSelection ? m_startCursorPos : m_cursorPos;
						int cpylength = bInverseSelection ? (m_cursorPos - m_startCursorPos) : (m_startCursorPos - m_cursorPos);

						EqString tmpString(con_Text.Mid(cpystartpos,cpylength));
#ifdef PLAT_SDL
						SDL_SetClipboardText(tmpString.c_str());
#elif PLAT_WIN
						if(OpenClipboard( m_hwnd ))
						{
							HGLOBAL hgBuffer;

							char* chBuffer;
							EmptyClipboard();
							hgBuffer= GlobalAlloc(GMEM_DDESHARE, tmpString.GetLength()+1);
							chBuffer= (char*)GlobalLock(hgBuffer);
							strcpy(chBuffer, tmpString.GetData());
							GlobalUnlock(hgBuffer);
							SetClipboardData(CF_TEXT,hgBuffer);
							CloseClipboard();
						}
#endif // PLAT_SDL

						if(m_cursorPos > m_startCursorPos)
						{
							consoleRemTextInRange(m_startCursorPos, m_cursorPos - m_startCursorPos);
							m_cursorPos = m_startCursorPos;
						}
						else
						{
							consoleRemTextInRange(m_cursorPos, m_startCursorPos - m_cursorPos);
						}

						m_startCursorPos = -1;
					}
				}
				return true;
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
							return true;
						}

						tmpString = chBuffer;
						GlobalUnlock(hData);
						CloseClipboard();
#endif // PLAT_SDL

						if(m_startCursorPos != -1)
						{
							if(m_cursorPos > m_startCursorPos)
							{
								consoleRemTextInRange(m_startCursorPos, m_cursorPos - m_startCursorPos);
								m_cursorPos = m_startCursorPos;
							}
							else
							{
								consoleRemTextInRange(m_cursorPos, m_startCursorPos - m_cursorPos);
							}
						}

						consoleInsText((char*)tmpString.GetData(),m_cursorPos);
						m_cursorPos += tmpString.GetLength();
					}
				}
				return true;
			case KEY_ENTER:
				if (con_Text.GetLength() > 0)
				{
					MsgInfo("> %s\n",con_Text.GetData());

					g_sysConsole->ResetCounter();
					g_sysConsole->SetCommandBuffer((char*)con_Text.GetData());

					bool stat = g_sysConsole->ExecuteCommandBuffer();
					if(!stat)
						Msg("Unknown command or variable '%s'\n",con_Text.GetData());

					// Compare the last command with current and add history if needs
					if(commandHistory.numElem() > 0)
					{
						if(stricmp(commandHistory[commandHistory.numElem()-1].GetData(),con_Text.GetData()))
						{
							commandHistory.append(con_Text);
						}
					}
					else
					{
						commandHistory.append(con_Text);
					}
					con_histIndex = commandHistory.numElem() - 1;

				}
				else
				{
					Msg("]\n");
				}
				con_Text = "";
				m_cursorPos = 0;
				return true;

			case KEY_TAB:
				{
					const DkList<ConCommandBase*> *base = g_sysConsole->GetAllCommands();

					int enumcount = 0;
					int enumcount2 = 0;

					int cmdindex = 0;
					int char_index = 0;

					int max_match_chars = -1;
					EqString matching_str;

					EqString con_Text_low = con_Text.LowerCase();

					for(int i = 0; i < base->numElem();i++)
					{
						EqString conVarName = _Es(base->ptr()[i]->GetName()).LowerCase();
						char_index = conVarName.Find(con_Text_low.c_str(), true);

						if(char_index != -1 && !(base->ptr()[i]->GetFlags() & CV_INVISIBLE))
						{
							if(char_index == 0 && !con_fastfind.GetBool())
							{
								MsgAccept("%s - %s\n",base->ptr()[i]->GetName(),base->ptr()[i]->GetDesc());
								enumcount2++;
							}

							if(max_match_chars == -1)
							{
								matching_str = conVarName;
								max_match_chars = matching_str.GetLength();
							}
							else
							{
								int cmpLen = matching_str.GetMathingChars( conVarName );

								if(cmpLen < max_match_chars)
								{
									matching_str = matching_str.Left(cmpLen);
									max_match_chars = cmpLen;
								}
							}

							enumcount++;
							cmdindex = i;
						}
					}

					if(enumcount2 > 1)
						Msg(" \n");

					con_histIndex = commandHistory.numElem()-1;

					if(enumcount == 1)
					{
						con_Text = varargs("%s ",base->ptr()[cmdindex]->GetName());
						m_cursorPos = con_Text.GetLength();
					}
					else if(max_match_chars != 0)
					{
						con_Text = matching_str;
						m_cursorPos = con_Text.GetLength();
					}

				}
				return true;

			case KEY_PGUP:
				if(m_logScrollPosition > 0)
					m_logScrollPosition--;
				return true;
			case KEY_PGDN:
				//int m_maxLines;

				if(fullscreen)
					m_maxLines = ((m_height) / 22) -2;
				else
					m_maxLines = ((m_height*2) / 22) -2;

				if(m_logScrollPosition < GetAllMessages()->numElem())
				{
					m_logScrollPosition++;
				}
				return true;

			case KEY_LEFT:
				if (m_cursorPos > 0)
				{
					m_cursorPos--;

					if(!m_shiftModifier)
						m_startCursorPos = -1;
				}
				return true;
			case KEY_TILDE:
				if(!fullscreen)
					return true;

				con_histIndex = commandHistory.numElem()-1;

				//m_visible = !m_visible;
				return true;
			case KEY_RIGHT:
				if (m_cursorPos < con_Text.GetLength())
				{
					m_cursorPos++;

					if(!m_shiftModifier)
						m_startCursorPos = -1;
				}
				return true;
			case KEY_DOWN: // FIXME: invalid indices

				if(!commandHistory.numElem())
					return true;

				if(con_Text.GetLength() == 0)
				{
					// display last
					con_histIndex = 0;

					con_Text = commandHistory[con_histIndex];
					m_cursorPos = commandHistory[con_histIndex].GetLength();
					return true;
				}

				con_histIndex++;

				if(con_histIndex < 0) con_histIndex = 0;
				if(con_histIndex > commandHistory.numElem()-1) con_histIndex = commandHistory.numElem()-1;

				con_Text = commandHistory[con_histIndex];
				m_cursorPos = commandHistory[con_histIndex].GetLength();

				return true;
			case KEY_UP:
				if(!commandHistory.numElem())
					return true;

				if(con_Text.GetLength() == 0)
				{
					// display last
					con_histIndex = commandHistory.numElem()-1;

					con_Text = commandHistory[con_histIndex];
					m_cursorPos = commandHistory[con_histIndex].GetLength();
					return true;
				}

				con_histIndex--;

				if(con_histIndex < 0) con_histIndex = 0;
				if(con_histIndex > commandHistory.numElem()-1) con_histIndex = commandHistory.numElem()-1;

				con_Text = commandHistory[con_histIndex];
				m_cursorPos = commandHistory[con_histIndex].GetLength();

				return true;
			default:
				con_histIndex = commandHistory.numElem()-1;
				m_startCursorPos = -1;
				return true;
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
		}
	}

	return true;
}
