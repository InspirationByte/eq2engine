//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Game basic weapon class
//
//				TODO: Divide from BaseEntity//////////////////////////////////////////////////////////////////////////////////

#ifndef BASEANIMATING_H
#define BASEANIMATING_H

#include "BaseEngineHeader.h"
#include "anim_activity.h"
#include "anim_events.h"

#include "iksolver_ccd.h"

#include "GameRenderer.h"

#define MAX_SEQUENCE_TIMERS			5

#define IK_ATTACH_WORLD				0
#define IK_ATTACH_LOCAL				1
#define IK_ATTACH_GROUND			2

// basic animating class
// for ragdoll use baseragdollanimating
class BaseAnimating : public BaseEntity
{
	DEFINE_CLASS_BASE(BaseEntity);
public:

								BaseAnimating();

// standard stuff

	virtual	void				InitScriptHooks();

	virtual void				InitAnimationThings();

	virtual void				PreloadMotionData(studioHwData_t::motionData_t* pMotionData);
	virtual void				PrecacheSoundEvents(studioHwData_t::motionData_t* pMotionData);

	// applyes damage to entity (use this)
	virtual void				MakeDecal( const decalmakeinfo_t& info );

	virtual int					FindBone(const char* boneName);				// finds bone

	virtual Vector3D			GetLocalBoneOrigin(int nBone);				// gets local bone position

	virtual Vector3D			GetLocalBoneDirection(int nBone);			// gets local bone direction

	virtual int					FindAttachment(const char* name);			// finds attachment

	virtual Vector3D			GetLocalAttachmentOrigin(int nAttach);		// gets local attachment position

	virtual Vector3D			GetLocalAttachmentDirection(int nAttach);	// gets local attachment direction

// forward kinematics

	void						SetSequence(int animIndex, int slot = 0);	// sets animation

	int							FindSequence(const char* name);				// finds animation

	virtual void				SetActivity(Activity act, int slot = 0);	// sets activity

	virtual Activity			GetCurrentActivity(int slot = 0);			// returns current activity

	virtual void				ResetAnimationTime(int slot = 0);			// resets animation time, and restarts animation

	virtual void				SetAnimationTime(float newTime, int slot = 0);	// sets new animation time

	virtual float				GetCurrentAnimationDuration();				// returns duration time of the current animation

	virtual float				GetAnimationDuration(int animIndex);		// returns duration time of the specific animation

	virtual float				GetCurrentRemainingAnimationDuration();		// returns remaining duration time of the current animation
	virtual void				PlayAnimation(int slot = 0);				// plays/resumes animation
	virtual void				StopAnimation(int slot = 0);				// stops/pauses animation

	virtual Activity			TranslateActivity(Activity act, int slotindex = 0);			// translates activity

	virtual void				UpdateEvents(float framerate);				// updates events

	virtual Matrix4x4*			GetBoneMatrices();							// returns transformed bones
	virtual int					GetBoneCount();								// returns bone count

	int							FindPoseController(char *name);								// returns pose controller index
	void						SetPoseControllerValue(int nPoseCtrl, float value);			// sets value of the pose controller

	void						SetPlaybackSpeedScale(float scale, int slotindex = 0);		// sets playback speed scale

// inverse kinematics

	void						SetIKWorldTarget(int chain_id, const Vector3D &world_position, Matrix4x4 *externaltransform = NULL); // sets ik world point, use external transform if model transform differs from entity transform
	void						SetIKLocalTarget(int chain_id, const Vector3D &local_position);	// sets local, model-space ik point target

	void						SetIKChainEnabled(int chain_id, bool enabled);				// enables or disables ik chain.
	bool						IsIKChainEnabled(int chain_id);								// returns status if ik chain
	int							FindIKChain(char* pszName);									// searches for ik chain

	virtual void				AttachIKChain(int chain, int attach_type);					// attaches chain (target) with IK_ATTACH_* type

	// sets model for this entity
	void						SetModel(IEqModel* pModel);

	// sets model for this entity
	void						SetModel(const char* pszModelName);

protected:
	// advances frame (and computes interpolation between all blended animations)
	void						AdvanceFrame(float frameTime);

	// updates bones
	void						UpdateBones();

	// updates inverse kinematics
	void						UpdateIK();

	// solves single ik chain
	void						UpdateIkChain(gikchain_t* pIkChain);

	// makes standard pose
	void						StandardPose();

	void						GetInterpolatedBoneFrame(studioHwData_t::motionData_t::animation_t* pAnim, int nBone, int firstframe, int lastframe, float interp, animframe_t &out);

	void						GetInterpolatedBoneFrameBetweenTwoAnimations(studioHwData_t::motionData_t::animation_t* pAnim1, studioHwData_t::motionData_t::animation_t* pAnim2, int nBone, int firstframe, int lastframe, float interp, float animTransition, animframe_t &out);

	void						GetSequenceLayerBoneFrame(gsequence_t* pSequence, int nBone, animframe_t &out);

	void						SetSequenceBlending(int slot, float factor);

	virtual void				HandleEvent(AnimationEvent nEvent, char* options);

	virtual void				Update(float dt);

	void						DestroyAnimationThings();

	// renders model
	virtual void				Render(int nViewRenderFlags);

	virtual void				OnRemove();

protected:

	// swaps sequence timers
	void						SwapSequenceTimers(int index, int swapTo);

	// sequence timers. first timer is main, and transitional is last
	sequencetimer_t				m_sequenceTimers[MAX_SEQUENCE_TIMERS];

	// blend values for sequence timers.
	// first blend value is always 1.
	float						m_sequenceTimerBlendings[MAX_SEQUENCE_TIMERS];

	// transition time from previous
	float						m_fRemainingTransitionTime;

	// last computed bone frames
	animframe_t*				m_pLastBoneFrames;

	animframe_t*				m_pBoneVelocities;
	animframe_t*				m_pBoneSpringingAdd;

	animframe_t*				m_pTransitionAnimationBoneFrames;

	// computed ready-to-use matrices
	Matrix4x4*					m_BoneMatrixList;

	// local bones/base pose
	Matrix4x4*					m_LocalBonematrixList;

	// animation-only bone matrix list, for blending with IK
	Matrix4x4*					m_AnimationBoneMatrixList;

	int*						m_nParentIndexList;
	int							m_numBones;

	DkList<gsequence_t>			m_pSequences; // loaded sequences
	DkList<gposecontroller_t>	m_poseControllers; // pose controllers
	DkList<gikchain_t*>			m_IkChains;

	DECLARE_DATAMAP();
};

#endif // BASEANIMATING_H