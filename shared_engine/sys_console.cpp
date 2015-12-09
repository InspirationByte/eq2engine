//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Provides base console interface
//////////////////////////////////////////////////////////////////////////////////

#include "sys_console.h"

#include <stdio.h>

//#include "IEngineGame.h"

#include "EngineSpew.h"
#include "EngineVersion.h"
#include "KeyBinding/Keys.h"

#ifdef _DEBUG
#define CONSOLE_ENGINEVERSION_STR varargs(ENGINE_NAME " Engine " ENGINE_VERSION " DEBUG build %d (" COMPILE_DATE ")", BUILD_NUMBER_ENGINE)
#else
#define CONSOLE_ENGINEVERSION_STR varargs(ENGINE_NAME " Engine " ENGINE_VERSION " build %d (" COMPILE_DATE ")", BUILD_NUMBER_ENGINE)
#endif // _DEBUG

#define CONSOLE_INPUT_STARTSTR ("> ")

ConVar con_cursortime("con_cursortime","0.2","Cursor decay time(console)",CV_ARCHIVE);
ConVar con_fastfind("con_fastfind","1","Show FastFind popup in console",CV_ARCHIVE);
ConVar con_fastfind_count("con_fastfind_count","35","FastFind listed vars count",CV_ARCHIVE);
ConVar con_autocompletion_enable("con_autocompletion_enable","1","Enable autocompletion for console\n See file <game dir>/common/autocompletion.cfg",CV_ARCHIVE);

static CEqSysConsole s_SysConsole;
CEqSysConsole* g_pSysConsole = &s_SysConsole;

#define FONT_WIDE 9
#define FONT_TALL 14

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

			EqString dirname = GetFileSystem()->GetCurrentGameDirectory() + _Es("/maps/");
			//AutoCompletionCheckExtensionDir("*.eqbsp",dirname.getData(),true,&pNode->args);

			EqString level_dir(GetFileSystem()->GetCurrentGameDirectory());
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
						if(!GetFileSystem()->FileExist(("worlds/" + filename + "/world.build").GetData()))
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

			EqString dirname = GetFileSystem()->GetCurrentGameDirectory() + _Es("/cfg/");
			AutoCompletionCheckExtensionDir("*.cfg",dirname.GetData(), false, &pNode->args);

			dirname = GetFileSystem()->GetCurrentDataDirectory() + _Es("/cfg/");
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

/*
void drawShadedRectangle(Vector2D &mins,Vector2D &maxs,Vector4D &color1,Vector4D &color2)
{
	color1.w = 1;
	color2.w = 1;

	g_pShaderAPI->DrawSetColor(color1);
	g_pShaderAPI->DrawLine2D(mins,Vector2D(maxs.x,mins.y));
	g_pShaderAPI->DrawSetColor(color2);
	g_pShaderAPI->DrawLine2D(Vector2D(mins.x,maxs.y),maxs);

	g_pShaderAPI->DrawSetColor(color1);
	g_pShaderAPI->DrawLine2D(mins,Vector2D(mins.x,maxs.y));
	g_pShaderAPI->DrawSetColor(color2);
	g_pShaderAPI->DrawLine2D(Vector2D(maxs.x,mins.y),maxs);
}
*/

