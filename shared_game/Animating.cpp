//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2019
//////////////////////////////////////////////////////////////////////////////////
// Description: Animating base
//////////////////////////////////////////////////////////////////////////////////

#include "Animating.h"
#include "GameSoundEmitterSystem.h"
#include "IDebugOverlay.h"

ConVar r_debug_ik("r_debug_ik", "0", "Draw debug information about Inverse Kinematics", CV_CHEAT);
ConVar r_debug_showskeletons("r_debug_showskeletons", "0", "Draw debug information about skeletons", CV_CHEAT);

ConVar r_ik_iterations("r_ik_iterations", "100", "IK link iterations per update", CV_ARCHIVE);

/*
ConVar r_springanimations("r_springanimations", "0", "Animation springing", CV_CHEAT);
ConVar r_springanimations_velmul("r_springanimations_velmul", "1.0f", "Animation springing", CV_CHEAT | CV_ARCHIVE);
ConVar r_springanimations_damp("r_springanimations_damp", "1.0f", "Animation springing", CV_CHEAT | CV_ARCHIVE);
ConVar r_springanimations_spring("r_springanimations_spring", "1.0f", "Animation springing", CV_CHEAT | CV_ARCHIVE);
*/

CAnimatingEGF::CAnimatingEGF()
{
	memset(m_seqBlendWeights, 0, sizeof(m_seqBlendWeights));
	m_seqBlendWeights[0] = 1.0f;

	m_boneTransforms = nullptr;
	m_ikBones = nullptr;
	m_joints = nullptr;

	m_transitionFrames = nullptr;
	m_prevFrames = nullptr;

	m_transitionTime = 0.0f;
}

void CAnimatingEGF::DestroyAnimating()
{
	m_seqList.clear();

	if (m_boneTransforms)
		PPFree(m_boneTransforms);
	m_boneTransforms = nullptr;

	if (m_ikBones)
		PPFree(m_ikBones);
	m_ikBones = nullptr;

	if (m_transitionFrames)
		PPFree(m_transitionFrames);
	m_transitionFrames = nullptr;

	if (m_prevFrames)
		PPFree(m_prevFrames);
	m_prevFrames = nullptr;

	m_joints = nullptr;

	for (int i = 0; i < m_ikChains.numElem(); i++)
	{
		PPFree(m_ikChains[i]->links);
		delete m_ikChains[i];
	}

	m_ikChains.clear();

	//if(m_velocityFrames)
	//	PPFree(m_velocityFrames);

	//m_velocityFrames = NULL;

	//if(m_springFrames)
	//	PPFree(m_springFrames);

	//m_springFrames = NULL;

	// stop all sequence timers
	for (int i = 0; i < MAX_SEQUENCE_TIMERS; i++)
		m_sequenceTimers[i].Reset();

	m_seqBlendWeights[0] = 1.0f;

	m_transitionTime = 0.0f;

	m_numBones = 0;
}

Activity CAnimatingEGF::TranslateActivity(Activity act, int slot)
{
	// base class, no translation
	return act;
}

