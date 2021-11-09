//////////////////////////////////////////////////////////////////////////////////
// Copyright Š Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Core Debug interface
//////////////////////////////////////////////////////////////////////////////////

#include "core/DebugInterface.h"
#include "core/platform/Platform.h"
#include "core/core_base_header.h"
#include "resource.h"
#include "utils/eqthread.h"
#include <stdarg.h>
#include "utils/SmartPtr.h"

#ifdef _WIN32
#include <direct.h>
#endif

#ifdef ANDROID
#include <android/log.h>
#define EQENGINE_LOG_TAG(v) EqString::Format("%s %s", GetCore()->GetApplicationName(), v).ToCString()
#endif // ANDROID

static const char* s_spewTypeStr[] = {
	"",
	"[INFO]",
	"[WARN]",
	"[ERR]",
	"",
};

Threading::CEqMutex g_debugOutputMutex;
bool g_bLoggingInitialized = false;
FILE* g_logFile = NULL;

bool g_logContinue = false;

bool g_logForceFlush = true;

void Log_WriteBOM(const char* fileName);

// initializes logging to the disk
void Log_Init()
{
	if(g_logFile)
		return;

#ifdef PLAT_POSIX
	mkdir("logs",S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
#else
	mkdir("logs");
#endif // PLAT_POSIX

#ifndef NO_CORE
	char tmp_path[2048];
	sprintf(tmp_path, "logs/%s.log",GetCore()->GetApplicationName());

	Log_WriteBOM( tmp_path );

	g_logFile = fopen(tmp_path, g_logContinue ? "a" : "w");
#else
	Log_WriteBOM( "logs/dkcore.log" );

	g_logFile = fopen("logs/dkcore.log", g_logContinue ? "a" : "w");
#endif

	if(g_logFile)
		g_logContinue = true;
}

// flushes the console log to the disk
void Log_Flush()
{
	if(!g_logFile)
		return;

	fflush(g_logFile);
}

// closes the log file
void Log_Close()
{
	if(!g_logFile)
		return;

	fclose(g_logFile);
	g_logFile = NULL;
}

int g_developerMode = 0;

DECLARE_CONCOMMAND_FN(developer)
{
	int newMode = 0;

	for(int i = 0; i < CMD_ARGC; i++)
	{
		EqString& str = CMD_ARGV(i);

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
			newMode = atoi( str.ToCString() );
	}

	if( newMode > 0 )
        g_developerMode = newMode;
    else if(CMD_ARGC == 0 || newMode == 0)
	{
		Msg("Usage: developer <string1> [string2] ...");
		Msg("supported modes:\n");
		Msg("\tcore\n");
		Msg("\tlocale\n");
		Msg("\tfilesys\n");
		Msg("\tmatsys\n");
		Msg("\trenderer\n");
		Msg("\tsound\n");
		Msg("\tnetwork\n");
		Msg("\tgame\n");
		Msg("\tall\n");
		Msg("use 'developer disable' to turn off\n");
		return;
	}
}

ConCommand c_developer("developer",CONCOMMAND_FN(developer),"Sets developer modes",CV_CHEAT | CV_UNREGISTERED);

// Default spew
void DefaultSpewFunc(SpewType_t type,const char* pMsg)
{
	puts( pMsg );
}

//Spew callback
static SpewFunc_fn g_fnConSpewFunc = DefaultSpewFunc;

IEXPORTS void SetSpewFunction(SpewFunc_fn newfunc)
{
	if(newfunc == nullptr)
		g_fnConSpewFunc = DefaultSpewFunc;
	else
		g_fnConSpewFunc = newfunc;
}

DECLARE_CONCOMMAND_FN(echo)
{
	if(CMD_ARGC == 0)
	{
		MsgWarning("Example: echo <text> [additional text]\n");
		return;
	}

	EqString outText;

	for(int i = 0; i < CMD_ARGC; i++)
	{
		char tmp_path[2048];
		sprintf(tmp_path, "%s ",CMD_ARGV(i).ToCString());

		outText.Append(tmp_path);
	}

	Msg("%s\n", outText.GetData());
}
ConCommand c_echo("echo",CONCOMMAND_FN(echo),"Displays the entered args",CV_UNREGISTERED);

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

#define DEBUGMESSAGE_BUFFER_SIZE 2048

void SpewMessage(SpewType_t spewtype, char const* msg)
{
#ifdef ANDROID
	const char* logTag = EQENGINE_LOG_TAG(s_spewTypeStr[spewtype]);

	// force log into android debug output
	__android_log_print(ANDROID_LOG_DEBUG, logTag, "%s", msg);
#else

#endif // ANDROID

	{
		Threading::CScopedMutex m(g_debugOutputMutex);

		if (!g_bLoggingInitialized)
		{
			puts(msg);
		}

		// print to log file if enabled
		if (g_logFile)
		{
			fputs(msg, g_logFile);

			if (g_logForceFlush)
				Log_Flush();
		}

		ASSERT(g_fnConSpewFunc);
		(g_fnConSpewFunc)(spewtype, msg);
	}
}

void SpewMessageToOutput(SpewType_t spewtype,char const* pMsgFormat, va_list args)
{
	char pTempBuffer[DEBUGMESSAGE_BUFFER_SIZE];

	int len = vsnprintf(pTempBuffer, DEBUGMESSAGE_BUFFER_SIZE, pMsgFormat, args );

	if (len >= 2048)
	{
		S_NEWA(tempBufferExt, char, len + 1);
		len = vsnprintf(tempBufferExt, len + 1, pMsgFormat, args);

		SpewMessage(spewtype, tempBufferExt);
	}
	else
	{
		SpewMessage(spewtype, pTempBuffer);
	}
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
