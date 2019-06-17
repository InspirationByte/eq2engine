//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2019
//////////////////////////////////////////////////////////////////////////////////
// Description: Animating base
//////////////////////////////////////////////////////////////////////////////////

#include "Animating.h"

#include "DebugInterface.h"
#include "ConVar.h"

#include "IDebugOverlay.h"

#include "anim_activity.h"
#include "IEqModel.h"

ConVar r_debug_ik("r_debug_ik", "0", "Draw debug information about Inverse Kinematics", CV_CHEAT);
ConVar r_debug_skeleton("r_debug_skeleton", "0", "Draw debug information about bones", CV_CHEAT);

ConVar r_debug_showbone("r_debug_showbone", "-1", "Shows the bone", CV_CHEAT);

ConVar r_ik_iterations("r_ik_iterations", "100", "IK link iterations per update", CV_ARCHIVE);

inline Matrix4x4 CalculateLocalBonematrix(const animframe_t &frame)
{
	Matrix4x4 bonetransform(Quaternion(frame.angBoneAngles.x, frame.angBoneAngles.y, frame.angBoneAngles.z));
	//bonetransform.setRotation();
	bonetransform.setTranslation(frame.vecBonePosition);

	return bonetransform;
}

// computes blending animation index and normalized weight
inline void ComputeAnimationBlend(int numWeights, float blendrange[2], float blendValue, float &blendWeight, int &blendMainAnimation1, int &blendMainAnimation2)
{
	blendValue = clamp(blendValue, blendrange[0], blendrange[1]);

	// convert to value in range 0..1.
	float actualBlendValue = (blendValue - blendrange[0]) / (blendrange[1] - blendrange[0]);

	// compute animation index
	float normalizedBlend = actualBlendValue * (float)(numWeights - 1);

	int blendMainAnimation = (int)normalizedBlend;

	blendWeight = normalizedBlend - ((int)normalizedBlend);

	int minAnim = blendMainAnimation;
	int maxAnim = blendMainAnimation + 1;

	if (maxAnim > numWeights - 1)
	{
		maxAnim = numWeights - 1;
		minAnim = numWeights - 2;

		blendWeight = 1.0f;
	}

	if (minAnim < 0)
		minAnim = 0;

	blendMainAnimation1 = minAnim;
	blendMainAnimation2 = maxAnim;
}

// interpolates frame transform
inline void InterpolateFrameTransform(const animframe_t &frame1, const animframe_t &frame2, float value, animframe_t &out)
{
	Quaternion q1(frame1.angBoneAngles.x, frame1.angBoneAngles.y, frame1.angBoneAngles.z);
	Quaternion q2(frame2.angBoneAngles.x, frame2.angBoneAngles.y, frame2.angBoneAngles.z);

	Quaternion finQuat = slerp(q1, q2, value);

	out.angBoneAngles = eulers(finQuat);
	out.vecBonePosition = lerp(frame1.vecBonePosition, frame2.vecBonePosition, value);
}

// adds transform TODO: Quaternion rotation
inline void AddFrameTransform(const animframe_t &frame1, const animframe_t &frame2, animframe_t &out)
{
	out.angBoneAngles = frame1.angBoneAngles + frame2.angBoneAngles;
	out.vecBonePosition = frame1.vecBonePosition + frame2.vecBonePosition;
}

// adds and multiplies transform
inline void AddMultiplyFrameTransform(const animframe_t &frame1, const animframe_t &frame2, animframe_t &out)
{
	Quaternion q1(frame1.angBoneAngles.x, frame1.angBoneAngles.y, frame1.angBoneAngles.z);
	Quaternion q2(frame2.angBoneAngles.x, frame2.angBoneAngles.y, frame2.angBoneAngles.z);

	Quaternion finQuat = q1 * q2;

	out.angBoneAngles = eulers(finQuat);

	out.vecBonePosition = frame1.vecBonePosition + frame2.vecBonePosition;
}

// zero frame
inline void ZeroFrameTransform(animframe_t &frame)
{
	frame.angBoneAngles = vec3_zero;
	frame.vecBonePosition = vec3_zero;
}

CAnimatingEGF::CAnimatingEGF()
{
	m_boneTransforms = nullptr;
	m_joints = nullptr;
	m_numBones = 0;

	m_transitionFrames = nullptr;

	m_transitionTime = m_transitionRemTime = DEFAULT_TRANSITION_TIME;
}