void CAnimatingEGF::InitAnimating(IEqModel* model)
{
	for (int i = 0; i < MAX_SEQUENCE_TIMERS; i++)
		m_sequenceTimers[i].Reset();

	if (model)
	{
		studiohdr_t* studio = model->GetHWData()->studio;

		m_joints = model->GetHWData()->joints;
		m_numBones = studio->numBones;

		m_boneTransforms = PPAllocStructArray(Matrix4x4, m_numBones);
		m_ikBones = PPAllocStructArray(Matrix4x4, m_numBones);

		for (int i = 0; i < m_numBones; i++)
		{
			m_boneTransforms[i] = identity4();
			m_ikBones[i] = identity4();
		}

		m_prevFrames = PPAllocStructArray(animframe_t, m_numBones);
		memset(m_prevFrames, 0, sizeof(animframe_t)*m_numBones);

		m_transitionFrames = PPAllocStructArray(animframe_t, m_numBones);
		memset(m_transitionFrames, 0, sizeof(animframe_t)*m_numBones);

		//m_springFrames = PPAllocStructArray(animframe_t, m_numBones);
		//memset(m_springFrames, 0, sizeof(animframe_t)*m_numBones);

		//m_velocityFrames = PPAllocStructArray(animframe_t, m_numBones);
		//memset(m_velocityFrames, 0, sizeof(animframe_t)*m_numBones);

		// make standard pose
		DefaultPose();

		int numIkChains = studio->numIKChains;

		// load ik chains
		for (int i = 0; i < numIkChains; i++)
		{
			studioikchain_t* pStudioChain = studio->pIkChain(i);

			gikchain_t* chain = new gikchain_t;

			strcpy(chain->name, pStudioChain->name);
			chain->numLinks = pStudioChain->numLinks;
			chain->local_target = Vector3D(0, 0, 10);
			chain->enable = false;

			chain->links = PPAllocStructArray(giklink_t, chain->numLinks);

			for (int j = 0; j < chain->numLinks; j++)
			{
				giklink_t link;
				link.l = pStudioChain->pLink(j);

				studioiklink_t* pLink = pStudioChain->pLink(j);

				studioHwData_t::joint_t& joint = m_joints[link.l->bone];

				const Vector3D& rotation = joint.rotation;

				link.position = joint.position;
				link.quat = Quaternion(rotation.x, rotation.y, rotation.z);

				// initial transform
				link.localTrans = Matrix4x4(link.quat);
				link.localTrans.setTranslation(link.position);
				link.absTrans = identity4();

				int parentLinkId = j - 1;

				if (parentLinkId >= 0)
					link.parent = &chain->links[j - 1];
				else
					link.parent = nullptr;

				chain->links[j] = link;

				link.chain = chain;

				// set link for fast access, can be used for multiple instances of base animating entities with single model
				joint.link_id = j;
				joint.chain_id = i;
			}

			for (int j = 0; j < chain->numLinks; j++)
			{
				giklink_t& link = chain->links[j];

				if (link.parent)
					link.absTrans = link.localTrans * link.parent->absTrans;
				else
					link.absTrans = link.localTrans;
			}

			m_ikChains.append(chain);
		}
	}

	int numMotionPackages = model->GetHWData()->numMotionPackages;

	// build activity table for loaded model
	if (model && numMotionPackages)
	{
		for (int i = 0; i < numMotionPackages; i++)
		{
			studioHwData_t::motionData_t* motion = model->GetHWData()->motiondata[i];

			PreloadMotionData(motion);
			PrecacheSoundEvents(motion);
		}
	}
}

void CAnimatingEGF::PreloadMotionData(studioHwData_t::motionData_t* pMotionData)
{
	// create pose controllers
	for (int i = 0; i < pMotionData->numPoseControllers; i++)
	{
		gposecontroller_t controller;
		controller.p = &pMotionData->poseControllers[i];

		// get center in blending range
		controller.value = lerp(controller.p->blendRange[0], controller.p->blendRange[1], 0.5f);
		controller.interpolatedValue = controller.value;

		m_poseControllers.append(controller);
	}

	// copy sequences
	for (int i = 0; i < pMotionData->numsequences; i++)
	{
		sequencedesc_t& seq = pMotionData->sequences[i];

		gsequence_t seqData;
		memset(&seqData, 0, sizeof(seqData));

		seqData.s = &seq;
		seqData.activity = GetActivityByName(seq.activity);

		if (seqData.activity == ACT_INVALID && stricmp(seq.activity, "ACT_INVALID"))
			MsgError("Motion Data: Activity '%s' not registered\n", seq.activity);

		if (seq.posecontroller >= 0)
			seqData.posecontroller = &m_poseControllers[seq.posecontroller];

		for (int j = 0; j < seq.numAnimations; j++)
			seqData.animations[j] = &pMotionData->animations[seq.animations[j]];

		for (int j = 0; j < seq.numEvents; j++)
			seqData.events[j] = &pMotionData->events[seq.events[j]];

		for (int j = 0; j < seq.numSequenceBlends; j++)
			seqData.blends[j] = &m_seqList[seq.sequenceblends[j]];

		m_seqList.append(seqData);
	}
}

void CAnimatingEGF::PrecacheSoundEvents(studioHwData_t::motionData_t* pMotionData)
{
	for (int i = 0; i < pMotionData->numEvents; i++)
	{
		sequenceevent_t& evt = pMotionData->events[i];
		AnimationEvent event_type = GetEventByName(evt.command);

		switch (event_type)
		{
			//case EV_INVALID:
			//	event_type = (AnimationEvent)atoi(evt.command);
			//	break;
		case EV_SOUND:
			g_sounds->PrecacheSound(evt.parameter);
			break;
		}
	}
}

