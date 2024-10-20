//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Animating base
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "BoneSetup.h"
#include "anim_events.h"
#include "anim_activity.h"

static constexpr const int MAX_SEQUENCE_TIMERS = 5;

//--------------------------------------------------------------------------------------

class CEqStudioGeom;
struct StudioJoint;

class CAnimatingEGF
{
public:
	CAnimatingEGF();
	virtual ~CAnimatingEGF();

	virtual void		InitAnimating(CEqStudioGeom* model);
	void				InitMotionData(const char* name);

	void				DestroyAnimating();

	int					FindBone(const char* boneName) const;			// finds bone

	const Vector3D&		GetLocalBoneOrigin(int nBone) const;			// gets local bone position
	const Vector3D&		GetLocalBoneDirection(int nBone) const;			// gets local bone direction

	Matrix4x4			GetLocalStudioTransformMatrix(int transformIdx) const;

	Matrix4x4*			GetBoneMatrices() const;						// returns transformed bones
	bool				RecalcBoneTransforms();

	// advances frame (and computes interpolation between all blended animations)
	void				AdvanceFrame(float fDt);
	void				UpdateIK(float fDt, const Matrix4x4& worldTransform);

	void				DebugRender(const Matrix4x4& worldTransform);

// activity control (simple sequence state machine)

	void				SetActivity(Activity act, int slot = 0);	// sets activity
	Activity			GetCurrentActivity(int slot = 0) const;		// returns current activity

// forward kinematics sequence control

	int					FindSequence(const char* name) const;				// finds animation
	int					FindSequenceByActivity(Activity act, int slot = 0) const;
	void				SetSequence(int sequenceId, int slot = 0);		// sets new sequence
	void				SetSequenceByName(const char* name, int slot = 0);	// sets new sequence by it's name

	const AnimSequence* GetSequenceById(int sequenceId) const;

	bool				IsSequencePlaying(int slot = 0) const;
	void				PlaySequence(int slot = 0);							// plays/resumes animation
	void				StopSequence(int slot = 0);							// stops/pauses animation
	void				SetPlaybackSpeedScale(float scale, int slot = 0);	// sets playback speed scale
	void				ResetSequenceTime(int slot = 0);					// resets animation time, and restarts animation
	void				SetSequenceTime(float newTime, int slot = 0);		// sets new animation time
	void				SetSequenceBlending(float factor, int slot);

	void				SwapSequenceTimers(int slotFrom, int swapTo);

	bool				IsTransitionCompleted(int slot = 0) const;

	int					GetCurrentAnimationFrameCount(int slot = 0) const;
	float				GetCurrentAnimationDuration(int slot = 0) const;			// returns duration time of the current animation
	float				GetCurrentAnimationTime(int slot = 0) const;				// returns elapsed time of the current animation
	float				GetCurrentAnimationRemainingDuration(int slot = 0) const;	// returns remaining duration time of the current animation

// pose controllers

	int					FindPoseController(const char* name) const;					// returns pose controller index
	void				SetPoseControllerValue(int poseCtrlId, float value);		// sets value of the pose controller
	float				GetPoseControllerValue(int poseCtrlId) const;
	float				GetPoseControllerInterpValue(int poseCtrlId) const;
	void				GetPoseControllerRange(int poseCtrlId, float& rMin, float& rMax) const;

// inverse kinematics

	void				SetIKWorldTarget(int chainId, const Vector3D &world_position, const Matrix4x4& worldTransform); // sets ik world point, use external transform if model transform differs from entity transform
	void				SetIKLocalTarget(int chainId, const Vector3D &local_position);	// sets local, model-space ik point target

	void				SetIKChainEnabled(int chainId, bool enabled);				// enables or disables ik chain.
	bool				IsIKChainEnabled(int chainId);								// returns status if ik chain
	int					FindIKChain(const char* pszName);							// searches for ik chain

protected:

	bool				AddMotionData(const StudioMotionData* motionData);

	void				RaiseSequenceEvents(AnimSequenceTimer& timer);
	void				UpdateIkChain(AnimIkChain& chain, float fDt);

	virtual Activity	TranslateActivity(Activity act, int slotindex = 0) const;			// translates activity
	virtual void		HandleAnimatingEvent(AnimationEvent nEvent, const char* options);

	using SequenceTimers = FixedArray<AnimSequenceTimer, MAX_SEQUENCE_TIMERS>;
	using StudioTransforms = ArrayCRef<studioTransform_t>;

	// transition time from previous
	// sequence timers. first timer is main, and transitional is last
	SequenceTimers				m_sequenceTimers;
	SequenceTimers				m_transitionTimers;

	CEqStudioGeom*				m_geomReference{ nullptr };

	// computed ready-to-use matrices
	Matrix4x4*					m_boneTransforms{ nullptr };

	// local bones/base pose
	ArrayCRef<StudioJoint>		m_joints{ nullptr };
	StudioTransforms			m_transforms{ nullptr };
	Array<AnimDataProvider>		m_animData{ PP_SL };
	Map<int, int>				m_nameToAnimData{ PP_SL };
	Array<AnimIkChain>			m_ikChains{ PP_SL };
	Array<AnimPoseController>	m_poseControllers{ PP_SL };

	volatile uint				m_bonesNeedUpdate{ TRUE };
};