CEqSysConsole::CEqSysConsole()
{
	// release build needs false
	bShowConsole = false;

	con_full_enabled = false;

	con_cursorTime = 0.0f;
	drawcount = 10;

	con_font = 0;

	m_bshiftHolder = false;
	m_bCtrlHolder = false;

	win_wide = 512;
	win_tall = 512;
	fullscreen = false;

	con_cursorPos = 0;
	con_cursorPos_locked = -1;
	conLinePosition = 0;
	con_histIndex = 0;
	con_valueindex = 0;
	con_fastfind_selection_index = -1;
	con_fastfind_selection_autocompletion_index = -1;
	con_fastfind_selection_autocompletion_val_index = -1;
	con_fastfind_isbeingselected = false;
	con_fastfind_isbeingselected_autoc = false;
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
	if(start + len > con_Text.GetLength())
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

void CEqSysConsole::drawFastFind(float x, float y, float w, IEqFont* fontid)
{
	con_fastfind_isbeingselected = false;
	con_fastfind_selection_index = -1;

	BlendStateParam_t blending;
	blending.srcFactor = BLENDFACTOR_SRC_ALPHA;
	blending.dstFactor = BLENDFACTOR_ONE_MINUS_SRC_ALPHA;

	if(con_Text.GetLength() > 0 && con_fastfind.GetBool())
	{
		const DkList<ConCommandBase*> *base = GetCvars()->GetAllCommands();

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

		ConCommandBase* pCommand = (ConCommandBase*)GetCvars()->FindBase( name.GetData() );

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
			Rectangle_t rect(x,y,w,y+14*(numLines+1));
			DrawAlphaFilledRectangle(rect, s_conBackFastFind, s_conBorderColor);

			fontid->DrawSetColor( s_conHelpTextColor );
			fontid->DrawTextInRect(string_to_draw.GetData(), rect, FONT_DIMS, false);

			// draw autocompletion if available
			commandinfo_size += DrawAutoCompletion(x, y+14*(numLines+1)+4, w, fontid);
			commandinfo_size += numLines+1;
			//return;

			if(enumcount <= 1)
				return;
		}

		if(ff_numelems >= con_fastfind_count.GetInt()-1)
			enumcount = 1;

		if(ff_numelems <= 0)
			return;

		y += 14*(commandinfo_size)+8;

		Rectangle_t rect(x,y,w,y+14*(enumcount+1));
		DrawAlphaFilledRectangle(rect, s_conBackFastFind, s_conBorderColor);

		if(ff_numelems >= con_fastfind_count.GetInt()-1)
		{
			// Set color
			fontid->DrawSetColor( s_conHelpTextColor );

			float textYPos = y+8;

			char *str = varargs("%d commands found (too many to show here)", ff_numelems);

			fontid->DrawText(str, x+5,textYPos, FONT_DIMS, false);

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

				float textYPos = (y + 14*enumcount2) + 4;

				enumcount2++;

				bool bSelected = false;

				int clen = strlen(base->ptr()[i]->GetName());

				if(IsInRectangle(mousePosition.x,mousePosition.y,x,textYPos+2,w-x,12))
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

				// Set color
				fontid->DrawSetColor(selTextColor);

				if(base->ptr()[i]->IsConVar())
				{
					ConVar *cv = (ConVar*)base->ptr()[i];
					char str[256];


					if(bHasAutocompletion)
						sprintf(str,"%s [%s] ->", cv->GetName(), cv->GetString());
					else
						sprintf(str,"%s [%s]",cv->GetName(), cv->GetString());

					fontid->DrawText(str,x+5,textYPos,FONT_DIMS, false);
				}
				else
				{
					if(!bHasAutocompletion)
					{
						fontid->DrawText(varargs("%s ()",base->ptr()[i]->GetName()),x+5,textYPos,FONT_DIMS, false);
					}
					else
					{
						fontid->DrawText(varargs("%s ->",base->ptr()[i]->GetName()),x+5,textYPos,FONT_DIMS, false);
					}
				}

				const char *cstr = xstristr(base->ptr()[i]->GetName(), con_Text.GetData());

				if(cstr)
				{
					int ofs = cstr - base->ptr()[i]->GetName();
					int len = con_Text.GetLength();

					Vertex2D_t rect[] = { MAKETEXQUAD(x+5+ofs*FONT_WIDE, textYPos, x+5+(ofs+len)*FONT_WIDE, textYPos+FONT_TALL, 0) };
					// Cancel textures
					g_pShaderAPI->Reset();

					materials->DrawPrimitives2DFFP(PRIM_TRIANGLE_STRIP,rect,elementsOf(rect), NULL, ColorRGBA(1.0f, 1.0f, 1.0f, 0.3f), &blending);
				}
			}
		}
	}
}