// finds animation
int CAnimatingEGF::FindSequence(const char* name)
{
	for (int i = 0; i < m_seqList.numElem(); i++)
	{
		if (!stricmp(m_seqList[i].s->name, name))
		{
			return i;
		}
	}
	return -1;
}

// sets animation
void CAnimatingEGF::SetSequence(int animIndex, int slot)
{
	if (animIndex == -1)
		return;

	sequencetimer_t& timer = m_sequenceTimers[slot];

	bool wasEmpty = (timer.sequence_index == -1);

	timer.sequence_index = animIndex;

	if (timer.sequence_index != -1)
		timer.seq = &m_seqList[timer.sequence_index];

	// reset playback speed to avoid some errors
	SetPlaybackSpeedScale(1.0f, slot);

	if (slot == 0)
	{
		m_transitionTime = timer.seq->s->transitiontime;

		if (wasEmpty)
			m_transitionTime = 0.0f;

		// copy last frames for transition
		memcpy(m_transitionFrames, m_prevFrames, sizeof(animframe_t)*m_numBones);
	}
}

// sets activity
void CAnimatingEGF::SetActivity(Activity act, int slot)
{
	// translate activity
	Activity nTranslatedActivity = TranslateActivity(act, slot);

	for (int i = 0; i < m_seqList.numElem(); i++)
	{
		if (m_seqList[i].activity == nTranslatedActivity)
		{
			SetSequence(i, slot);

			ResetAnimationTime(slot);

			PlayAnimation(slot);

			return;
		}
	}

	DevMsg(1, "Activity \"%s\" not found!\n", GetActivityName(act));
}

// returns current activity
Activity CAnimatingEGF::GetCurrentActivity(int slot)
{
	if (m_sequenceTimers[slot].seq)
		return m_sequenceTimers[slot].seq->activity;

	return ACT_INVALID;
}

// resets animation time, and restarts animation
void CAnimatingEGF::ResetAnimationTime(int slot)
{
	m_sequenceTimers[slot].ResetPlayback();
}

// sets new animation time
void CAnimatingEGF::SetAnimationTime(float newTime, int slot)
{
	m_sequenceTimers[slot].SetTime(newTime);
}

Matrix4x4* CAnimatingEGF::GetBoneMatrices() const
{
	return m_boneTransforms;
}

// makes default pose
void CAnimatingEGF::DefaultPose()
{
	if (!m_joints)
		return;

	for (int i = 0; i < m_numBones; i++)
		m_boneTransforms[i] = m_joints[i].absTrans;
}

// finds bone
int CAnimatingEGF::FindBone(const char* boneName) const
{
	for (int i = 0; i < m_numBones; i++)
	{
		if (!stricmp(m_joints[i].name, boneName))
			return i;
	}

	return -1;
}

// gets absolute bone position
const Vector3D& CAnimatingEGF::GetLocalBoneOrigin(int nBone) const
{
	static Vector3D _zero(0.0f);

	if (nBone == -1)
		return _zero;

	return m_boneTransforms[nBone].rows[3].xyz();
}

// gets absolute bone direction
const Vector3D& CAnimatingEGF::GetLocalBoneDirection(int nBone) const
{
	return m_boneTransforms[nBone].rows[2].xyz();
}

// returns duration time of the current animation
float CAnimatingEGF::GetCurrentAnimationDuration() const
{
	if (!m_sequenceTimers[0].seq)
		return 1.0f;

	return (m_sequenceTimers[0].seq->animations[0]->bones[0].numFrames - 1) / m_sequenceTimers[0].seq->s->framerate;
}

// returns duration time of the specific animation
float CAnimatingEGF::GetAnimationDuration(int animIndex) const
{
	if (animIndex == -1)
		return 1.0f;

	return (m_seqList[animIndex].animations[0]->bones[0].numFrames - 1) / m_seqList[animIndex].s->framerate;
}

// returns remaining duration time of the current animation
float CAnimatingEGF::GetCurrentRemainingAnimationDuration() const
{
	return GetCurrentAnimationDuration() - m_sequenceTimers[0].sequence_time;
}

// plays/resumes animation
void CAnimatingEGF::PlayAnimation(int slot)
{
	m_sequenceTimers[slot].bPlaying = true;
}

// stops/pauses animation
void CAnimatingEGF::StopAnimation(int slot)
{
	m_sequenceTimers[slot].bPlaying = false;
}

void CAnimatingEGF::SetPlaybackSpeedScale(float scale, int slot)
{
	m_sequenceTimers[slot].playbackSpeedScale = scale;
}

