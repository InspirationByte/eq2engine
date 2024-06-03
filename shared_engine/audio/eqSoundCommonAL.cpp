#include <AL/al.h>
#include <AL/alc.h>
#include <AL/efx.h>

#include "core/core_common.h"
#include "core/ConVar.h"

#include "eqSoundCommonAL.h"

DECLARE_CVAR(al_report_errors, "0", nullptr, 0);
DECLARE_CVAR(al_break_on_error, "0", nullptr, 0);
DECLARE_CVAR(al_bypass_errors, "0", nullptr, 0);

static ALExtFuncs alExt;
ALExtFuncs& GetAlExt()
{
	return alExt;
}

bool ALCheckError(const char* op, ...)
{
	const int lastError = alGetError();
	if (lastError != AL_NO_ERROR)
	{
		EqString errString = EqString::Format("code %x", lastError);
		switch (lastError)
		{
		case AL_INVALID_NAME:
			errString = "AL_INVALID_NAME";
			break;
		case AL_INVALID_ENUM:
			errString = "AL_INVALID_ENUM";
			break;
		case AL_INVALID_VALUE:
			errString = "AL_INVALID_VALUE";
			break;
		case AL_INVALID_OPERATION:
			errString = "AL_INVALID_OPERATION";
			break;
		case AL_OUT_OF_MEMORY:
			errString = "AL_OUT_OF_MEMORY";
			break;
		}

		va_list argptr;
		va_start(argptr, op);
		EqString errorMsg = EqString::FormatV(op, argptr);
		va_end(argptr);

		if (al_break_on_error.GetBool())
		{
			_DEBUG_BREAK;
		}

		if (al_report_errors.GetBool())
			MsgError("*OpenAL* error occured while '%s' (%s)\n", errorMsg.ToCString(), errString.ToCString());

		return al_bypass_errors.GetBool();
	}

	return true;
}


//---------------------------------------------------------
// AL COMMON

const char* GetALCErrorString(int err)
{
	switch (err)
	{
	case ALC_NO_ERROR:
		return "AL_NO_ERROR";
	case ALC_INVALID_DEVICE:
		return "ALC_INVALID_DEVICE";
	case ALC_INVALID_CONTEXT:
		return "ALC_INVALID_CONTEXT";
	case ALC_INVALID_ENUM:
		return "ALC_INVALID_ENUM";
	case ALC_INVALID_VALUE:
		return "ALC_INVALID_VALUE";
	case ALC_OUT_OF_MEMORY:
		return "ALC_OUT_OF_MEMORY";
	default:
		return "AL_UNKNOWN";
	}
}

const char* GetALErrorString(int err)
{
	switch (err)
	{
	case AL_NO_ERROR:
		return "AL_NO_ERROR";
	case AL_INVALID_NAME:
		return "AL_INVALID_NAME";
	case AL_INVALID_ENUM:
		return "AL_INVALID_ENUM";
	case AL_INVALID_VALUE:
		return "AL_INVALID_VALUE";
	case AL_INVALID_OPERATION:
		return "AL_INVALID_OPERATION";
	case AL_OUT_OF_MEMORY:
		return "AL_OUT_OF_MEMORY";
	default:
		return "AL_UNKNOWN";
	}
}

bool ALCheckDeviceForErrors(ALCdevice* dev, const char* stage)
{
	ALCenum alErr = alcGetError(dev);
	if (alErr != AL_NO_ERROR)
	{
		MsgError("%s error: %s\n", stage, GetALCErrorString(alErr));
		return false;
	}
	return true;
}