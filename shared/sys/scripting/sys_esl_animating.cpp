#include "core/core_common.h"
#include "sys_esl.h"
#include "sys_esl_animating.h"

#include "animating/Animating.h"

ESL_ENUM(Activity)

EQSCRIPT_TYPE_BEGIN(CAnimatingEGF)
	EQSCRIPT_BIND_FUNC(FindBone)

	EQSCRIPT_BIND_FUNC(GetLocalBoneOrigin)
	EQSCRIPT_BIND_FUNC(GetLocalBoneDirection)

	//EQSCRIPT_BIND_FUNC(GetLocalStudioTransformMatrix)

	EQSCRIPT_BIND_FUNC(SetActivity)
	EQSCRIPT_BIND_FUNC(GetCurrentActivity)

	EQSCRIPT_BIND_FUNC(FindSequence)
	EQSCRIPT_BIND_FUNC(FindSequenceByActivity)
	EQSCRIPT_BIND_FUNC(SetSequence)
	EQSCRIPT_BIND_FUNC(SetSequenceByName)

	EQSCRIPT_BIND_FUNC(IsSequencePlaying)
	EQSCRIPT_BIND_FUNC(PlaySequence)
	EQSCRIPT_BIND_FUNC(StopSequence)
	EQSCRIPT_BIND_FUNC(SetPlaybackSpeedScale)
	EQSCRIPT_BIND_FUNC(ResetSequenceTime)
	EQSCRIPT_BIND_FUNC(SetSequenceTime)
	EQSCRIPT_BIND_FUNC(SetSequenceBlending)

	EQSCRIPT_BIND_FUNC(IsTransitionCompleted)

	EQSCRIPT_BIND_FUNC(GetCurrentAnimationFrameCount)
	EQSCRIPT_BIND_FUNC(GetCurrentAnimationDuration)
	EQSCRIPT_BIND_FUNC(GetCurrentAnimationTime)
	EQSCRIPT_BIND_FUNC(GetCurrentAnimationRemainingDuration)

	EQSCRIPT_BIND_FUNC(FindPoseController)
	EQSCRIPT_BIND_FUNC(SetPoseControllerValue)
	EQSCRIPT_BIND_FUNC(GetPoseControllerValue)
	EQSCRIPT_BIND_FUNC(GetPoseControllerInterpValue)
	//EQSCRIPT_BIND_FUNC(GetPoseControllerRange)
EQSCRIPT_TYPE_END

bool eslSysAnimatingInit(const esl::ScriptState& state)
{
	state.RegisterClass<CAnimatingEGF>();
	state.RegisterClassStatic<CAnimatingEGF>("GetActivityByName", EQSCRIPT_CFUNC(GetActivityByName));
	state.RegisterClassStatic<CAnimatingEGF>("GetActivityName", EQSCRIPT_CFUNC(GetActivityName));

	return true;
}