void CAnimatingEGF::SetSequenceBlending(int slot, float factor)
{
	m_seqBlendWeights[slot] = factor;
}

// updates events
void CAnimatingEGF::UpdateEvents(float framerate)
{
	// handle events
	for (int i = 0; i < MAX_SEQUENCE_TIMERS; i++)
	{
		sequencetimer_t& timer = m_sequenceTimers[i];

		for (int j = 0; j < timer.called_events.numElem(); j++)
		{
			sequenceevent_t* called = timer.called_events[j];

			AnimationEvent event_type = GetEventByName(called->command);

			if (event_type == EV_INVALID)
			{
				event_type = (AnimationEvent)atoi(called->command);
			}

			HandleAnimatingEvent(event_type, called->parameter);
		}

		timer.called_events.clear();
	}
}

// advances frame (and computes interpolation between all blended animations)
void CAnimatingEGF::AdvanceFrame(float frameTime)
{
	m_seqBlendWeights[0] = 1.0f;

	if (m_sequenceTimers[0].seq)
	{
		float div_frametime = (frameTime * 30) / 8;

		// interpolate pose parameter values
		for (int i = 0; i < m_poseControllers.numElem(); i++)
		{
			gposecontroller_t& ctrl = m_poseControllers[i];

			for (int j = 0; j < 8; j++)
			{
				ctrl.interpolatedValue = lerp(ctrl.interpolatedValue, ctrl.value, div_frametime);
			}
		}
	}

	m_transitionTime -= frameTime;

	if (m_transitionTime < 0)
		m_transitionTime = 0;

	// play animations
	for (int i = 0; i < MAX_SEQUENCE_TIMERS; i++)
	{
		sequencetimer_t& timer = m_sequenceTimers[i];

		if (timer.sequence_index != -1)
			timer.seq = &m_seqList[timer.sequence_index];

		timer.AdvanceFrame(frameTime);
	}

	// call events.
	UpdateEvents(frameTime);
}

// swaps sequence timers
void CAnimatingEGF::SwapSequenceTimers(int index, int swapTo)
{
	sequencetimer_t swap_to = m_sequenceTimers[swapTo];
	m_sequenceTimers[swapTo] = m_sequenceTimers[index];
	m_sequenceTimers[index] = swap_to;

	float swap_to_time = m_seqBlendWeights[swapTo];
	m_seqBlendWeights[swapTo] = m_seqBlendWeights[index];
	m_seqBlendWeights[index] = swap_to_time;
}

void CAnimatingEGF::GetInterpolatedBoneFrame(studioHwData_t::motionData_t::animation_t* pAnim, int nBone, int firstframe, int lastframe, float interp, animframe_t &out) const
{
	studioHwData_t::motionData_t::animation_t::boneframe_t& frame = pAnim->bones[nBone];

	InterpolateFrameTransform(frame.keyFrames[firstframe], frame.keyFrames[lastframe], clamp(interp, 0, 1), out);
}

void CAnimatingEGF::GetInterpolatedBoneFrameBetweenTwoAnimations(
	studioHwData_t::motionData_t::animation_t* pAnim1, 
	studioHwData_t::motionData_t::animation_t* pAnim2,
	int nBone, int firstframe, int lastframe, float interp, float animTransition, animframe_t &out) const
{
	// compute frame 1
	animframe_t anim1transform;
	GetInterpolatedBoneFrame(pAnim1, nBone, firstframe, lastframe, interp, anim1transform);

	// compute frame 2
	animframe_t anim2transform;
	GetInterpolatedBoneFrame(pAnim2, nBone, firstframe, lastframe, interp, anim2transform);

	// resultative interpolation
	InterpolateFrameTransform(anim1transform, anim2transform, animTransition, out);
}

void CAnimatingEGF::GetSequenceLayerBoneFrame(gsequence_t* pSequence, int nBone, animframe_t &out) const
{
	float blendWeight = 0;
	int blendAnimation1 = 0;
	int blendAnimation2 = 0;

	ComputeAnimationBlend(pSequence->s->numAnimations,
		pSequence->posecontroller->p->blendRange,
		pSequence->posecontroller->value,
		blendWeight,
		blendAnimation1,
		blendAnimation2
	);

	studioHwData_t::motionData_t::animation_t* pAnim1 = pSequence->animations[blendAnimation1];
	studioHwData_t::motionData_t::animation_t* pAnim2 = pSequence->animations[blendAnimation2];

	GetInterpolatedBoneFrameBetweenTwoAnimations(pAnim1,
		pAnim2,
		nBone,
		0,
		0,
		0,
		blendWeight,
		out
	);

}