int CEqSysConsole::DrawAutoCompletion(float x, float y, float w, IEqFont* fontid)
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

		const DkList<ConCommandBase*> *base = GetCvars()->GetAllCommands();

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

		enumcount++;

		// draw as autocompletion
		Rectangle_t rect(x,y,x+max_string_length*FONT_WIDE,y+14*enumcount);
		DrawAlphaFilledRectangle(rect, s_conBackFastFind, s_conBorderColor);

		fontid->DrawSetColor(s_conInputTextColor);
		fontid->DrawText("Possible variants: ",x+5,y,FONT_DIMS, false);

		fontid->DrawSetColor(s_conHelpTextColor);
		

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
						float textYPos = (y + 14*enumcount2);

						enumcount2++;

						bool bSelected = false;

						if(IsInRectangle(mousePosition.x,mousePosition.y,x,textYPos+2,(max_string_length*FONT_WIDE)-x,12))
						{
							Vertex2D_t selrect[] = { MAKETEXQUAD(x, textYPos,x+max_string_length*FONT_WIDE, textYPos + 14 , 1) };

							// Cancel textures
							g_pShaderAPI->Reset(STATE_RESET_TEX);

							// Set color
							//g_pShaderAPI->DrawSetColor(1.0f, 1.0f, 1.0f, 0.8f);

							// Set alpha,rasterizer and depth parameters
							//g_pShaderAPI->ChangeBlendingEx(BLENDFACTOR_SRC_ALPHA, BLENDFACTOR_ONE_MINUS_SRC_ALPHA,BLENDFUNC_ADD,false,0.9f);
							//g_pShaderAPI->ChangeRasterStateEx(CULL_NONE,FILL_SOLID);
							//g_pShaderAPI->ChangeDepthStateEx(false,false);

							// Draw the rectangle
							materials->DrawPrimitives2DFFP(PRIM_TRIANGLE_STRIP,selrect,elementsOf(selrect));

							bSelected = true;

							con_fastfind_isbeingselected_autoc = true;
							con_fastfind_selection_autocompletion_index = j;
							con_fastfind_selection_autocompletion_val_index = k;
						}

						Vector4D selTextColor = bSelected ? s_conSelectedTextColor : s_conTextColor;

						fontid->DrawSetColor( selTextColor );
						fontid->DrawText(m_hAutoCompletionNodes[j]->args[k].GetData(),x+5,textYPos,FONT_DIMS, false);
					}
				}
			}
		}
	}

	return enumcount2;
}

void CEqSysConsole::SetLastLine()
{
	conLinePosition = GetAllMessages()->numElem() - drawcount;
}

void CEqSysConsole::AddToLinePos(int num)
{
	conLinePosition += num;
}