void CAnimatingEGF::DestroyAnimating()
{
	m_seqList.clear();
	m_poseControllers.clear();

	for (int i = 0; i < m_ikChains.numElem(); i++)
	{
		PPFree(m_ikChains[i]->links);
		delete m_ikChains[i];
	}

	m_ikChains.clear();

	m_joints = nullptr;

	if (m_boneTransforms)
		PPFree(m_boneTransforms);
	m_boneTransforms = nullptr;

	if (m_transitionFrames)
		PPFree(m_transitionFrames);
	m_transitionFrames = nullptr;

	// stop all sequence timers
	for (int i = 0; i < MAX_SEQUENCE_TIMERS; i++)
		m_sequenceTimers[i].Reset();

	m_transitionTime = m_transitionRemTime = DEFAULT_TRANSITION_TIME;

	m_numBones = 0;
}

void CAnimatingEGF::InitAnimating(IEqModel* model)
{
	for (int i = 0; i < MAX_SEQUENCE_TIMERS; i++)
	{
		m_sequenceTimers[i].Reset();
		m_seqBlendWeights[i] = 0.0f;
	}

	if (!model)
		return;

	studiohdr_t* studio = model->GetHWData()->studio;

	m_joints = model->GetHWData()->joints;
	m_numBones = studio->numBones;

	m_boneTransforms = PPAllocStructArray(Matrix4x4, m_numBones);

	for (int i = 0; i < m_numBones; i++)
		m_boneTransforms[i] = m_joints[i].absTrans;

	m_transitionFrames = PPAllocStructArray(animframe_t, m_numBones);
	memset(m_transitionFrames, 0, sizeof(animframe_t)*m_numBones);

	//m_velocityFrames = PPAllocStructArray(animframe_t, m_numBones);
	//memset(m_velocityFrames, 0, sizeof(animframe_t)*m_numBones);

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

			//studioiklink_t* pLink = pStudioChain->pLink(j);

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

	int numMotionPackages = model->GetHWData()->numMotionPackages;

	// build activity table for loaded model
	for (int i = 0; i < numMotionPackages; i++)
	{
		studioHwData_t::motionData_t* motion = model->GetHWData()->motiondata[i];

		AddMotions(motion);
	}
}

int _compareEvents(sequenceevent_t* const &a, sequenceevent_t* const &b)
{
	return a->frame - b->frame;
}

void CAnimatingEGF::AddMotions(studioHwData_t::motionData_t* motionData)
{
	// create pose controllers
	for (int i = 0; i < motionData->numPoseControllers; i++)
	{
		gposecontroller_t controller;
		controller.p = &motionData->poseControllers[i];

		// get center in blending range
		controller.value = lerp(controller.p->blendRange[0], controller.p->blendRange[1], 0.5f);
		controller.interpolatedValue = controller.value;

		m_poseControllers.append(controller);
	}

	// copy sequences
	for (int i = 0; i < motionData->numsequences; i++)
	{
		sequencedesc_t& seq = motionData->sequences[i];

		int seqIdx = m_seqList.numElem();
		m_seqList.setNum(seqIdx+1);

		gsequence_t& seqData = m_seqList[seqIdx];
		memset(&seqData, 0, sizeof(seqData));

		seqData.s = &seq;
		seqData.activity = GetActivityByName(seq.activity);

		if (seqData.activity == ACT_INVALID && stricmp(seq.activity, "ACT_INVALID"))
			MsgError("Motion Data: Activity '%s' not registered\n", seq.activity);

		if (seq.posecontroller >= 0)
			seqData.posecontroller = &m_poseControllers[seq.posecontroller];

		for (int j = 0; j < seq.numAnimations; j++)
			seqData.animations[j] = &motionData->animations[seq.animations[j]];

		for (int j = 0; j < seq.numEvents; j++)
			seqData.events[j] = &motionData->events[seq.events[j]];

		for (int j = 0; j < seq.numSequenceBlends; j++)
			seqData.blends[j] = &m_seqList[seq.sequenceblends[j]];

		// sort events
		quickSort<sequenceevent_t*>(seqData.events, _compareEvents, 0, seq.numEvents - 1);
	}
}

int CAnimatingEGF::FindSequence(const char* name) const
{
	for (int i = 0; i < m_seqList.numElem(); i++)
	{
		if (!stricmp(m_seqList[i].s->name, name))
			return i;
	}

	return -1;
}

int CAnimatingEGF::FindSequenceByActivity(Activity act) const
{
	for (int i = 0; i < m_seqList.numElem(); i++)
	{
		if (m_seqList[i].activity == act)
			return i;
	}

	return -1;
}