int CAnimatingEGF::FindPoseController(char *name)
{
	for (int i = 0; i < m_poseControllers.numElem(); i++)
	{
		if (!stricmp(m_poseControllers[i].p->name, name))
			return i;
	}

	return -1;
}

void CAnimatingEGF::SetPoseControllerValue(int nPoseCtrl, float value)
{
	if (nPoseCtrl == -1 || m_poseControllers.numElem() - 1 < nPoseCtrl)
		return;

	m_poseControllers[nPoseCtrl].value = value;
}

// updates bones
void CAnimatingEGF::UpdateBones(float fDt, const Matrix4x4& worldTransform)
{
	for (int i = 0; i < MAX_SEQUENCE_TIMERS; i++)
	{
		sequencetimer_t& timer = m_sequenceTimers[i];

		if (timer.sequence_index != -1)
			timer.seq = &m_seqList[timer.sequence_index];
	}

	if (!m_sequenceTimers[0].seq || m_seqList.numElem() == 0)
	{
		DefaultPose();
		return;
	}

	// setup each bone's transformation
	for (int boneId = 0; boneId < m_numBones; boneId++)
	{
		animframe_t cComputedFrame;
		ZeroFrameTransform(cComputedFrame);

		for (int j = 0; j < MAX_SEQUENCE_TIMERS; j++)
		{
			sequencetimer_t& timer = m_sequenceTimers[j];

			// if no animation plays on this timer, continue
			if (!timer.seq)
				continue;

			gsequence_t* seq = timer.seq;
			sequencedesc_t* seqDesc = seq->s;

			float blend_weight = m_seqBlendWeights[j];

			if (blend_weight <= 0)
				continue;

			studioHwData_t::motionData_t::animation_t* curanim = seq->animations[0];

			if (!curanim)
				continue;

			// the computed frame
			animframe_t cTimedFrame;
			ZeroFrameTransform(cTimedFrame);

			float frame_interp = timer.sequence_time - timer.currFrame;

			int numAnims = seqDesc->numAnimations;

			// blend between many animations in sequence using pose controller
			if (numAnims > 1 && timer.seq->posecontroller)
			{
				gposecontroller_t* ctrl = timer.seq->posecontroller;

				float playingBlendWeight = 0;
				int playingBlendAnimation1 = 0;
				int playingBlendAnimation2 = 0;

				// get frame indexes and lerp value of blended animation
				ComputeAnimationBlend(numAnims,
					ctrl->p->blendRange,
					ctrl->value,
					playingBlendWeight,
					playingBlendAnimation1,
					playingBlendAnimation2);

				// get frame pointers
				studioHwData_t::motionData_t::animation_t* pPlayingAnim1 = seq->animations[playingBlendAnimation1];
				studioHwData_t::motionData_t::animation_t* pPlayingAnim2 = seq->animations[playingBlendAnimation2];

				// compute blending frame
				GetInterpolatedBoneFrameBetweenTwoAnimations(pPlayingAnim1,
					pPlayingAnim2,
					boneId,
					timer.currFrame,
					timer.nextFrame,
					frame_interp,
					playingBlendWeight,
					cTimedFrame);
			}
			else
			{
				// simply compute frames
				GetInterpolatedBoneFrame(curanim, boneId, timer.currFrame, timer.nextFrame, frame_interp, cTimedFrame);
			}

			animframe_t cAddFrame;
			ZeroFrameTransform(cAddFrame);

			int seqBlends = seqDesc->numSequenceBlends;

			// add blended sequences to this
			for (int blend_seq = 0; blend_seq < seqBlends; blend_seq++)
			{
				gsequence_t* pSequence = seq->blends[blend_seq];

				animframe_t frame;

				// get bone frame of layer
				GetSequenceLayerBoneFrame(pSequence, boneId, frame);

				AddFrameTransform(cAddFrame, frame, cAddFrame);
			}

			AddFrameTransform(cTimedFrame, cAddFrame, cTimedFrame);

			// interpolate or add the slots, this is useful for body part splitting
			if (seqDesc->flags & SEQFLAG_SLOTBLEND)
			{
				cTimedFrame.angBoneAngles *= blend_weight;
				cTimedFrame.vecBonePosition *= blend_weight;

				AddFrameTransform(cComputedFrame, cTimedFrame, cComputedFrame);
			}
			else
				InterpolateFrameTransform(cComputedFrame, cTimedFrame, blend_weight, cComputedFrame);
		}

		gsequence_t* firstTimerSeq = m_sequenceTimers[0].seq;

		// transition of the animation frames TODO: additional doubling slots for non-base timers
		if (firstTimerSeq && m_transitionTime > 0)
		{
			float transition_factor = m_transitionTime / firstTimerSeq->s->transitiontime;

			InterpolateFrameTransform(cComputedFrame, m_transitionFrames[boneId], transition_factor, cComputedFrame);
		}

		/*
		if(r_springanimations.GetBool() && firstTimerSeq)
		{
			animframe_t mulFrame = firstTimerSeq->animations[0]->bones[boneId].keyFrames[m_sequenceTimers[0].nextFrame];

			mulFrame.angBoneAngles -= cComputedFrame.angBoneAngles;
			mulFrame.vecBonePosition -= cComputedFrame.vecBonePosition;

			mulFrame.angBoneAngles *= r_springanimations_velmul.GetFloat();
			mulFrame.vecBonePosition *= r_springanimations_velmul.GetFloat();

			AddFrameTransform(m_velocityFrames[boneId], mulFrame, m_velocityFrames[boneId]);
		}


		if(r_springanimations.GetBool() && dot(m_velocityFrames[boneId].angBoneAngles, m_velocityFrames[boneId].angBoneAngles) > 0.000001f || dot(m_springFrames[boneId].angBoneAngles, m_springFrames[boneId].angBoneAngles) > 0.000001f)
		{
			m_springFrames[boneId].angBoneAngles += m_velocityFrames[boneId].angBoneAngles * gpGlobals->frametime;
			float damping = 1 - (r_springanimations_damp.GetFloat() * gpGlobals->frametime);

			if ( damping < 0 )
				damping = 0.0f;

			m_velocityFrames[boneId].angBoneAngles *= damping;

			// torsional spring
			float springForceMagnitude = r_springanimations_spring.GetFloat() * gpGlobals->frametime;
			springForceMagnitude = clamp(springForceMagnitude, 0, 2 );
			m_velocityFrames[boneId].angBoneAngles -= m_springFrames[boneId].angBoneAngles * springForceMagnitude;

			cComputedFrame.angBoneAngles += m_springFrames[boneId].angBoneAngles;
		}

		if(r_springanimations.GetBool() && dot(m_velocityFrames[boneId].vecBonePosition, m_velocityFrames[boneId].vecBonePosition) > 0.000001f || dot(m_springFrames[boneId].vecBonePosition, m_springFrames[boneId].vecBonePosition) > 0.000001f)
		{

			m_springFrames[boneId].vecBonePosition += m_velocityFrames[boneId].vecBonePosition * gpGlobals->frametime;
			float damping = 1 - (r_springanimations_damp.GetFloat() * gpGlobals->frametime);

			if ( damping < 0 )
				damping = 0.0f;

			m_velocityFrames[boneId].vecBonePosition *= damping;

			// torsional spring
			// UNDONE: Per-axis spring constant?
			float springForceMagnitude = r_springanimations_spring.GetFloat() * gpGlobals->frametime;
			springForceMagnitude = clamp(springForceMagnitude, 0, 2 );
			m_velocityFrames[boneId].vecBonePosition -= m_springFrames[boneId].vecBonePosition * springForceMagnitude;

			cComputedFrame.vecBonePosition += m_springFrames[boneId].vecBonePosition;
		}
		*/

		// compute transformation
		Matrix4x4 bone_transform = CalculateLocalBonematrix(cComputedFrame);

		// set last bones
		m_prevFrames[boneId] = cComputedFrame;

		m_boneTransforms[boneId] = (bone_transform*m_joints[boneId].localTrans);
	}

	// setup each bone's transformation
	for (int i = 0; i < m_numBones; i++)
	{
		if (m_joints[i].parentbone != -1)
		{
			// multiply by parent transform
			m_boneTransforms[i] = m_boneTransforms[i] * m_boneTransforms[m_joints[i].parentbone];

			//debugoverlay->Line3D(m_boneTransforms[i].rows[3].xyz(),m_boneTransforms[m_nParentIndexList[i]].rows[3].xyz(), color4_white, color4_white);
		}
	}

	// copy animation frames
	memcpy(m_ikBones, m_boneTransforms, sizeof(Matrix4x4)*m_numBones);

	// update inverse kinematics
	UpdateIK(fDt, worldTransform);

	if (r_debug_showskeletons.GetBool())
	{
		// setup each bone's transformation
		for (int i = 0; i < m_numBones; i++)
		{
			if (m_joints[i].parentbone != -1)
			{
				Matrix4x4& transform = m_boneTransforms[i];
				Matrix4x4& parentTransform = m_boneTransforms[m_joints[i].parentbone];

				Vector3D pos = (worldTransform*Vector4D(transform.rows[3].xyz(), 1.0f)).xyz();
				Vector3D parent_pos = (worldTransform*Vector4D(m_boneTransforms[m_joints[i].parentbone].rows[3].xyz(), 1.0f)).xyz();

				Vector3D dX = worldTransform.getRotationComponent()*transform.rows[0].xyz();
				Vector3D dY = worldTransform.getRotationComponent()*transform.rows[1].xyz();
				Vector3D dZ = worldTransform.getRotationComponent()*transform.rows[2].xyz();

				debugoverlay->Line3D(pos, parent_pos, color4_white, color4_white);

				// draw axis
				debugoverlay->Line3D(pos, pos + dX, ColorRGBA(1, 0, 0, 1), ColorRGBA(1, 0, 0, 1));
				debugoverlay->Line3D(pos, pos + dY, ColorRGBA(0, 1, 0, 1), ColorRGBA(0, 1, 0, 1));
				debugoverlay->Line3D(pos, pos + dZ, ColorRGBA(0, 0, 1, 1), ColorRGBA(0, 0, 1, 1));
			}
		}
	}
}