void CEqSysConsole::DrawSelf(bool transparent,int width,int height, IEqFont* font, float curTime)
{
	BlendStateParam_t blending;
	blending.srcFactor = BLENDFACTOR_SRC_ALPHA;
	blending.dstFactor = BLENDFACTOR_ONE_MINUS_SRC_ALPHA;

	fullscreen = transparent;
	con_font = font;

	int drawstart = conLinePosition;



	if(conLinePosition > GetAllMessages()->numElem())
		conLinePosition = GetAllMessages()->numElem();

	if(!bShowConsole)
		return;

	drawcount = (int)(height / 16) -2;

	if(drawcount < -4)
		return;

	int drawending = (-drawstart*16) + 16 * (drawcount+drawstart);

	int draws = 0;

	Vector4D color(0.75f, 0.75f, 0.75f, 1);

	static ITexture* pConsoleFrame = g_pShaderAPI->LoadTexture("ui/ui_console", TEXFILTER_TRILINEAR_ANISO,ADDRESSMODE_WRAP, TEXFLAG_NOQUALITYLOD);

	if(!transparent)
	{
		Vertex2D_t tmprect[] = { MAKETEXQUAD(0, 0,(float) width, (float)height, 0) };

		// Cancel textures
		g_pShaderAPI->Reset();

		// Set color
		//g_pShaderAPI->DrawSetColor(color);

		// Set alpha,rasterizer and depth parameters
		materials->SetBlendingStates(BLENDFACTOR_ONE, BLENDFACTOR_ZERO);
		materials->SetRasterizerStates(CULL_FRONT,FILL_SOLID);
		materials->SetDepthStates(false,false);
		g_pShaderAPI->SetTexture( pConsoleFrame );

		materials->DrawPrimitives2DFFP(PRIM_TRIANGLE_STRIP,tmprect,elementsOf(tmprect), pConsoleFrame, color, &blending);

		// render slider
	}

	Rectangle_t frame_rect(64, 26, width-64,46);

	Rectangle_t con_outputRectangle(64,frame_rect.vrightBottom.y+26, width - 64, height-frame_rect.vleftTop.y);

	DrawAlphaFilledRectangle(frame_rect, s_conInputBackColor, s_conBorderColor);

	if(con_full_enabled)
	{
		DrawAlphaFilledRectangle(con_outputRectangle, s_conBackColor, s_conBorderColor);

		int cnumLines = 0;

		int numRenderLines = GetAllMessages()->numElem();

		drawcount = (con_outputRectangle.GetSize().y / 16);

		int numDrawn = 0;

		for(int i = drawstart; i < numRenderLines; i++, numDrawn++)
		{
			font->DrawSetColor(GetAllMessages()->ptr()[i]->color);
			font->DrawTextInRect((char *) (const char *)GetAllMessages()->ptr()[i]->text,con_outputRectangle,FONT_DIMS,false,&cnumLines);

			con_outputRectangle.vleftTop.y += 16*cnumLines;

			if(i-drawstart >= drawcount)
			{
				font->DrawSetColor(ColorRGBA(0.5f,0.5f,1.0f,1.0f));

				font->DrawText("^ ^ ^ ^ ^ ^", con_outputRectangle.vleftTop.x, con_outputRectangle.vrightBottom.y-16, FONT_DIMS, false);

				break;
			}
		}
	}

	drawFastFind( 128,frame_rect.vleftTop.y+25, width-128,font );

	char* con_str = CONSOLE_INPUT_STARTSTR;
	int con_str_len = strlen(con_str);

	EqString conDraw(con_str);
	conDraw.Append(con_Text);

	static bool con_showcursor = false;

	if(con_cursorTime < curTime)
	{
		con_showcursor = !con_showcursor;
		con_cursorTime = curTime + con_cursortime.GetFloat();
	}

	font->DrawSetColor( s_conInputTextColor );
	g_pShaderAPI->Apply();

	font->DrawText(conDraw.GetData(),64,frame_rect.vleftTop.y+4,FONT_DIMS, false);

	float con_strl = font->GetStringLength((char*)con_Text.GetData(),con_cursorPos, false) * FONT_WIDE;
	float con_specLen = font->GetStringLength(con_str,con_str_len, false) * FONT_WIDE;

	if(con_cursorPos_locked != -1)
	{
		float con_selpos = font->GetStringLength((char*)con_Text.GetData(),con_cursorPos_locked, false) * FONT_WIDE;

		Vertex2D_t rect[] = { MAKETEXQUAD(con_specLen + con_selpos + 64, frame_rect.vleftTop.y+4,con_strl + con_specLen + 64, frame_rect.vleftTop.y+17, 0) };
		// Cancel textures
		g_pShaderAPI->Reset();

		materials->DrawPrimitives2DFFP(PRIM_TRIANGLE_STRIP,rect,elementsOf(rect), NULL, ColorRGBA(1.0f, 1.0f, 1.0f, 0.3f), &blending);
	}

	if(con_showcursor)
	{
		float con_selpos = font->GetStringLength((char*)con_Text.GetData(),con_cursorPos, false) * FONT_WIDE;

		Vertex2D_t rect[] = { MAKETEXQUAD(con_specLen + con_selpos + 64, frame_rect.vleftTop.y+4,con_specLen + con_selpos + 64 + 1, frame_rect.vleftTop.y+17, 0) };

		// Cancel textures
		g_pShaderAPI->Reset();

		materials->DrawPrimitives2DFFP(PRIM_TRIANGLE_STRIP,rect,elementsOf(rect), NULL, ColorRGBA(1.0f), &blending);
	}

	font->DrawSetColor(ColorRGBA(1,1,1,0.5f));

	font->DrawTextEx( CONSOLE_ENGINEVERSION_STR, 5, 5, FONT_WIDE, FONT_TALL, TEXT_ORIENT_RIGHT, TEXT_STYLE_SHADOW, false);

}

void CEqSysConsole::MousePos(const Vector2D &pos)
{
	mousePosition = pos;
}

void CEqSysConsole::KeyChar(ubyte ch)
{
	if(bShowConsole)
	{
		// Font is not loaded, skip
		if(con_font == NULL)
			return;

		if(ch == '~')
			return;

		// THis is a weird thing
		if((((CFont*)con_font)->m_FontChars[ch].ratio != 0.0f) && ch != '`')
		{
			con_cursorPos+=1;

			char text[2];
			text[0] = ch;
			text[1] = 0;

			con_Text.Insert( text, con_cursorPos-1);
		}
	}
}

void CEqSysConsole::MouseEvent(const Vector2D &pos, int Button,bool pressed)
{
	if (pressed)
	{
		if (Button == MOU_B1)
		{

			if(bShowConsole)
			{
				if(con_fastfind_isbeingselected && con_fastfind_selection_index != -1)
				{
					const DkList<ConCommandBase*> *base = GetCvars()->GetAllCommands();
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

						con_cursorPos = con_Text.GetLength();
					}
				}

				if(con_fastfind_isbeingselected_autoc)
				{
					if(con_fastfind_selection_autocompletion_index < m_hAutoCompletionNodes.numElem())
					{
						int ac_ind = con_fastfind_selection_autocompletion_index;
						int val_ind = con_fastfind_selection_autocompletion_val_index;

						con_Text = m_hAutoCompletionNodes[ac_ind]->cmd_name + _Es(" \"") + m_hAutoCompletionNodes[ac_ind]->args[val_ind] + _Es("\"");
						con_cursorPos = con_Text.GetLength();
					}
				}
			}
		}
	}
}