// sets animation
void CAnimatingEGF::SetSequence(int seqIdx, int slot)
{
	sequencetimer_t& timer = m_sequenceTimers[slot];

	bool wasEmpty = (timer.seq == nullptr);

	if (slot == 0 && !wasEmpty)
		RecalcBoneTransforms(true);

	gsequence_t* prevSeq = timer.seq;

	// assign sequence and reset playback speed
	// if sequence is not valid, reset it to the Default Pose
	timer.seq = seqIdx >= 0 ? &m_seqList[seqIdx] : nullptr;
	timer.seq_idx = timer.seq ? seqIdx : -1;
	timer.playbackSpeedScale = 1.0f;

	if (slot == 0)
	{
		if (!wasEmpty)
			m_transitionTime = m_transitionRemTime = timer.seq ? timer.seq->s->transitiontime : DEFAULT_TRANSITION_TIME;
		else
			m_transitionTime = m_transitionRemTime = 0.0f;	// if not were used, don't apply transition
	}
}

Activity CAnimatingEGF::TranslateActivity(Activity act, int slot) const
{
	// base class, no translation
	return act;
}

// sets activity
void CAnimatingEGF::SetActivity(Activity act, int slot)
{
	// pick the best activity if available
	Activity nTranslatedActivity = TranslateActivity(act, slot);

	int seqIdx = FindSequenceByActivity(nTranslatedActivity);

	if (seqIdx == -1)
	{
		MsgWarning("Activity \"%s\" not valid!\n", GetActivityName(act));
	}

	SetSequence(seqIdx, slot);

	ResetSequenceTime(slot);
	PlaySequence(slot);
}

// returns current activity
Activity CAnimatingEGF::GetCurrentActivity(int slot)
{
	if (m_sequenceTimers[slot].seq)
		return m_sequenceTimers[slot].seq->activity;

	return ACT_INVALID;
}

// resets animation time, and restarts animation
void CAnimatingEGF::ResetSequenceTime(int slot)
{
	m_sequenceTimers[slot].ResetPlayback();
}

// sets new animation time
void CAnimatingEGF::SetSequenceTime(float newTime, int slot)
{
	m_sequenceTimers[slot].SetTime(newTime);
}

Matrix4x4* CAnimatingEGF::GetBoneMatrices() const
{
	return m_boneTransforms;
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
		return 0.0f;

	return (m_sequenceTimers[0].seq->animations[0]->bones[0].numFrames - 1) / m_sequenceTimers[0].seq->s->framerate;
}

// returns duration time of the specific animation
float CAnimatingEGF::GetAnimationDuration(int animIndex) const
{
	if (animIndex == -1)
		return 0.0f;

	return (m_seqList[animIndex].animations[0]->bones[0].numFrames - 1) / m_seqList[animIndex].s->framerate;
}

// returns remaining duration time of the current animation
float CAnimatingEGF::GetCurrentRemainingAnimationDuration() const
{
	return GetCurrentAnimationDuration() - m_sequenceTimers[0].seq_time;
}

bool CAnimatingEGF::IsSequencePlaying(int slot) const
{
	return m_sequenceTimers[slot].active;
}

// plays/resumes animation
void CAnimatingEGF::PlaySequence(int slot)
{
	m_sequenceTimers[slot].active = true;
}

// stops/pauses animation
void CAnimatingEGF::StopSequence(int slot)
{
	m_sequenceTimers[slot].active = false;
}

void CAnimatingEGF::SetPlaybackSpeedScale(float scale, int slot)
{
	m_sequenceTimers[slot].playbackSpeedScale = scale;
}

void CAnimatingEGF::SetSequenceBlending(int slot, float factor)
{
	m_seqBlendWeights[slot] = factor;
}


// advances frame (and computes interpolation between all blended animations)
void CAnimatingEGF::AdvanceFrame(float frameTime)
{
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

		if (m_sequenceTimers[0].active)
		{
			m_transitionRemTime -= frameTime;
			m_transitionRemTime = max(m_transitionRemTime, 0.0f);
		}
	}

	// update timers and raise events
	for (int i = 0; i < MAX_SEQUENCE_TIMERS; i++)
	{
		sequencetimer_t& timer = m_sequenceTimers[i];

		// for savegame purpose resolving sequences
		if (timer.seq_idx >= 0 && !timer.seq)
			timer.seq = &m_seqList[timer.seq_idx];

		timer.AdvanceFrame(frameTime);

		RaiseSequenceEvents(timer);
	}
}

