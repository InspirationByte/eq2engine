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

#include "materialsystem/MeshBuilder.h"
#include "KeyBinding/InputCommandBinder.h"

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
	g_pSysConsole->SetLogVisible(false);
}

// shows console with log
DECLARE_CMD(con_show_full, "Show console", 0)
{
	g_pSysConsole->SetVisible(true);
	g_pSysConsole->SetLogVisible(true);
}

// hides console
DECLARE_CMD(con_hide, "Hides console", 0)
{
	g_pSysConsole->SetVisible(false);
	g_pSysConsole->SetLogVisible(false);
}

ConVar con_enable("con_enable","1",NULL, CV_CHEAT);
ConVar con_fastfind("con_fastfind","1",NULL,CV_ARCHIVE);
ConVar con_fastfind_count("con_fastfind_count","35",NULL,CV_ARCHIVE);
ConVar con_autocompletion_enable("con_autocompletion_enable","1","See file <game dir>/cfg/autocompletion.cfg",CV_ARCHIVE);

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

/*
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
			if(filename.Length() > 1 && filename != ".." && filename != "." && !(wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
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
*/

DECLARE_CMD(autocompletion_addcommandbase,"Adds autocompletion variants", CV_INVISIBLE)
{
	if(CMD_ARGC == 0)
	{
		Msg("Usage: autocompletion_addcommandbase <console command/variable> [argument1,argument2,...]\n");
		return;
	}

	AutoCompletionNode_s* pNode = new AutoCompletionNode_s;
	pNode->cmd_name = CMD_ARGV(0);

	xstrsplit(CMD_ARGV(1).c_str(),",",pNode->args);

	g_pSysConsole->AddAutoCompletionNode(pNode);
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
	m_fullscreen = false;

	m_cursorPos = 0;
	m_startCursorPos = -1;
	m_logScrollPosition = 0;
	m_histIndex = 0;
	m_fastfind_cmdbase = NULL;
	m_variantSelection = -1;

	m_alternateHandler = NULL;
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
	if(uint(start+len) > m_inputText.Length())
		return;

	m_inputText.Remove(start,len);
	OnTextUpdate();
}

void CEqSysConsole::consoleInsText(char* text,int pos)
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

