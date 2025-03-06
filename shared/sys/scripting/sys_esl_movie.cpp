#include "core/core_common.h"
#include "movie/MoviePlayer.h"
#include "sys/sys_state.h"

#include "sys_esl.h"
#include "sys_esl_movie.h"

class CLuaMoviePlayer : public CMoviePlayer
{
public:
	CLuaMoviePlayer()
	{
		InitLoopCb();
	}

	CLuaMoviePlayer(const char* aliasName) 
		: CMoviePlayer(aliasName)
	{
		InitLoopCb();
	}

	void InitLoopCb()
	{
		postUpdateSub = eqAppStateMng::g_onPostUpdateState += [this](float fDt) {
			Present();
		};

		OnCompleted += [this]() {
			if (m_loop)
				Rewind();
		};
	}

	void SetLoop(bool enable) { m_loop = enable; }
	bool IsLooped() const { return m_loop; }

protected:
	StatePostUpdateEvent::Sub postUpdateSub;
	bool	m_loop{ false };
};

EQSCRIPT_TYPE_BEGIN(CMoviePlayer)
	EQSCRIPT_BIND_FUNC(Init)
	EQSCRIPT_BIND_FUNC(Destroy)
	
	EQSCRIPT_BIND_FUNC(Start)
	EQSCRIPT_BIND_FUNC(Stop)
	EQSCRIPT_BIND_FUNC(Rewind)
	
	EQSCRIPT_BIND_FUNC(IsPlaying)
	EQSCRIPT_BIND_FUNC(SetTimeScale)
EQSCRIPT_TYPE_END

EQSCRIPT_TYPE_BEGIN(CLuaMoviePlayer)
	EQSCRIPT_BIND_CONSTRUCTOR()
	EQSCRIPT_BIND_CONSTRUCTOR(const char*)

	EQSCRIPT_BIND_FUNC(SetLoop)
	EQSCRIPT_BIND_FUNC(IsLooped)
EQSCRIPT_TYPE_END

bool eslSysMoviePlayerInit(const esl::ScriptState& state)
{
	state.RegisterClass<CMoviePlayer>();
    state.RegisterClass<CLuaMoviePlayer>();
	return true;
}
