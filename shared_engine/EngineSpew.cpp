//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: DakrTech Engine.dll spew to console functions
//////////////////////////////////////////////////////////////////////////////////

#include "EngineSpew.h"
#include "DebugInterface.h"
#include "IDebugOverlay.h"

#include "utils/DkLinkedList.h"

#define DEFAULT_MAX_CONSOLE_LINES 256
int g_maxConsoleLines = DEFAULT_MAX_CONSOLE_LINES;

static DkList<connode_t*> s_Messages;
DkList<connode_t*> *s_pMessages = &s_Messages;

DkList<connode_t*> *GetAllMessages( void ) {return s_pMessages;}

DECLARE_CMD(clear,NULL,0)
{
	EngineSpewClear();
}

static ConVar con_minicon("con_minicon","0",NULL,CV_ARCHIVE);
static ConVar con_minicon_time("con_minicon_time","0.25f",NULL,CV_ARCHIVE);

ColorRGBA console_spew_colors[] =
{
	Vector4D(1,1,1,1),
	Vector4D(0.8f,0.8f,0.8f,1),
	Vector4D(1,1,0,1),
	Vector4D(0.8f,0,0,1),
	Vector4D(0.2f,1,0.2f,1)
};

void EngineSpewClear()
{
	for(int i = 0;i < s_Messages.numElem();i++)
	{
		delete [] s_Messages[i]->text;
		delete s_Messages[i];
	}

	s_Messages.clear();
}

void EngineSpewFunc(SpewType_t type,const char* pMsg)
{
	printf("%s", pMsg );

#ifdef _WIN32
	OutputDebugString(pMsg);
#endif // _WIN32

	DkList<EqString> temp;
	xstrsplit(pMsg,"\n",temp);

	int nMsgLen = strlen(pMsg);

	//console->SetLastLine();
	//console->AddToLinePos(temp.numElem());

	for(int i = 0; i < temp.numElem(); i++)
	{
		int len = strlen(temp[i].GetData())+1;

		connode_t *pNode = new connode_t;

		pNode->text = new char[len];
		memcpy(pNode->text,temp[i].GetData(),len);

		pNode->color = console_spew_colors[type];

		if(con_minicon.GetBool())
			debugoverlay->TextFadeOut(0, pNode->color,con_minicon_time.GetFloat(), pNode->text);

		GetAllMessages()->append(pNode);
	}
}

void InstallEngineSpewFunction()
{
	kvkeybase_t* consoleSettings = GetCore()->GetConfig()->FindKeyBase("Console");
	g_maxConsoleLines = KV_GetValueInt(consoleSettings ? consoleSettings->FindKeyBase("MaxLines") : NULL, 0, DEFAULT_MAX_CONSOLE_LINES);

	// init list

	SetSpewFunction(EngineSpewFunc);
}

void UninstallEngineSpewFunction()
{
	EngineSpewClear();
	SetSpewFunction(NULL);
}