bool CEqSysConsole::KeyPress(int key, bool pressed)
{
	if(!bShowConsole)
		return false;

	int action_index = GetKeyBindings()->GetBindingIndexByKey(key);

	if(action_index != -1)
	{
		if(!stricmp(GetKeyBindings()->GetBindingList()->ptr()[action_index]->commandString.GetData(), "toggleconsole"))
		{
			return false;
		}
	}

	if(pressed)
	{
		switch (key)
		{
			case KEY_BACKSPACE:
				if(con_cursorPos_locked != -1 && con_cursorPos_locked != con_cursorPos)
				{
					if(con_cursorPos > con_cursorPos_locked)
					{
						consoleRemTextInRange(con_cursorPos_locked, con_cursorPos - con_cursorPos_locked);
						con_cursorPos = con_cursorPos_locked;
					}
					else
					{
						consoleRemTextInRange(con_cursorPos, con_cursorPos_locked - con_cursorPos);
					}

					con_cursorPos_locked = -1; //con_cursorPos;
					con_histIndex = commandHistory.numElem()-1;
					return true;
				}

				if (con_cursorPos > 0)
				{
					con_cursorPos--;
					consoleRemTextInRange(con_cursorPos, 1);
					con_histIndex = commandHistory.numElem()-1;
				}
				return true;
			case KEY_DELETE:
				if (con_cursorPos <= con_Text.GetLength())
				{
					if(con_cursorPos_locked != -1 && con_cursorPos_locked != con_cursorPos)
					{
						if(con_cursorPos > con_cursorPos_locked)
						{
							consoleRemTextInRange(con_cursorPos_locked, con_cursorPos - con_cursorPos_locked);
							con_cursorPos = con_cursorPos_locked;
						}
						else
						{
							consoleRemTextInRange(con_cursorPos, con_cursorPos_locked - con_cursorPos);
						}

						con_cursorPos_locked = con_cursorPos;
						con_histIndex = commandHistory.numElem()-1;
						return true;
					}
					consoleRemTextInRange(con_cursorPos, 1);
					con_histIndex = commandHistory.numElem()-1;
				}
				return true;
			case KEY_SHIFT:
				if(!m_bshiftHolder)
				{
					con_cursorPos_locked = con_cursorPos;
					m_bshiftHolder = true;
				}
				return true;
			case KEY_CTRL:
				if(!m_bCtrlHolder)
				{
					m_bCtrlHolder = true;
				}
				return true;
			case KEY_HOME:
				conLinePosition = 0;
				return true;
			case KEY_END:
				conLinePosition = clamp(GetAllMessages()->numElem() - drawcount, 0, GetAllMessages()->numElem()-1);
				return true;
			case KEY_A:
				if(m_bCtrlHolder)
				{
					con_cursorPos = con_Text.GetLength();
					con_cursorPos_locked = 0;
				}
			case KEY_C:
				if(m_bCtrlHolder)
				{
					if(con_cursorPos_locked != -1 && con_cursorPos_locked != con_cursorPos)
					{
						bool bInverseSelection = con_cursorPos > con_cursorPos_locked;
						int cpystartpos = bInverseSelection ? con_cursorPos_locked : con_cursorPos;
						int cpylength = bInverseSelection ? (con_cursorPos - con_cursorPos_locked) : (con_cursorPos_locked - con_cursorPos);

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
							xstrcpy(chBuffer, tmpString.GetData());
							GlobalUnlock(hgBuffer);
							SetClipboardData(CF_TEXT,hgBuffer);
							CloseClipboard();
						}
#endif // PLAT_SDL
					}
				}
				return true;
			case KEY_X:
				if(m_bCtrlHolder)
				{
					if(con_cursorPos_locked != -1 && con_cursorPos_locked != con_cursorPos)
					{
						bool bInverseSelection = con_cursorPos > con_cursorPos_locked;
						int cpystartpos = bInverseSelection ? con_cursorPos_locked : con_cursorPos;
						int cpylength = bInverseSelection ? (con_cursorPos - con_cursorPos_locked) : (con_cursorPos_locked - con_cursorPos);

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
							xstrcpy(chBuffer, tmpString.GetData());
							GlobalUnlock(hgBuffer);
							SetClipboardData(CF_TEXT,hgBuffer);
							CloseClipboard();
						}
#endif // PLAT_SDL

						if(con_cursorPos > con_cursorPos_locked)
						{
							consoleRemTextInRange(con_cursorPos_locked, con_cursorPos - con_cursorPos_locked);
							con_cursorPos = con_cursorPos_locked;
						}
						else
						{
							consoleRemTextInRange(con_cursorPos, con_cursorPos_locked - con_cursorPos);
						}

						con_cursorPos_locked = -1;
					}
				}
				return true;
			case KEY_V:
				if(m_bCtrlHolder)
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

						if(con_cursorPos_locked != -1)
						{
							if(con_cursorPos > con_cursorPos_locked)
							{
								consoleRemTextInRange(con_cursorPos_locked, con_cursorPos - con_cursorPos_locked);
								con_cursorPos = con_cursorPos_locked;
							}
							else
							{
								consoleRemTextInRange(con_cursorPos, con_cursorPos_locked - con_cursorPos);
							}
						}

						consoleInsText((char*)tmpString.GetData(),con_cursorPos);
						con_cursorPos += tmpString.GetLength();
					}
				}
				return true;
			case KEY_ENTER:
				if (con_Text.GetLength() > 0)
				{
					MsgInfo("> %s\n",con_Text.GetData());

					GetCommandAccessor()->ResetCounter();
					GetCommandAccessor()->SetCommandBuffer((char*)con_Text.GetData());

					bool stat = GetCommandAccessor()->ExecuteCommandBuffer();
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
				con_cursorPos = 0;
				return true;

			case KEY_TAB:
				{
					const DkList<ConCommandBase*> *base = GetCvars()->GetAllCommands();

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
						con_cursorPos = con_Text.GetLength();
					}
					else if(max_match_chars != 0)
					{
						con_Text = matching_str;
						con_cursorPos = con_Text.GetLength();
					}

				}
				return true;

			case KEY_PGUP:
				if(conLinePosition > 0)
					conLinePosition--;
				return true;
			case KEY_PGDN:
				//int drawcount;

				if(fullscreen)
					drawcount = ((win_tall) / 22) -2;
				else
					drawcount = ((win_tall*2) / 22) -2;

				if(conLinePosition < GetAllMessages()->numElem())
				{
					conLinePosition++;
				}
				return true;

			case KEY_LEFT:
				if (con_cursorPos > 0)
				{
					con_cursorPos--;

					if(!m_bshiftHolder)
						con_cursorPos_locked = -1;
				}
				return true;
			case KEY_TILDE:
				if(!fullscreen)
					return true;

				con_histIndex = commandHistory.numElem()-1;

				//bShowConsole = !bShowConsole;
				return true;
			case KEY_RIGHT:
				if (con_cursorPos < con_Text.GetLength())
				{
					con_cursorPos++;

					if(!m_bshiftHolder)
						con_cursorPos_locked = -1;
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
					con_cursorPos = commandHistory[con_histIndex].GetLength();
					return true;
				}

				con_histIndex++;

				if(con_histIndex < 0) con_histIndex = 0;
				if(con_histIndex > commandHistory.numElem()-1) con_histIndex = commandHistory.numElem()-1;

				con_Text = commandHistory[con_histIndex];
				con_cursorPos = commandHistory[con_histIndex].GetLength();

				return true;
			case KEY_UP:
				if(!commandHistory.numElem())
					return true;

				if(con_Text.GetLength() == 0)
				{
					// display last
					con_histIndex = commandHistory.numElem()-1;

					con_Text = commandHistory[con_histIndex];
					con_cursorPos = commandHistory[con_histIndex].GetLength();
					return true;
				}

				con_histIndex--;

				if(con_histIndex < 0) con_histIndex = 0;
				if(con_histIndex > commandHistory.numElem()-1) con_histIndex = commandHistory.numElem()-1;

				con_Text = commandHistory[con_histIndex];
				con_cursorPos = commandHistory[con_histIndex].GetLength();

				return true;
			default:
				con_histIndex = commandHistory.numElem()-1;
				con_cursorPos_locked = -1;
				return true;
		}
	}
	else
	{
		switch (key)
		{
			case KEY_SHIFT:
				m_bshiftHolder = false;
				break;
			case KEY_CTRL:
				m_bCtrlHolder = false;
		}
	}

	return true;
}
