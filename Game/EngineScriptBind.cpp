//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Script library registrator
//////////////////////////////////////////////////////////////////////////////////

#include "EqGMS.h"

#include "BaseEngineHeader.h"

//----------------------------------------------------------------------------------------
// Console
//----------------------------------------------------------------------------------------

EQGMS_DEFINE_LIBFUNC(console, Msg)
{
	GM_CHECK_NUM_PARAMS(1);
	GM_CHECK_STRING_PARAM(a_string, 0);

	Msg(a_string);

	return GM_OK;
}

EQGMS_DEFINE_LIBFUNC(console, MsgAccept)
{
	GM_CHECK_NUM_PARAMS(1);
	GM_CHECK_STRING_PARAM(a_string, 0);

	MsgAccept(a_string);

	return GM_OK;
}

EQGMS_DEFINE_LIBFUNC(console, MsgInfo)
{
	GM_CHECK_NUM_PARAMS(1);
	GM_CHECK_STRING_PARAM(a_string, 0);

	MsgAccept(a_string);

	return GM_OK;
}

EQGMS_DEFINE_LIBFUNC(console, MsgWarning)
{
	GM_CHECK_NUM_PARAMS(1);
	GM_CHECK_STRING_PARAM(a_string, 0);

	MsgInfo(a_string);

	return GM_OK;
}

EQGMS_DEFINE_LIBFUNC(console, MsgError)
{
	GM_CHECK_NUM_PARAMS(1);
	GM_CHECK_STRING_PARAM(a_string, 0);

	MsgError(a_string);

	return GM_OK;
}

EQGMS_DEFINE_LIBFUNC(console,DevMsg)
{
	GM_CHECK_NUM_PARAMS(2);
	GM_CHECK_INT_PARAM(a_devmode, 0)
	GM_CHECK_STRING_PARAM(a_string, 1);

	DevMsg(a_devmode, a_string);

	return GM_OK;
}

EQGMS_DEFINE_LIBFUNC(console, Command)
{
	GM_CHECK_NUM_PARAMS(1);
	GM_CHECK_STRING_PARAM(a_string, 0);

	g_sysConsole->SetCommandBuffer(a_string);
	g_sysConsole->ExecuteCommandBuffer();
	g_sysConsole->ClearCommandBuffer();

	return GM_OK;
}

//----------------------------------------------------------------------------------------
// Engine host
//----------------------------------------------------------------------------------------

EQGMS_DEFINE_LIBFUNC(enginehost, GetTime)
{
	float fTime = g_pEngineHost->GetCurTime();

	GM_THREAD_ARG->PushFloat(fTime);

	return GM_OK;
}

EQGMS_DEFINE_LIBFUNC(engine, ShowCursor)
{
	GM_CHECK_NUM_PARAMS(1);
	GM_CHECK_INT_PARAM(show, 0);

	g_pEngineHost->SetCursorShow(show);

	return GM_OK;
}

EQGMS_DEFINE_LIBFUNC(engine, SetCursorPosition)
{
	GM_CHECK_NUM_PARAMS(2);
	GM_CHECK_FLOAT_OR_INT_PARAM(x, 0);
	GM_CHECK_FLOAT_OR_INT_PARAM(y, 1);

	g_pEngineHost->SetCursorPosition(IVector2D(x,y));

	return GM_OK;
}

EQGMS_DEFINE_LIBFUNC(engine, SetCenterCursor)
{
	GM_CHECK_NUM_PARAMS(1);
	GM_CHECK_INT_PARAM(center, 0);

	engine->SetCenterMouseCursor(center);
	

	return GM_OK;
}

EQGMS_DEFINE_LIBFUNC(engine, GetFrameTime)
{
	float fTime = engine->GetFrameTime();

	GM_THREAD_ARG->PushFloat(fTime);

	return GM_OK;
}

EQGMS_DEFINE_LIBFUNC(engine, LoadWorld)
{
	GM_CHECK_NUM_PARAMS(1);
	GM_CHECK_STRING_PARAM(a_string, 0);

	engine->SetLevelName(a_string);
	engine->SetGameState( IEngineGame::GAME_LEVEL_LOAD );

	return GM_OK;
}

EQGMS_DEFINE_LIBFUNC(game, CurTime)
{
	GM_THREAD_ARG->PushFloat(gpGlobals->curtime);

	return GM_OK;
}

EQGMS_DEFINE_LIBFUNC(game, FrameTime)
{
	GM_THREAD_ARG->PushFloat(gpGlobals->frametime);

	return GM_OK;
}

EQGMS_DEFINE_LIBFUNC(game, RealTime)
{
	GM_THREAD_ARG->PushFloat(gpGlobals->realtime);

	return GM_OK;
}

//----------------------------------------------------------------------------------------
// debug overlay
//----------------------------------------------------------------------------------------

EQGMS_DEFINE_LIBFUNC(debug, Text)
{
	GM_CHECK_NUM_PARAMS(4);

	GM_CHECK_STRING_PARAM(text, 0);
	GM_CHECK_FLOAT_OR_INT_PARAM(color_r, 1);
	GM_CHECK_FLOAT_OR_INT_PARAM(color_g, 2);
	GM_CHECK_FLOAT_OR_INT_PARAM(color_b, 3);

	debugoverlay->Text(ColorRGBA(color_r,color_g,color_b, 1.0f),text);

	return GM_OK;
}

EQGMS_DEFINE_GLOBALFUNC(PrecacheStudioModel)
{
	GM_CHECK_NUM_PARAMS(1);
	GM_CHECK_STRING_PARAM(a_string, 0);

	PrecacheStudioModel(a_string);

	return GM_OK;
}

EQGMS_DEFINE_GLOBALFUNC(PracacheScriptSound)
{
	GM_CHECK_NUM_PARAMS(1);
	GM_CHECK_STRING_PARAM(a_string, 0);

	PrecacheScriptSound(a_string);

	return GM_OK;
}