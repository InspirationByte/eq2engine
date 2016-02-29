//////////////////////////////////////////////////////////////////////////////////
// Copyright Š Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Core Debug interface - DarkTech Core Implementation
//////////////////////////////////////////////////////////////////////////////////

#include "DebugInterface.h"
#include "core_base_header.h"

#include "platform/Platform.h"
#include "resource.h"
#include <stdarg.h>
#include "utils/eqthread.h"

#ifdef ANDROID
#include <android/log.h>
#define EQENGINE_LOG_TAG(v) varargs("%s %s", GetCore()->GetApplicationName(), v)
#endif // ANDROID

static const char* s_spewTypeStr[] = {
	"",
	"[INFO]",
	"[WARN]",
	"[ERR]",
	"",
};

Threading::CEqMutex g_debugOutputMutex;

//#define DEBUG_NO_OUTPUT

extern bool bDoLogs;
extern bool bLoggingInitialized;

int g_developerMode = 0;

void cc_developer_f( DkList<EqString>* args )
{
	if(CMD_ARGC == 0)
	{
		Msg("Usage: developer <string1> [string2] ...");
		Msg("supported modes:\n");
		Msg("\tcore\n");
		Msg("\tlocale\n");
		Msg("\tfilesys\n");
		Msg("\tmatsys\n");
		Msg("\renderert\n");
		Msg("\tsound\n");
		Msg("\tnetwork\n");
		Msg("\tall\n");
		Msg("use 'developer disable' to turn off\n");
		return;
	}

	int newMode = 0;

	for(int i = 0; i < args->numElem(); i++)
	{
		EqString& str = args->ptr()[i];

		if( !str.CompareCaseIns("disable"))
		{
			g_developerMode = 0;
			break;
		}
		else if( !str.CompareCaseIns("all") )
			newMode = DEVMSG_ALL;
		else if( !str.CompareCaseIns("core") )
			newMode |= DEVMSG_CORE;
		else if( !str.CompareCaseIns("locale") )
			newMode |= DEVMSG_LOCALE;
		else if( !str.CompareCaseIns("filesys") )
			newMode |= DEVMSG_FS;
		else if( !str.CompareCaseIns("matsys") )
			newMode |= DEVMSG_MATSYSTEM;
		else if( !str.CompareCaseIns("renderer") )
			newMode |= DEVMSG_SHADERAPI;
		else if( !str.CompareCaseIns("sound") )
			newMode |= DEVMSG_SOUND;
		else if( !str.CompareCaseIns("network") )
			newMode |= DEVMSG_NETWORK;
		else if( !str.CompareCaseIns("game") )
			newMode |= DEVMSG_GAME;
		else
			newMode = atoi( str.c_str() );
	}

	if( newMode > 0 )
		g_developerMode = newMode;
}

ConCommand c_developer("developer",cc_developer_f,"Sets developer modes",CV_CHEAT | CV_UNREGISTERED);

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
ConCommand c_echo("echo",cc_echo_f,"Displays the entered args",CV_UNREGISTERED);

#ifndef DEBUG_NO_OUTPUT

#pragma warning(push)
#pragma warning(disable:4309)

void Log_WriteBOM(const char* fileName)
{
	// don't add if file is exist
	if( g_fileSystem->FileExist(fileName) )
		return;

	char bom[3] = {(char)0xEF,(char)0xBB,(char)0xBF};

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

#ifdef ANDROID
	const char* logTag = EQENGINE_LOG_TAG(s_spewTypeStr[spewtype]);

	// force log into android debug output
	__android_log_print(ANDROID_LOG_DEBUG, logTag, "%s", pTempBuffer);
#else

#endif // ANDROID

	if(!bLoggingInitialized)
	{
		printf( "%s", pTempBuffer );
	/*
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
		*/
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
IEXPORTS void DevMsg(int level, const char *fmt,...)
{
	// Don't print messages that lower than developer level
	if( !(level & g_developerMode) )
		return;

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