// updates inverse kinematics
void CAnimatingEGF::UpdateIK(float fDt, const Matrix4x4& worldTransform)
{
	bool ik_enabled_bones[128];
	memset(ik_enabled_bones, 0, sizeof(ik_enabled_bones));

	// run through bones and find enabled bones by IK chain
	for (int boneId = 0; boneId < m_numBones; boneId++)
	{
		bool bone_for_ik = false;

		int chain_id = m_joints[boneId].chain_id;
		int link_id = m_joints[boneId].link_id;

		if (link_id != -1 && chain_id != -1 && m_ikChains[chain_id]->enable)
		{
			gikchain_t* chain = m_ikChains[chain_id];

			for (int i = 0; i < chain->numLinks; i++)
			{
				if (chain->links[i].l->bone == boneId)
				{
					ik_enabled_bones[boneId] = true;

					bone_for_ik = true;
					break;
				}
			}
		}
	}

	// solve ik links or copy frames to disabled links
	for (int i = 0; i < m_ikChains.numElem(); i++)
	{
		gikchain_t* chain = m_ikChains[i];

		if (chain->enable)
		{
			// display target
			if (r_debug_ik.GetBool())
			{
				Vector3D target_pos = chain->local_target;
				target_pos = (worldTransform*Vector4D(target_pos, 1.0f)).xyz();

				debugoverlay->Box3D(target_pos - Vector3D(1), target_pos + Vector3D(1), ColorRGBA(0, 1, 0, 1));
			}

			// update chain
			UpdateIkChain(m_ikChains[i], fDt);

			if (r_debug_ik.GetBool())
			{
				for (int j = 0; j < chain->numLinks; j++)
				{
					giklink_t& link = chain->links[j];

					const Vector3D& bone_pos = link.absTrans.rows[3].xyz();
					Vector3D parent_pos = link.parent->absTrans.rows[3].xyz();

					parent_pos = (worldTransform*Vector4D(parent_pos, 1.0f)).xyz();

					Vector3D dX = worldTransform.getRotationComponent()*link.absTrans.rows[0].xyz();
					Vector3D dY = worldTransform.getRotationComponent()*link.absTrans.rows[1].xyz();
					Vector3D dZ = worldTransform.getRotationComponent()*link.absTrans.rows[2].xyz();

					debugoverlay->Line3D(parent_pos, bone_pos, ColorRGBA(1, 1, 0, 1), ColorRGBA(1, 1, 0, 1));
					debugoverlay->Box3D(bone_pos + Vector3D(1), bone_pos - Vector3D(1), ColorRGBA(1, 0, 0, 1));
					debugoverlay->Text3D(bone_pos, 200.0f, color4_white, 0.0f, "%s", m_joints[link.l->bone].name);

					// draw axis
					debugoverlay->Line3D(bone_pos, bone_pos + dX, ColorRGBA(1, 0, 0, 1), ColorRGBA(1, 0, 0, 1));
					debugoverlay->Line3D(bone_pos, bone_pos + dY, ColorRGBA(0, 1, 0, 1), ColorRGBA(0, 1, 0, 1));
					debugoverlay->Line3D(bone_pos, bone_pos + dZ, ColorRGBA(0, 0, 1, 1), ColorRGBA(0, 0, 1, 1));
				}

			}
		}
		else
		{
			// copy last frames to all links
			for (int j = 0; j < chain->numLinks; j++)
			{
				giklink_t& link = chain->links[j];

				int bone_id = link.l->bone;
				studioHwData_t::joint_t& joint = m_joints[link.l->bone];

				link.quat = Quaternion(m_ikBones[bone_id].getRotationComponent());
				link.position = joint.position;

				link.localTrans = Matrix4x4(link.quat);
				link.localTrans.setTranslation(link.position);

				// fix local transform for animation
				link.localTrans = joint.localTrans * link.localTrans;
				link.absTrans = m_ikBones[bone_id];
			}
		}
	}
}

