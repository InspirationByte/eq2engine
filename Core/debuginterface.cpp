//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Core Debug interface - DarkTech Core Implementation
//////////////////////////////////////////////////////////////////////////////////

#include "DebugInterface.h"
#include "core_base_header.h"

#include "Platform.h"
#include "resource.h"
#include <stdarg.h>
#include "utils/eqthread.h"

Threading::CEqMutex g_debugOutputMutex;

//#define DEBUG_NO_OUTPUT

extern bool bDoLogs;
extern bool bLoggingInitialized;

ConVar developer("developer","0","Enable developer messages (2 is verbose)",CV_CHEAT);

// Default spew
void DefaultSpewFunc(SpewType_t type,const char* pMsg)
{
	printf( "%s", pMsg );
}

//Spew callback
static SpewFunc_fn g_fnConSpewFunc = DefaultSpewFunc;

IEXPORTS void SetSpewFunction(SpewFunc_fn newfunc)
{
	g_fnConSpewFunc = newfunc;
}

//DECLARE_CMD_NONSTATIC(echo,Displays the entered args,0)
void cc_echo_f(DkList<EqString>* args)
{
	if(args->numElem() == 0)
	{
		MsgWarning("Example: echo <text> [additional text]\n");
		return;
	}

	EqString outText;

	for(int i = 0; i < args->numElem(); i++)
	{
		char tmp_path[2048];
		sprintf(tmp_path, "%s ",args->ptr()[i].GetData());

		outText.Append(tmp_path);
	}

	Msg("%s\n", outText.GetData());
}
ConCommand c_echo("echo",cc_echo_f,"Displays the entered args",0);

#ifndef DEBUG_NO_OUTPUT

#pragma warning(push)
#pragma warning(disable:4309)

void Log_WriteBOM(const char* fileName)
{
	// don't add if file is exist
	if( GetFileSystem()->FileExist(fileName) )
		return;

	char bom[3] = {0xEF,0xBB,0xBF};
		
	FILE* pFile = fopen( fileName, "w" );
	if(pFile)
	{
		fwrite(bom, 3, 1, pFile);

		fclose(pFile);
	}
}

#pragma warning(pop)

void SpewMessageToOutput(SpewType_t spewtype,char const* pMsgFormat, va_list args)
{
	Threading::CScopedMutex m(g_debugOutputMutex);

	char pTempBuffer[2048];
	int len = 0;

	/* Create the message.... */
	len += vsprintf( &pTempBuffer[len], pMsgFormat, args );

	ASSERT( len < 2048 );
	ASSERT( g_fnConSpewFunc );

	if(!bLoggingInitialized)
	{
		switch(spewtype)
		{
			case SPEW_NORM:
			case SPEW_SUCCESS:
			case SPEW_INFO:
				InfoMsg(pTempBuffer);
				break;
			case SPEW_WARNING:
				WarningMsg(pTempBuffer);
				break;
			case SPEW_ERROR:
				ErrorMsg(pTempBuffer);
				break;
		}
		return;
	}

	if(bDoLogs)
	{
#ifndef NO_CORE
		char tmp_path[2048];
		sprintf(tmp_path, "logs/%s_%s.log",GetCore()->GetApplicationName(),GetCore()->GetCurrentUserName());

		Log_WriteBOM( tmp_path );

		FILE* g_logFile = fopen(tmp_path, "a");
#else
		Log_WriteBOM( "logs/dkcore.log" );

		FILE* g_logFile = fopen("logs/dkcore.log", "a");
#endif
		if(g_logFile)
		{
			fprintf(g_logFile, "%s", pTempBuffer);
			fclose(g_logFile);
		}
	}

	(g_fnConSpewFunc)(spewtype,pTempBuffer);
}


// Simple messages
IEXPORTS void Msg(const char *fmt,...)
{
	va_list		argptr;

	va_start (argptr,fmt);
	SpewMessageToOutput(SPEW_NORM,fmt,argptr);
	va_end (argptr);
}

// Error messages
IEXPORTS void MsgError(const char *fmt,...)
{
	va_list		argptr;

	va_start (argptr,fmt);
	SpewMessageToOutput(SPEW_ERROR,fmt,argptr);
	va_end (argptr);
}

// Info messages
IEXPORTS void MsgInfo(const char *fmt,...)
{
	va_list		argptr;

	va_start (argptr,fmt);
	SpewMessageToOutput(SPEW_INFO,fmt,argptr);
	va_end (argptr);
}

//Warning messages
IEXPORTS void MsgWarning(const char *fmt,...)
{
	va_list		argptr;

	va_start (argptr,fmt);
	SpewMessageToOutput(SPEW_WARNING,fmt,argptr);
	va_end (argptr);
}

// Good messages
IEXPORTS void MsgAccept(const char *fmt,...)
{
	va_list		argptr;

	va_start (argptr,fmt);
	SpewMessageToOutput(SPEW_SUCCESS,fmt,argptr);
	va_end (argptr);
}

// Developer messages
IEXPORTS void DevMsg(int level,const char *fmt,...)
{
	// Don't print messages that lower than developer level
	if(developer.GetInt() != 6)
	{
		if(developer.GetInt() != level)
			return;
	}

	va_list argptr;

	va_start (argptr,fmt);
	SpewMessageToOutput(SPEW_WARNING,fmt,argptr);
	va_end (argptr);
}
#else

IEXPORTS void Msg(char *fmt,...) {}
IEXPORTS void MsgInfo(char *fmt,...) {}
IEXPORTS void MsgWarning(char *fmt,...) {}
IEXPORTS void MsgError(char *fmt,...) {}
IEXPORTS void MsgAccept(char *fmt,...) {}

#endif
