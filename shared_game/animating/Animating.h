//////////////////////////////////////////////////////////////////////////////////
// Copyright � Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Animating base
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "BoneSetup.h"
#include "anim_events.h"
#include "anim_activity.h"

#define MAX_SEQUENCE_TIMERS			5

//--------------------------------------------------------------------------------------

class CEqStudioGeom;
struct studioJoint_t;

class CAnimatingEGF
{
public:
	CAnimatingEGF();
	virtual ~CAnimatingEGF();

	virtual void				InitAnimating(CEqStudioGeom* model);
	void						DestroyAnimating();

	virtual int					FindBone(const char* boneName) const;			// finds bone

	virtual const Vector3D&		GetLocalBoneOrigin(int nBone) const;			// gets local bone position
	virtual const Vector3D&		GetLocalBoneDirection(int nBone) const;			// gets local bone direction

	virtual Matrix4x4*			GetBoneMatrices() const;						// returns transformed bones

	void						SetSequenceBlending(int slot, float factor);
	void						SwapSequenceTimers(int index, int swapTo);

// forward kinematics

	int							FindSequence(const char* name) const;				// finds animation
	int							FindSequenceByActivity(Activity act, int slot = 0) const;

	void						SetActivity(Activity act, int slot = 0);	// sets activity
	Activity					GetCurrentActivity(int slot = 0);			// returns current activity

	bool						IsSequencePlaying(int slot = 0) const;
	void						SetSequence(int seqIdx, int slot = 0);			// sets new sequence
	void						SetSequenceByName(const char* name, int slot = 0);	// sets new sequence by it's name
	void						PlaySequence(int slot = 0);						// plays/resumes animation
	void						StopSequence(int slot = 0);						// stops/pauses animation
	void						ResetSequenceTime(int slot = 0);				// resets animation time, and restarts animation
	void						SetSequenceTime(float newTime, int slot = 0);	// sets new animation time

	float						GetCurrentAnimationDuration(int slot = 0) const;			// returns duration time of the current animation
	float						GetCurrentAnimationTime(int slot = 0) const;				// returns elapsed time of the current animation
	float						GetCurrentRemainingAnimationDuration(int slot = 0) const;	// returns remaining duration time of the current animation

	float						GetAnimationDuration(int animIndex) const;		// returns duration time of the specific animation
	
	int							FindPoseController(const char *name);						// returns pose controller index
	void						SetPoseControllerValue(int nPoseCtrl, float value);			// sets value of the pose controller
	float						GetPoseControllerValue(int nPoseCtrl) const;

	void						SetPlaybackSpeedScale(float scale, int slotindex = 0);		// sets playback speed scale

	Matrix4x4					GetLocalStudioTransformMatrix(int transformIdx) const;

// inverse kinematics

	void						SetIKWorldTarget(int chain_id, const Vector3D &world_position, const Matrix4x4& worldTransform); // sets ik world point, use external transform if model transform differs from entity transform
	void						SetIKLocalTarget(int chain_id, const Vector3D &local_position);	// sets local, model-space ik point target

	void						SetIKChainEnabled(int chain_id, bool enabled);				// enables or disables ik chain.
	bool						IsIKChainEnabled(int chain_id);								// returns status if ik chain
	int							FindIKChain(const char* pszName);							// searches for ik chain

	// advances frame (and computes interpolation between all blended animations)
	void						AdvanceFrame(float fDt);
	void						UpdateIK(float fDt, const Matrix4x4& worldTransform);

	void						RecalcBoneTransforms();

	void						DebugRender(const Matrix4x4& worldTransform);
protected:

	void						RaiseSequenceEvents(sequencetimer_t& timer);
	void						UpdateIkChain(gikchain_t* pIkChain, float fDt);
	

	virtual Activity			TranslateActivity(Activity act, int slotindex = 0) const;			// translates activity
	virtual void				HandleAnimatingEvent(AnimationEvent nEvent, const char* options);

	virtual void				AddMotions(const studioMotionData_t& motionData);

	// transition time from previous
	float						m_transitionTime;
	float						m_transitionRemTime;
	qanimframe_t*				m_transitionFrames{ nullptr };
	qanimframe_t*				m_velocityFrames{ nullptr };

	// computed ready-to-use matrices
	Matrix4x4*					m_boneTransforms{ nullptr };

	// local bones/base pose
	ArrayCRef<studioJoint_t>		m_joints{ nullptr };
	ArrayCRef<studiotransform_t>	m_transforms{ nullptr };

	// different motion packages has different sequience list
	Array<gsequence_t>			m_seqList{ PP_SL }; // loaded sequences
	Array<gposecontroller_t>	m_poseControllers{ PP_SL }; // pose controllers
	Array<gikchain_t>			m_ikChains{ PP_SL };

	// sequence timers. first timer is main, and transitional is last
	sequencetimer_t				m_sequenceTimers[MAX_SEQUENCE_TIMERS];
};