// solves single ik chain
void CAnimatingEGF::UpdateIkChain(gikchain_t* pIkChain, float fDt)
{
	for (int i = 0; i < pIkChain->numLinks; i++)
	{
		giklink_t& link = pIkChain->links[i];

		// this is broken
		link.localTrans = Matrix4x4(link.quat);
		link.localTrans.setTranslation(link.position);

		//int bone_id = link.bone_index;
		//link.localTrans = m_joints[bone_id].localTrans * link.localTrans;
	}

	for (int i = 0; i < pIkChain->numLinks; i++)
	{
		giklink_t& link = pIkChain->links[i];

		if (link.parent)
		{
			link.absTrans = link.localTrans * link.parent->absTrans;

			//link.absTrans = !link.localTrans * link.absTrans;

			// FIXME:	INVALID CALCULATIONS HAPPENED HERE
			//			REWORK IK!!!
			m_boneTransforms[link.l->bone] = link.absTrans;
		}
		else // just copy transform
		{
			link.absTrans = m_boneTransforms[link.l->bone];
		}
	}

	// use last bone for movement
	// TODO: use more points to solve to do correct IK
	int nEffector = pIkChain->numLinks - 1;

	// solve link now
	SolveIKLinks(pIkChain->links[nEffector], pIkChain->local_target, fDt, r_ik_iterations.GetInt());
}

