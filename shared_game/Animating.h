//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2019
//////////////////////////////////////////////////////////////////////////////////
// Description: Animating base
//////////////////////////////////////////////////////////////////////////////////

#ifndef ANIMATING_H
#define ANIMATING_H

#include "anim_activity.h"
#include "anim_events.h"
#include "iksolver_ccd.h"

#define MAX_SEQUENCE_TIMERS			5

#define IK_ATTACH_WORLD				0
#define IK_ATTACH_LOCAL				1
#define IK_ATTACH_GROUND			2

class CAnimatingEGF
{
public:
	CAnimatingEGF();

	virtual void				InitAnimating(IEqModel* model);
	void						DestroyAnimating();

	virtual int					FindBone(const char* boneName) const;			// finds bone

	virtual const Vector3D&		GetLocalBoneOrigin(int nBone) const;			// gets local bone position
	virtual const Vector3D&		GetLocalBoneDirection(int nBone) const;			// gets local bone direction

	virtual Matrix4x4*			GetBoneMatrices() const;						// returns transformed bones

	void						DefaultPose();



	void						SetSequenceBlending(int slot, float factor);
	void						SwapSequenceTimers(int index, int swapTo);

// forward kinematics

	void						SetSequence(int animIndex, int slot = 0);	// sets animation
	int							FindSequence(const char* name);				// finds animation

	void						SetActivity(Activity act, int slot = 0);	// sets activity
	Activity					GetCurrentActivity(int slot = 0);			// returns current activity

	bool						IsSequencePlaying(int slot = 0) const;
	void						PlaySequence(int slot = 0);					// plays/resumes animation
	void						StopSequence(int slot = 0);					// stops/pauses animation
	void						ResetSequenceTime(int slot = 0);			// resets animation time, and restarts animation
	void						SetSequenceTime(float newTime, int slot = 0);	// sets new animation time

	float						GetCurrentAnimationDuration() const;			// returns duration time of the current animation
	float						GetCurrentRemainingAnimationDuration() const;	// returns remaining duration time of the current animation

	float						GetAnimationDuration(int animIndex) const;		// returns duration time of the specific animation
	
	int							FindPoseController(char *name);								// returns pose controller index
	void						SetPoseControllerValue(int nPoseCtrl, float value);			// sets value of the pose controller
	float						GetPoseControllerValue(int nPoseCtrl) const;

	void						SetPlaybackSpeedScale(float scale, int slotindex = 0);		// sets playback speed scale

// inverse kinematics

	void						SetIKWorldTarget(int chain_id, const Vector3D &world_position, const Matrix4x4& worldTransform); // sets ik world point, use external transform if model transform differs from entity transform
	void						SetIKLocalTarget(int chain_id, const Vector3D &local_position);	// sets local, model-space ik point target

	void						SetIKChainEnabled(int chain_id, bool enabled);				// enables or disables ik chain.
	bool						IsIKChainEnabled(int chain_id);								// returns status if ik chain
	int							FindIKChain(char* pszName);									// searches for ik chain

protected:
	// advances frame (and computes interpolation between all blended animations)
	void						AdvanceFrame(float fDt);

	void						UpdateEvents(float fDt);
	void						UpdateBones(float fDt, const Matrix4x4& worldTransform);
	void						UpdateIK(float fDt, const Matrix4x4& worldTransform);
	void						UpdateIkChain(gikchain_t* pIkChain, float fDt);

	virtual Activity			TranslateActivity(Activity act, int slotindex = 0);			// translates activity
	virtual void				HandleAnimatingEvent(AnimationEvent nEvent, char* options) = 0;

protected:

	virtual void				AddMotions(studioHwData_t::motionData_t* motionData);

	void						GetInterpolatedBoneFrame(studioHwData_t::motionData_t::animation_t* pAnim, int nBone, int firstframe, int lastframe, float interp, animframe_t &out) const;

	void						GetInterpolatedBoneFrameBetweenTwoAnimations(
									studioHwData_t::motionData_t::animation_t* pAnim1, 
									studioHwData_t::motionData_t::animation_t* pAnim2, 
									int nBone, int firstframe, int lastframe, float interp, float animTransition, animframe_t &out) const;

	void						GetSequenceLayerBoneFrame(gsequence_t* pSequence, int nBone, animframe_t &out) const;

	// sequence timers. first timer is main, and transitional is last
	sequencetimer_t				m_sequenceTimers[MAX_SEQUENCE_TIMERS];

	// blend values for sequence timers.
	// first blend value should be always 1.
	float						m_seqBlendWeights[MAX_SEQUENCE_TIMERS];

	// transition time from previous
	float						m_transitionTime;

	// last computed bone frames
	animframe_t*				m_prevFrames;

	animframe_t*				m_velocityFrames;
	animframe_t*				m_springFrames;

	animframe_t*				m_transitionFrames;

	// computed ready-to-use matrices
	Matrix4x4*					m_boneTransforms;

	// local bones/base pose
	//Matrix4x4*					m_LocalBonematrixList;
	studioHwData_t::joint_t*	m_joints;

	// animation-only bone matrix list, for blending with IK
	Matrix4x4*					m_ikBones;

	int							m_numBones;

	DkList<gsequence_t>			m_seqList; // loaded sequences
	DkList<gposecontroller_t>	m_poseControllers; // pose controllers
	DkList<gikchain_t*>			m_ikChains;
};

#endif // ANIMATING_H