void CAnimatingEGF::RaiseSequenceEvents(sequencetimer_t& timer)
{
	if (!timer.seq)
		return;

	sequencedesc_t* seqDesc = timer.seq->s;
	int numEvents = seqDesc->numEvents;

	int eventStart = timer.eventCounter;

	for (int i = eventStart; i < numEvents; i++)
	{
		sequenceevent_t* evt = timer.seq->events[i];

		if (timer.seq_time < evt->frame)
			break;

		AnimationEvent event_type = GetEventByName(evt->command);

		if (event_type == EV_INVALID)	// try as event number
			event_type = (AnimationEvent)atoi(evt->command);

		HandleAnimatingEvent(event_type, evt->parameter);

		// to the next
		timer.eventCounter++;
	}
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



int CAnimatingEGF::FindPoseController(char *name)
{
	for (int i = 0; i < m_poseControllers.numElem(); i++)
	{
		if (!stricmp(m_poseControllers[i].p->name, name))
			return i;
	}

	return -1;
}

float CAnimatingEGF::GetPoseControllerValue(int nPoseCtrl) const
{
	if (!m_poseControllers.inRange(nPoseCtrl))
		return 0.0f;

	return m_poseControllers[nPoseCtrl].value;
}

void CAnimatingEGF::SetPoseControllerValue(int nPoseCtrl, float value)
{
	if (!m_poseControllers.inRange(nPoseCtrl))
		return;

	m_poseControllers[nPoseCtrl].value = value;
}

void GetInterpolatedBoneFrame(studioHwData_t::motionData_t::animation_t* pAnim, int nBone, int firstframe, int lastframe, float interp, animframe_t &out)
{
	studioHwData_t::motionData_t::animation_t::boneframe_t& frame = pAnim->bones[nBone];

	InterpolateFrameTransform(frame.keyFrames[firstframe], frame.keyFrames[lastframe], clamp(interp, 0, 1), out);
}

void GetInterpolatedBoneFrameBetweenTwoAnimations(
	studioHwData_t::motionData_t::animation_t* pAnim1,
	studioHwData_t::motionData_t::animation_t* pAnim2,
	int nBone, int firstframe, int lastframe, float interp, float animTransition, animframe_t &out)
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

void GetSequenceLayerBoneFrame(gsequence_t* pSequence, int nBone, animframe_t &out)
{
	float blendWeight = 0;
	int blendAnimation1 = 0;
	int blendAnimation2 = 0;

	ComputeAnimationBlend(pSequence->s->numAnimations,
		pSequence->posecontroller->p->blendRange,
		pSequence->posecontroller->interpolatedValue,
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

// updates bones
void CAnimatingEGF::RecalcBoneTransforms(bool storeTransitionFrames /*= false*/)
{
	m_seqBlendWeights[0] = 1.0f;

	animframe_t finalBoneFrame;

	// setup each bone's transformation
	for (int boneId = 0; boneId < m_numBones; boneId++)
	{
		ZeroFrameTransform(finalBoneFrame);

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

			float frame_interp = timer.seq_time - timer.currFrame;
			frame_interp = min(frame_interp, 1.0f);

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
					ctrl->interpolatedValue,
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

				AddFrameTransform(finalBoneFrame, cTimedFrame, finalBoneFrame);
			}
			else
				InterpolateFrameTransform(finalBoneFrame, cTimedFrame, blend_weight, finalBoneFrame);
		}

		if (storeTransitionFrames)
		{
			m_transitionFrames[boneId] = finalBoneFrame;
		}
		else
		{
			// first sequence timer is main and has transition effects
			if (m_transitionTime > 0.0f && m_transitionRemTime > 0.0f)
			{
				// perform transition based on the last frame
				float transitionLerp = m_transitionRemTime / m_transitionTime;

				InterpolateFrameTransform(finalBoneFrame, m_transitionFrames[boneId], transitionLerp, finalBoneFrame);
			}
		}

		// compute transformation
		Matrix4x4 calculatedFrameMat = CalculateLocalBonematrix(finalBoneFrame);

		// store matrix
		m_boneTransforms[boneId] = (calculatedFrameMat*m_joints[boneId].localTrans);
	}

	// setup each bone's transformation
	for (int i = 0; i < m_numBones; i++)
	{
		if (m_joints[i].parentbone != -1)
			m_boneTransforms[i] = m_boneTransforms[i] * m_boneTransforms[m_joints[i].parentbone];
	}
}

void CAnimatingEGF::DebugRender(const Matrix4x4& worldTransform)
{
	if (!r_debug_skeleton.GetBool())
		return;

	// setup each bone's transformation
	for (int i = 0; i < m_numBones; i++)
	{
		if (r_debug_showbone.GetInt() == i)
		{
			Matrix4x4& transform = m_boneTransforms[i];
			const Vector3D& localPos = transform.rows[3].xyz();
			Vector3D pos = (worldTransform*Vector4D(localPos, 1.0f)).xyz();

			debugoverlay->Text3D(pos, 25, color4_white, 0.0f, "%s\npos: [%.2f %.2f %.2f]", m_joints[i].name, localPos.x, localPos.y, localPos.z);
		}

		if (m_joints[i].parentbone != -1)
		{
			Matrix4x4& transform = m_boneTransforms[i];
			Matrix4x4& parentTransform = m_boneTransforms[m_joints[i].parentbone];

			Vector3D pos = (worldTransform*Vector4D(transform.rows[3].xyz(), 1.0f)).xyz();
			Vector3D parent_pos = (worldTransform*Vector4D(parentTransform.rows[3].xyz(), 1.0f)).xyz();

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

	if (r_debug_ik.GetBool())
	{
		for (int i = 0; i < m_ikChains.numElem(); i++)
		{
			gikchain_t* chain = m_ikChains[i];

			if (!chain->enable)
				continue;

			Vector3D target_pos = chain->local_target;
			target_pos = (worldTransform*Vector4D(target_pos, 1.0f)).xyz();

			debugoverlay->Box3D(target_pos - Vector3D(1), target_pos + Vector3D(1), ColorRGBA(0, 1, 0, 1));

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
}

#define IK_DISTANCE_EPSILON 0.05f

void IKLimitDOF(giklink_t* link)
{
	// FIXME: broken here
	// gimbal lock always occurent
	// better to use quaternions...

	Vector3D euler = eulers(link->quat);

	euler = VRAD2DEG(euler);

	// clamp to this limits
	euler = clamp(euler, link->l->mins, link->l->maxs);

	//euler = NormalizeAngles180(euler);

	// get all back
	euler = VDEG2RAD(euler);

	link->quat = Quaternion(euler.x, euler.y, euler.z);
}

// solves Ik chain
bool SolveIKLinks(giklink_t& effector, Vector3D &target, float fDt, int numIterations = 100)
{
	Vector3D	rootPos, curEnd, targetVector, desiredEnd, curVector, crossResult;

	// start at the last link in the chain
	giklink_t* link = effector.parent;

	int nIter = 0;
	do
	{
		rootPos = link->absTrans.rows[3].xyz();
		curEnd = effector.absTrans.rows[3].xyz();

		desiredEnd = target;
		float dist = distance(curEnd, desiredEnd);

		// check distance
		if (dist > IK_DISTANCE_EPSILON)
		{
			// get directions
			curVector = normalize(curEnd - rootPos);
			targetVector = normalize(desiredEnd - rootPos);

			// get diff cosine between target vector and current bone vector
			float cosAngle = dot(targetVector, curVector);

			// check if we not in zero degrees
			if (cosAngle < 1.0)
			{
				// determine way of rotation
				crossResult = normalize(cross(targetVector, curVector));
				float turnAngle = acosf(cosAngle); // get the angle of dot product

				// damp using time
				turnAngle *= fDt * link->l->damping;

				Quaternion quat(turnAngle, crossResult);

				link->quat = link->quat * quat;

				// check limits
				IKLimitDOF(link);
			}

			if (!link->parent)
				link = effector.parent; // restart
			else
				link = link->parent;
		}
	} while (nIter++ < numIterations && distance(curEnd, desiredEnd) > IK_DISTANCE_EPSILON);

	if (nIter >= numIterations)
		return false;

	return true;
}

// updates inverse kinematics
void CAnimatingEGF::UpdateIK(float fDt, const Matrix4x4& worldTransform)
{
	bool ik_enabled_bones[128];
	memset(ik_enabled_bones, 0, sizeof(ik_enabled_bones));

	// run through bones and find enabled bones by IK chain
	for (int boneId = 0; boneId < m_numBones; boneId++)
	{
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
		}
		else
		{
			// copy last frames to all links
			for (int j = 0; j < chain->numLinks; j++)
			{
				giklink_t& link = chain->links[j];

				int bone_id = link.l->bone;
				studioHwData_t::joint_t& joint = m_joints[link.l->bone];

				link.quat = Quaternion(m_boneTransforms[bone_id].getRotationComponent());
				link.position = joint.position;

				link.localTrans = Matrix4x4(link.quat);
				link.localTrans.setTranslation(link.position);

				// fix local transform for animation
				link.localTrans = joint.localTrans * link.localTrans;
				link.absTrans = m_boneTransforms[bone_id];
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