void CEqSysConsole::DrawFastFind(float x, float y, float w)
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

		int startDraw = m_commandHistory.numElem()-con_fastfind_count.GetInt();
		startDraw = max(0,startDraw);

		//for(int order = 0, i = m_commandHistory.numElem()-1; i >= startDraw;i--,order++)
		for(int i = startDraw; i < m_commandHistory.numElem(); i++)
		{
			eqFontStyleParam_t historyTextParams;
			historyTextParams.textColor = Vector4D(0.7f,0.7f,0,1);
			historyTextParams.styleFlag = TEXT_STYLE_FROM_CAP;

			int textYPos = (i+1)*m_font->GetLineHeight(helpTextParams);

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

	if(con_fastfind.GetBool())
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

		if(m_foundCmdList.numElem() >= con_fastfind_count.GetInt())
		{
			Rectangle_t rect(x,y,w,y + m_font->GetLineHeight(helpTextParams) + 2 );
			DrawAlphaFilledRectangle(rect, s_conBackFastFind, s_conBorderColor);

			// Set color
			char* str = varargs("%d commands found (too many to show here)", m_foundCmdList.numElem());
			m_font->RenderText(str, rect.GetLeftTop() + Vector2D(5,4), helpTextParams);

			return;
		}
		else if(m_foundCmdList.numElem() > 0)
		{
			int max_string_length = 35;

			for(int i = 0; i < m_foundCmdList.numElem(); i++)
				max_string_length = max((uint)max_string_length, strlen(m_foundCmdList[i]->GetName()));

			int numElemsToDraw = m_foundCmdList.numElem();

			Rectangle_t rect(x,y,x+max_string_length*FONT_WIDE,y+numElemsToDraw*m_font->GetLineHeight(helpTextParams)+2);
			DrawAlphaFilledRectangle(rect, s_conBackFastFind, s_conBorderColor);

			for(int i = 0; i < m_foundCmdList.numElem();i++)
			{
				ConCommandBase* cmdBase = m_foundCmdList[i];

				bool bHasAutocompletion = cmdBase->HasVariants();

				// find autocompletion
				if(!bHasAutocompletion)
				{
					for(int j = 0;j < m_hAutoCompletionNodes.numElem();j++)
					{
						if(!stricmp(cmdBase->GetName(), m_hAutoCompletionNodes[j]->cmd_name.GetData()))
						{
							bHasAutocompletion = true;
							break;
						}
					}
				}

				bool bSelected = false;

				int clen = strlen(cmdBase->GetName());

				float textYPos = (y + i * m_font->GetLineHeight(helpTextParams)) + 4;

				if(IsInRectangle(m_mousePosition.x,m_mousePosition.y,x,textYPos+2,w-x,12) || m_cmdSelection == i)
				{
					g_pShaderAPI->Reset(STATE_RESET_TEX);
					Vertex2D_t selrect[] = { MAKETEXQUAD(x, textYPos, x+max_string_length*FONT_WIDE, textYPos + 15 , 0) };

					materials->DrawPrimitives2DFFP(PRIM_TRIANGLE_STRIP,selrect,elementsOf(selrect), NULL, ColorRGBA(1.0f, 1.0f, 1.0f, 0.8f), &blending);
					g_pShaderAPI->Apply();

					m_cmdSelection = i;

					bSelected = true;
				}

				ColorRGBA text_color = Vector4D(0.7f,0.7f,0,1);
				if(cmdBase->IsConCommand())
					text_color = Vector4D(0.7f,0.7f,0.8f,1);

				Vector4D selTextColor = bSelected ? Vector4D(0.1f,0.1f,0,1) : text_color;

				eqFontStyleParam_t variantsTextParams;
				variantsTextParams.textColor = selTextColor;
				variantsTextParams.styleFlag = TEXT_STYLE_FROM_CAP;

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

void CEqSysConsole::OnTextUpdate()
{
	m_histIndex = -1;

	// update command variants
	UpdateCommandAutocompletionList();

	// update argument variants
	int spaceIdx = m_inputText.Find(" ");
	if(spaceIdx != -1)
	{
		EqString firstArguemnt(m_inputText.Left(spaceIdx));
		m_fastfind_cmdbase = (ConCommandBase*)g_sysConsole->FindBase( firstArguemnt.c_str() );

		if(m_fastfind_cmdbase)
		{
			EqString nextArguemnt(m_inputText.Mid(spaceIdx+1, m_inputText.Length()));
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

void CEqSysConsole::UpdateCommandAutocompletionList()
{
	m_foundCmdList.clear();
	m_cmdSelection = -1;

	const DkList<ConCommandBase*>* baseList = g_sysConsole->GetAllCommands();

	for(int i = 0; i < baseList->numElem();i++)
	{
		ConCommandBase* cmdBase = baseList->ptr()[i];

		EqString conVarName(cmdBase->GetName());

		// find by input text
		if(conVarName.Find(m_inputText.c_str()) != -1)
		{
			if(cmdBase->GetFlags() & CV_INVISIBLE)
				continue;

			m_foundCmdList.append(cmdBase);
		}
	}
}

void CEqSysConsole::UpdateVariantsList( const EqString& queryStr )
{
	if(!con_autocompletion_enable.GetBool())
		return;

	m_variantSelection = -1;
	m_variantList.clear(false);

	DkList<EqString> allAutoCompletion;

	for(int i = 0; i < m_hAutoCompletionNodes.numElem(); i++)
	{
		AutoCompletionNode_s* node = m_hAutoCompletionNodes[i];

		if(stricmp(m_fastfind_cmdbase->GetName(), node->cmd_name.GetData()))
			continue;

		for(int j = 0; j < node->args.numElem(); j++)
			allAutoCompletion.append( node->args[j] );

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

int CEqSysConsole::DrawAutoCompletion(float x, float y, float w)
{
	if(!(con_autocompletion_enable.GetBool() && m_fastfind_cmdbase))
		return 0;

	int max_string_length = 35;

	for(int i = 0; i < m_variantList.numElem(); i++)
		max_string_length = max((uint)max_string_length, m_variantList[i].Length());

	int displayEnd = con_fastfind_count.GetInt();
	int displayStart = 0;

	while(m_variantSelection-displayStart > con_fastfind_count.GetInt()-1)
	{
		displayStart += con_fastfind_count.GetInt();
		displayEnd += con_fastfind_count.GetInt();
	}

	displayEnd = min(displayEnd, m_variantList.numElem());

	if(displayEnd <= 0)
		return 0;

	eqFontStyleParam_t variantsTextParams;
	variantsTextParams.textColor = s_conInputTextColor;
	variantsTextParams.styleFlag = TEXT_STYLE_FROM_CAP;

	int linesToDraw = displayEnd-displayStart+1;

	// draw as autocompletion
	Rectangle_t rect(x,y,x+max_string_length*FONT_WIDE,y+linesToDraw*m_font->GetLineHeight(variantsTextParams));
	DrawAlphaFilledRectangle(rect, s_conBackFastFind, s_conBorderColor);

	m_font->RenderText("Possible variants: ", Vector2D(x+5,y+2), variantsTextParams);

	for(int line = 1, i = displayStart; i < displayEnd; i++)
	{
		float textYPos = (y + line*m_font->GetLineHeight(variantsTextParams));
		line++;

		bool bSelected = false;

		if(IsInRectangle(m_mousePosition.x,m_mousePosition.y,
						x,textYPos,(max_string_length*FONT_WIDE)-x,12) || m_variantSelection == i)
		{
			Vertex2D_t selrect[] = { MAKETEXQUAD(x, textYPos-4,x+max_string_length*FONT_WIDE, textYPos + 14 , 1) };

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

void CEqSysConsole::SetLastLine()
{
	m_logScrollPosition = GetAllMessages()->numElem() - m_maxLines;
	m_logScrollPosition = max(0,m_logScrollPosition);
}

void CEqSysConsole::AddToLinePos(int num)
{
	int totalLines = GetAllMessages()->numElem();
	m_logScrollPosition += num;
	m_logScrollPosition = min(totalLines,m_logScrollPosition);
}

void CEqSysConsole::SetText( const char* text, bool quiet /*= false*/ )
{
	m_inputText = text;
	m_cursorPos = m_startCursorPos = m_inputText.Length();

	if(!quiet)
		OnTextUpdate();
}

void CEqSysConsole::DrawSelf(bool transparent,int width,int height, float curTime)
{
	BlendStateParam_t blending;
	blending.srcFactor = BLENDFACTOR_SRC_ALPHA;
	blending.dstFactor = BLENDFACTOR_ONE_MINUS_SRC_ALPHA;

	eqFontStyleParam_t fontStyle;

	m_fullscreen = transparent;

	int drawstart = m_logScrollPosition;

	if(m_logScrollPosition > GetAllMessages()->numElem())
		m_logScrollPosition = GetAllMessages()->numElem();

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

		int numRenderLines = GetAllMessages()->numElem();

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
			outputTextStyle.textColor = GetAllMessages()->ptr()[i]->color;

			m_font->RenderText(GetAllMessages()->ptr()[i]->text, con_outputRectangle.vleftTop, outputTextStyle);

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

	float inputGfxOfs = m_font->GetStringWidth(CONSOLE_INPUT_STARTSTR, inputTextStyle);
	float cursorPosition = inputGfxOfs + m_font->GetStringWidth(m_inputText.c_str(), inputTextStyle, m_cursorPos);

	// render selection
	if(m_startCursorPos != -1)
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


	// THis is a weird thing
	if(m_font->GetFontCharById(ch).advX > 0.0f && ch != '`')
	{
		char text[2];
		text[0] = ch;
		text[1] = 0;

		m_inputText.Insert( text, m_cursorPos);
		m_cursorPos += 1;
		OnTextUpdate();
	}

	return true;
}

bool CEqSysConsole::MouseEvent(const Vector2D &pos, int Button,bool pressed)
{
	if(!m_visible)
		return false;

	if (pressed)
	{
		if (Button == MOU_B1)
		{
			if(m_fastfind_cmdbase && m_variantSelection != -1)
			{
				SetText((m_fastfind_cmdbase->GetName() + _Es(" \"") + m_variantList[m_variantSelection] + _Es("\"")).c_str());
				return true;
			}
			else if(m_foundCmdList.numElem() && m_cmdSelection != -1)
			{
				SetText( (m_foundCmdList[m_cmdSelection]->GetName() + _Es(" ")).c_str() );
				return true;
			}
		}
	}

	return true;
}
void CEqSysConsole::SetVisible(bool bVisible)
{
	m_visible = bVisible;

	if(!m_visible)
	{
		m_histIndex = -1;
	}
}

bool CEqSysConsole::KeyPress(int key, bool pressed)
{
	if( pressed ) // catch "DOWN" event
	{
		in_binding_t* tgBind = g_inputCommandBinder->LookupBinding(key);

		if(tgBind && !stricmp(tgBind->commandString.c_str(), "toggleconsole"))
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

					m_startCursorPos = -1;
					m_histIndex = -1;
					return true;
				}

				if (m_cursorPos > 0)
				{
					m_cursorPos--;
					consoleRemTextInRange(m_cursorPos, 1);
					m_histIndex = -1;
				}
				return true;
			case KEY_DELETE:
				if (m_cursorPos <= m_inputText.Length())
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
						m_histIndex = -1;
						return true;
					}
					consoleRemTextInRange(m_cursorPos, 1);
					m_histIndex = -1;
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
				SetLastLine();
				
				return true;
			case KEY_A:
				if(m_ctrlModifier)
				{
					m_cursorPos = m_inputText.Length();
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

						EqString tmpString(m_inputText.Mid(cpystartpos,cpylength));

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
				return true;
			case KEY_X:
				if(m_ctrlModifier)
				{
					if(m_startCursorPos != -1 && m_startCursorPos != m_cursorPos)
					{
						bool bInverseSelection = m_cursorPos > m_startCursorPos;
						int cpystartpos = bInverseSelection ? m_startCursorPos : m_cursorPos;
						int cpylength = bInverseSelection ? (m_cursorPos - m_startCursorPos) : (m_startCursorPos - m_cursorPos);

						EqString tmpString(m_inputText.Mid(cpystartpos,cpylength));
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
						m_cursorPos += tmpString.Length();
					}
				}
				return true;
			case KEY_ENTER:
				if (m_inputText.Length() > 0)
				{
					if(m_fastfind_cmdbase && m_variantSelection != -1)
					{
						SetText( (m_fastfind_cmdbase->GetName() + _Es(" \"") + m_variantList[m_variantSelection] + _Es("\"")).c_str() );
						return true;
					}
					else if(m_foundCmdList.numElem() && m_cmdSelection != -1)
					{
						SetText( (m_foundCmdList[m_cmdSelection]->GetName() + _Es(" ")).c_str() );
						return true;
					}

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
				else
				{
					if(m_histIndex != -1 && m_commandHistory.numElem())
					{
						SetText(m_commandHistory[m_histIndex].c_str());
						m_histIndex = -1;
						return true;
					}

					Msg("]\n");
				}

				SetText("");

				return true;

			case KEY_TAB:
				{
					//const DkList<ConCommandBase*> *base = g_sysConsole->GetAllCommands();
					int char_index = 0;

					int max_match_chars = -1;
					EqString matching_str;

					if(m_fastfind_cmdbase != NULL)
					{
						if(m_variantList.numElem() == 1)
						{
							SetText( (m_fastfind_cmdbase->GetName() + _Es(" ") + m_variantList[0]).c_str() );
						}
						else
						{
							EqString queryStr;

							int spaceIdx = m_inputText.Find(" ");
							if(spaceIdx != -1)
								queryStr = m_inputText.c_str() + spaceIdx + 1;

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

							if(max_match_chars != 0)
								SetText( (m_fastfind_cmdbase->GetName() + _Es(" ") + matching_str).c_str() );
							else
								SetText( (m_fastfind_cmdbase->GetName() + _Es(" ")).c_str() );
						}
					}
					else
					{
						int exactCmdIdx = -1;

						if(m_foundCmdList.numElem() == 1)
						{
							SetText( varargs("%s ", m_foundCmdList[0]->GetName()) );
							return true;
						}

						for(int i = 0; i < m_foundCmdList.numElem();i++)
						{
							ConCommandBase* cmdBase = m_foundCmdList[i];

							EqString conVarName(cmdBase->GetName());
							char_index = conVarName.Find( m_inputText.c_str() );

							if(char_index == -1)
								continue;

							if(char_index == 0)
							{
								exactCmdIdx = i;

								// if we have 100% match, force to use this command
								if(!m_inputText.CompareCaseIns(cmdBase->GetName()))
									break;

								if(!con_fastfind.GetBool())
									MsgAccept("%s - %s\n",cmdBase->GetName(), cmdBase->GetDesc());
							}

							if(max_match_chars == -1)
							{
								matching_str = conVarName;
								max_match_chars = matching_str.Length();
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
						}

						if(exactCmdIdx != -1)
							SetText( varargs("%s ", m_foundCmdList[exactCmdIdx]->GetName()) );
						else if(max_match_chars != 0)
							SetText( matching_str.c_str() );

						if(!con_fastfind.GetBool())
							Msg(" \n");
					}

				}
				return true;

			case KEY_PGUP:
				if(m_logScrollPosition > 0)
					m_logScrollPosition--;
				return true;
			case KEY_PGDN:
				//int m_maxLines;

				if(m_fullscreen)
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
				if(!m_fullscreen)
					return true;

				m_histIndex = -1;

				//m_visible = !m_visible;
				return true;
			case KEY_RIGHT:
				if (m_cursorPos < m_inputText.Length())
				{
					m_cursorPos++;

					if(!m_shiftModifier)
						m_startCursorPos = -1;
				}
				return true;
			case KEY_DOWN: // FIXME: invalid indices

				if(m_fastfind_cmdbase && m_variantList.numElem())
				{
					m_variantSelection++;
					m_variantSelection = min(m_variantSelection, m_variantList.numElem()-1);
					return true;
				}
				else
				{
					if(m_foundCmdList.numElem())
					{
						m_cmdSelection++;
						m_cmdSelection = min(m_cmdSelection, m_foundCmdList.numElem()-1);
						return true;
					}
				}

				if(!m_commandHistory.numElem())
					return true;

				m_histIndex++;
				m_histIndex = min(m_histIndex, m_commandHistory.numElem()-1);

				if(m_commandHistory.numElem() > 0)
					SetText(m_commandHistory[m_histIndex].c_str(), true);

				return true;
			case KEY_UP:

				if(m_fastfind_cmdbase && m_variantList.numElem())
				{
					m_variantSelection--;
					m_variantSelection = max(m_variantSelection, 0);
					return true;
				}
				else
				{
					if(m_foundCmdList.numElem())
					{
						m_cmdSelection--;
						m_cmdSelection = max(m_cmdSelection, 0);
						return true;
					}
				}

				if(!m_commandHistory.numElem())
					return true;

				if(m_histIndex == -1)
					m_histIndex = m_commandHistory.numElem();

				m_histIndex--;
				m_histIndex = max(m_histIndex, 0);

				if(m_commandHistory.numElem() > 0)
					SetText(m_commandHistory[m_histIndex].c_str(), true);

				return true;
			default:
				if(m_histIndex != -1 && key == KEY_ESCAPE)
					SetText("", true);

				m_histIndex = -1; // others and escape
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