// inverse kinematics

void CAnimatingEGF::SetIKWorldTarget(int chain_id, const Vector3D &world_position, const Matrix4x4& worldTransform)
{
	if (chain_id == -1)
		return;

	Matrix4x4 world_to_model = transpose(worldTransform);

	Vector3D local = world_position;
	local = inverseTranslateVec(local, world_to_model);
	local = inverseRotateVec(local, world_to_model);

	SetIKLocalTarget(chain_id, local);
}

void CAnimatingEGF::SetIKLocalTarget(int chain_id, const Vector3D &local_position)
{
	if (chain_id == -1)
		return;

	m_ikChains[chain_id]->local_target = local_position;
}

void CAnimatingEGF::SetIKChainEnabled(int chain_id, bool enabled)
{
	if (chain_id == -1)
		return;

	m_ikChains[chain_id]->enable = enabled;
}

bool CAnimatingEGF::IsIKChainEnabled(int chain_id)
{
	if (chain_id == -1)
		return false;

	return m_ikChains[chain_id]->enable;
}

int CAnimatingEGF::FindIKChain(char* pszName)
{
	for (int i = 0; i < m_ikChains.numElem(); i++)
	{
		if (!stricmp(m_ikChains[i]->name, pszName))
			return i;
	}

	return -1;
}

/*
void CAnimatingEGF::AttachIKChain(int chain, int attach_type)
{
	if (chain == -1)
		return;

	int effector_id = m_ikChains[chain]->numLinks - 1;
	giklink_t& link = m_ikChains[chain]->links[effector_id];

	switch (attach_type)
	{
		case IK_ATTACH_WORLD:
		{
			SetIKWorldTarget(chain, GetAbsOrigin());
			SetIKChainEnabled(chain, true);
			break;
		}
		case IK_ATTACH_LOCAL:
		{
			SetIKLocalTarget(chain, link.absTrans.rows[3].xyz());
			SetIKChainEnabled(chain, true);
			break;
		}
		case IK_ATTACH_GROUND:
		{
			// don't handle ground
			break;
		}
	}
}*/