#pragma once

class CMoviePlayer;
class CLuaMoviePlayer;

EQSCRIPT_BIND_TYPE_NO_PARENT(CMoviePlayer, "CMoviePlayer", BY_REF)
EQSCRIPT_BIND_TYPE_WITH_PARENT(CLuaMoviePlayer, CMoviePlayer, "CLuaMoviePlayer")

bool eslSysMoviePlayerInit(const esl::ScriptState& state);