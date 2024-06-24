//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Animating base
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "core/ConVar.h"
#include "ds/sort.h"
#include "render/IDebugOverlay.h"
#include "Animating.h"
#include "studio/StudioGeom.h"
#include "studio/StudioCache.h"
#include "anim_activity.h"
#include "anim_events.h"

DECLARE_CVAR(r_debugIK, "0", "Draw debug information about Inverse Kinematics", CV_CHEAT);
DECLARE_CVAR(r_debugSkeleton, "0", "Draw debug information about bones", CV_CHEAT);
DECLARE_CVAR(r_debugShowBone, "-1", "Shows the bone", CV_CHEAT);
DECLARE_CVAR(r_ikIterations, "100", "IK link iterations per update", CV_ARCHIVE);

constexpr int ANIM_SEQUENCE_ID_BITS		= 24;
constexpr int ANIM_DATA_ID_BITS			= 8;

constexpr int ANIM_ACTIVITY_ID_BITS		= 24;
constexpr int ANIM_SLOT_ID_BITS			= 8;

constexpr int ANIM_MAX_ACTIVITIES		= (1 << ANIM_ACTIVITY_ID_BITS);
constexpr int ANIM_MAX_SEQUENCES		= (1 << ANIM_SEQUENCE_ID_BITS);

static Matrix4x4 CalculateLocalBonematrix(const AnimFrame& frame)
{
	Matrix4x4 bonetransform(frame.angBoneAngles);
	bonetransform.setTranslation(frame.vecBonePosition);

	return bonetransform;
}

// computes blending animation index and normalized weight
static void ComputeAnimationBlend(int numWeights, const float blendrange[2], float blendValue, float& blendWeight, int& blendMainAnimation1, int& blendMainAnimation2)
{
	blendValue = clamp(blendValue, blendrange[0], blendrange[1]);

	// convert to value in range 0..1.
	const float actualBlendValue = (blendValue - blendrange[0]) / (blendrange[1] - blendrange[0]);

	// compute animation index
	const float normalizedBlend = actualBlendValue * (float)(numWeights - 1);
	const int blendMainAnimation = (int)normalizedBlend;

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

// adds transform
static AnimFrame AddFrameTransform(const AnimFrame& frame1, const AnimFrame& frame2)
{
	AnimFrame out;
	out.angBoneAngles = frame1.angBoneAngles * frame2.angBoneAngles;
	out.angBoneAngles.fastNormalize();
	out.vecBonePosition = frame1.vecBonePosition + frame2.vecBonePosition;

	return out;
}

// interpolates frame transform
static AnimFrame InterpolateFrameTransform(const AnimFrame& frame1, const AnimFrame& frame2, float value)
{
	AnimFrame out;
	out.angBoneAngles = slerp(frame1.angBoneAngles, frame2.angBoneAngles, value);
	out.vecBonePosition = lerp(frame1.vecBonePosition, frame2.vecBonePosition, value);
	return out;
}

static AnimFrame GetInterpolatedBoneFrame(const AnimFrameData& anim, int nBone, int firstframe, int lastframe, float interp)
{
	const animframe_t* boneFrame = &anim.keyFrames[nBone * anim.numFrames];

	ASSERT(firstframe >= 0);
	ASSERT(lastframe >= 0);
	ASSERT(firstframe < anim.numFrames);
	ASSERT(lastframe < anim.numFrames);
	return InterpolateFrameTransform(boneFrame[firstframe], boneFrame[lastframe], interp);
}

static AnimFrame GetInterpolatedBoneFrameBetweenTwoAnimations(
	const AnimFrameData& anim1, const AnimFrameData& anim2,
	int boneIdx, int firstframe, int lastframe, float interp, float animTransition)
{
	AnimFrame frameA = GetInterpolatedBoneFrame(anim1, boneIdx, firstframe, lastframe, interp);
	AnimFrame frameB = GetInterpolatedBoneFrame(anim2, boneIdx, firstframe, lastframe, interp);

	// resultative interpolation
	return InterpolateFrameTransform(frameA, frameB, animTransition);
}

static AnimFrame GetSequenceLayerBoneFrame(const AnimSequence* seq, int boneIdx)
{
	float blendWeight = 0;
	int blendAnimation1 = 0;
	int blendAnimation2 = 0;

	ComputeAnimationBlend(seq->desc->numAnimations,
		seq->posecontroller->desc->blendRange,
		seq->posecontroller->interpolatedValue,
		blendWeight,
		blendAnimation1,
		blendAnimation2
	);

	const AnimFrameData* anim1 = seq->animations[blendAnimation1];
	const AnimFrameData* anim2 = seq->animations[blendAnimation2];

	return GetInterpolatedBoneFrameBetweenTwoAnimations(*anim1, *anim2, boneIdx, 0, 0, 0, blendWeight);
}

static void CalculateSequenceBoneFrame(const AnimSequenceTimer& timer, int boneId, AnimFrame& inOutBoneFrame)
{
	// if no animation plays on this timer, continue
	if (!timer.seq)
		return;

	if (timer.blendWeight <= 0)
		return;

	const AnimSequence* seq = timer.seq;
	const AnimFrameData* curAnim = seq->animations[0];
	if (!curAnim)
		return;

	const sequencedesc_t* seqDesc = seq->desc;
	const int numAnims = seqDesc->numAnimations;
	const int numSeqBlends = seqDesc->numSequenceBlends;

	// the computed frame
	AnimFrame animationFrame;

	const float frameInterp = min(timer.seqTime - timer.currFrame, 1.0f);

	// blend between many animations in sequence using pose controller
	if (numAnims > 1 && seq->posecontroller)
	{
		AnimPoseController* ctrl = seq->posecontroller;

		float playingBlendWeight = 0;
		int playingBlendAnimation1 = 0;
		int playingBlendAnimation2 = 0;

		// get frame indexes and lerp value of blended animation
		ComputeAnimationBlend(numAnims,
			ctrl->desc->blendRange,
			ctrl->interpolatedValue,
			playingBlendWeight,
			playingBlendAnimation1,
			playingBlendAnimation2);

		// get frame pointers
		const AnimFrameData* playingAnim1 = seq->animations[playingBlendAnimation1];
		const AnimFrameData* playingAnim2 = seq->animations[playingBlendAnimation2];

		// compute blending frame
		animationFrame = GetInterpolatedBoneFrameBetweenTwoAnimations(*playingAnim1, *playingAnim2, boneId, timer.currFrame, timer.nextFrame, frameInterp, playingBlendWeight);
	}
	else
	{
		// simply compute frames
		animationFrame = GetInterpolatedBoneFrame(*curAnim, boneId, timer.currFrame, timer.nextFrame, frameInterp);
	}

	AnimFrame seqBlendsFrame;
	for (int blendSeqIdx = 0; blendSeqIdx < numSeqBlends; blendSeqIdx++)
	{
		// get bone frame of layer
		AnimFrame seqFrame = GetSequenceLayerBoneFrame(seq->blends[blendSeqIdx], boneId);
		seqBlendsFrame = AddFrameTransform(seqBlendsFrame, seqFrame);
	}

	AnimFrame animFrameBlend = AddFrameTransform(animationFrame, seqBlendsFrame);

	// interpolate or add the slots, this is useful for body part splitting
	if (seqDesc->flags & SEQFLAG_SLOTBLEND)
	{
		// TODO: check if that incorrect since we've switched to quaternions
		animFrameBlend.angBoneAngles *= timer.blendWeight;
		animFrameBlend.vecBonePosition *= timer.blendWeight;

		inOutBoneFrame = AddFrameTransform(inOutBoneFrame, animFrameBlend);
	}
	else
		inOutBoneFrame = InterpolateFrameTransform(inOutBoneFrame, animFrameBlend, timer.blendWeight);
}

//-----------------------------------------------------------

CAnimatingEGF::CAnimatingEGF()
{
	m_sequenceTimers.setNum(m_sequenceTimers.numAllocated());
	m_transitionTimers.setNum(m_transitionTimers.numAllocated());
}

CAnimatingEGF::~CAnimatingEGF()
{
	SAFE_DELETE_ARRAY(m_boneTransforms);
}

void CAnimatingEGF::DestroyAnimating()
{
	m_sequenceTimers.clear();
	m_transitionTimers.clear();

	m_animData.clear(true);
	m_nameToAnimData.clear(true);
	m_ikChains.clear(true);
	m_poseControllers.clear(true);
	m_joints = ArrayCRef<StudioJoint>(nullptr);
	m_transforms = ArrayCRef<studioTransform_t>(nullptr);

	SAFE_DELETE_ARRAY(m_boneTransforms);

	m_sequenceTimers.setNum(m_sequenceTimers.numAllocated());
	m_transitionTimers.setNum(m_transitionTimers.numAllocated());

	m_geomReference = nullptr;
}

void CAnimatingEGF::InitAnimating(CEqStudioGeom* model)
{
	DestroyAnimating();
	m_geomReference = model;

	if (!model)
		return;

	const studioHdr_t& studio = model->GetStudioHdr();

	m_joints = model->GetJoints();
	m_transforms = ArrayCRef(model->GetStudioHdr().pTransform(0), model->GetStudioHdr().numTransforms);

	m_bonesNeedUpdate = TRUE;
	m_boneTransforms = PPNew Matrix4x4[m_joints.numElem()];
	for (int i = 0; i < m_joints.numElem(); i++)
		m_boneTransforms[i] = m_joints[i].absTrans;

	// init ik chains
	const int numIkChains = studio.numIKChains;
	for (int i = 0; i < numIkChains; i++)
	{
		const studioIkChain_t* studioChain = studio.pIkChain(i);

		AnimIkChain& chain = m_ikChains.append();
		chain.links = PPNewArrayRef(AnimIkChain::Link, studioChain->numLinks);
		chain.desc = studioChain;
		chain.enable = false;

		for (int j = 0; j < chain.links.numElem(); j++)
		{
			AnimIkChain::Link& link = chain.links[j];
			link.desc = studioChain->pLink(j);
			link.chain = &chain;

			const StudioJoint& joint = m_joints[link.desc->bone];

			const Vector3D& rotation = joint.bone->rotation;
			link.quat = rotateXYZ(rotation.x, rotation.y, rotation.z);
			link.position = joint.bone->position;

			// initial transform
			link.localTrans = Matrix4x4(link.quat);
			link.localTrans.setTranslation(link.position);
			link.absTrans = identity4;

			const int parentLinkId = j - 1;

			if (parentLinkId >= 0)
				link.parent = &chain.links[j - 1];
			else
				link.parent = nullptr;

		}

		for (AnimIkChain::Link& link : chain.links)
		{
			if (link.parent)
				link.absTrans = link.localTrans * link.parent->absTrans;
			else
				link.absTrans = link.localTrans;
		}
	}

	for (const int motionDataIdx : m_geomReference->GetMotionDataIdxs())
		AddMotionData(g_studioCache->GetMotionData(motionDataIdx));
}

void CAnimatingEGF::InitMotionData(const char* name)
{
	const int motionDataIdx = g_studioCache->PrecacheMotionData(name);
	if (motionDataIdx == -1)
		return;

	const StudioMotionData* motionData = g_studioCache->GetMotionData(motionDataIdx);
	AddMotionData(motionData);
}

bool CAnimatingEGF::AddMotionData(const StudioMotionData* motionData)
{
	if (!motionData)
		return false;

	const int animDataId = StringToHash(motionData->name, true);

	if (!m_nameToAnimData.find(animDataId).atEnd())
		return false;

	m_nameToAnimData.insert(animDataId, m_animData.numElem());

	AnimDataProvider& animData = m_animData.append();
	animData.motionData = motionData;

	// create pose controllers
	// they are shared between all motion datas and must be setup equal
	for (const posecontroller_t& poseCtrl : motionData->poseControllers)
	{
		const int existingIdx = arrayFindIndexF(m_poseControllers, [&poseCtrl](const AnimPoseController& ctrl) {
			return CString::CompareCaseIns(ctrl.desc->name, poseCtrl.name) == 0;
			});

		if (existingIdx != -1)
		{
			const posecontroller_t& existingCtrl = *m_poseControllers[existingIdx].desc;
			if (existingCtrl.blendRange[0] == poseCtrl.blendRange[0]
				&& existingCtrl.blendRange[1] == poseCtrl.blendRange[1])
			{
				MsgWarning("%s warning: pose controller %s was added from another package but blend ranges are mismatched", m_geomReference->GetName(), poseCtrl.name);
			}
			continue;
		}

		// get center in blending range
		const float defaultValue = lerp(poseCtrl.blendRange[0], poseCtrl.blendRange[1], 0.5f);;

		AnimPoseController& controller = m_poseControllers.append();
		controller.desc = &poseCtrl;
		controller.value = defaultValue;
		controller.interpolatedValue = defaultValue;
	}

	// create animations
	const int numBones = m_geomReference->GetStudioHdr().numBones;
	animData.animations.reserve(motionData->animations.numElem());
	for (const animationdesc_t& animDesc : motionData->animations)
	{
		AnimFrameData& anmData = animData.animations.append();
		anmData.desc = &animDesc;
		anmData.numFrames = animDesc.numFrames / numBones;
		anmData.keyFrames = &motionData->frames[animDesc.firstFrame];
	}

	// create sequences
	animData.sequences.reserve(motionData->sequences.numElem());
	for (const sequencedesc_t& seqDesc : motionData->sequences)
	{
		const int sequenceIdx = animData.sequences.numElem();
		animData.nameToSequence.insert(StringToHash(seqDesc.name, true), sequenceIdx);

		AnimSequence& seqData = animData.sequences.append();
		seqData.desc = &seqDesc;
		seqData.motionData = motionData;
		seqData.activity = GetActivityByName(seqDesc.activity);

		if (seqData.activity == ACT_INVALID)
		{
			if(CString::Compare(seqDesc.activity, "ACT_INVALID"))
				MsgError("Motion Data: Activity '%s' not registered\n", seqDesc.activity);
		}
		else
		{
			animData.activitySlotSequenceIds.insert(seqData.activity | (static_cast<int>(seqDesc.activityslot) << ANIM_ACTIVITY_ID_BITS), sequenceIdx);
		}

		// assign pose controller
		if (seqDesc.posecontroller >= 0)
		{
			const posecontroller_t& mopPoseCtrl = motionData->poseControllers[seqDesc.posecontroller];
			const int poseCtrlIdx = arrayFindIndexF(m_poseControllers, [&](const AnimPoseController& ctrl) {
				return CString::CompareCaseIns(ctrl.desc->name, mopPoseCtrl.name) == 0;
			});
			ASSERT(poseCtrlIdx != -1);
			seqData.posecontroller = &m_poseControllers[poseCtrlIdx];
		}

		for (int i = 0; i < seqDesc.numAnimations; i++)
			seqData.animations[i] = &animData.animations[seqDesc.animations[i]];

		for (int i = 0; i < seqDesc.numEvents; i++)
			seqData.events[i] = &motionData->events[seqDesc.events[i]];

		for (int i = 0; i < seqDesc.numSequenceBlends; i++)
			seqData.blends[i] = &animData.sequences[seqDesc.sequenceblends[i]];

		// sort events
		auto compareEvents = [](const sequenceevent_t* a, const sequenceevent_t* b) -> int
		{
			return sortCompare(a->frame, b->frame);
		};

		arraySort(seqData.events, compareEvents, 0, seqDesc.numEvents - 1);
	}

	return true;
}

int CAnimatingEGF::FindSequence(const char* name) const
{
	EqStringRef seqName = name;
	const int dotIdx = seqName.Find(".");
	if (dotIdx == -1)
	{
		ASSERT_FAIL("'%s' - sequence name is not valid (it must be formatted as 'package.sequence')", name);
		return -1;
	}

	const EqString motionDataName = seqName.Left(dotIdx);
	const EqString sequenceName = seqName.Right(seqName.Length() - dotIdx - 1);

	const auto animDataIdxIt = m_nameToAnimData.find(StringToHash(motionDataName, true));
	if (animDataIdxIt.atEnd())
	{
		MsgWarning("Warning: '%s' - motion package '%s' is not referenced by '%s'!\n", name, motionDataName.ToCString(), m_geomReference->GetName());
		return -1;
	}

	const AnimDataProvider& animData = m_animData[*animDataIdxIt];
	const auto sequenceIdxIt = animData.nameToSequence.find(StringToHash(sequenceName, true));
	if (sequenceIdxIt.atEnd())
	{
		MsgWarning("Warning: '%s' - Sequence '%s' not found in motion package '%s' referenced by '%s'!\n", name, sequenceName.ToCString(), motionDataName.ToCString(), m_geomReference->GetName());
		return -1;
	}

	return *sequenceIdxIt | (*animDataIdxIt << ANIM_SEQUENCE_ID_BITS);
}

int CAnimatingEGF::FindSequenceByActivity(Activity act, int slot) const
{
	const int activitySlotId = act | (slot << ANIM_ACTIVITY_ID_BITS);

	for (int i = 0; i < m_animData.numElem(); ++i)
	{
		const AnimDataProvider& animData = m_animData[i];
		const auto seqIdIt = animData.activitySlotSequenceIds.find(activitySlotId);
		if (seqIdIt.atEnd())
			continue;

		return *seqIdIt | (i << ANIM_SEQUENCE_ID_BITS);
	}

	MsgWarning("No sequence matching activity %s (slot %d) for '%s'!\n", GetActivityName(act), slot, m_geomReference->GetName());

	return -1;
}

const AnimSequence* CAnimatingEGF::GetSequenceById(int sequenceId) const
{
	if (sequenceId == -1)
		return nullptr;

	const int animDataIdx = (sequenceId >> ANIM_SEQUENCE_ID_BITS);
	const int sequenceIdx = sequenceId & (ANIM_MAX_ACTIVITIES - 1);

	return &m_animData[animDataIdx].sequences[sequenceIdx];
}

// sets animation
void CAnimatingEGF::SetSequence(int sequenceId, int slot)
{
	AnimSequenceTimer& timer = m_sequenceTimers[slot];
	AnimSequenceTimer oldTimer = timer;

	// assign sequence and reset playback speed
	// if sequence is not valid, reset it to the Default Pose
	timer.seq = GetSequenceById(sequenceId);
	timer.seqId = sequenceId;
	timer.playbackSpeedScale = 1.0f;

	AnimSequenceTimer& transitionTimer = m_transitionTimers[slot];
	if (transitionTimer.transitionRemainingTime > 0.0f && transitionTimer.seq)
		return;

	if (oldTimer.seq == nullptr)
	{
		transitionTimer.transitionTime = 0.0f;
		transitionTimer.transitionRemainingTime = 0.0f;
	}
	else
	{
		transitionTimer = oldTimer;

		transitionTimer.transitionTime = timer.seq ? timer.seq->desc->transitiontime : SEQ_DEFAULT_TRANSITION_TIME;
		transitionTimer.transitionRemainingTime = transitionTimer.transitionTime;	
	}
}

Activity CAnimatingEGF::TranslateActivity(Activity act, int slot) const
{
	// base class, no translation
	return act;
}

void CAnimatingEGF::HandleAnimatingEvent(AnimationEvent nEvent, const char* options)
{
	// do nothing
}

// sets activity
void CAnimatingEGF::SetActivity(Activity act, int slot)
{
	if(!m_geomReference)
		return;

	// pick the best activity if available
	const Activity newActivity = TranslateActivity(act, slot);

	const int sequenceId = FindSequenceByActivity(newActivity, slot);
	SetSequence(sequenceId, slot);

	ResetSequenceTime(slot);
	PlaySequence(slot);
}

void CAnimatingEGF::SetSequenceByName(const char* name, int slot)
{
	if(!m_geomReference)
		return;

	const int sequenceId = FindSequence(name);
	SetSequence(sequenceId, slot);

	ResetSequenceTime(slot);
	PlaySequence(slot);
}

// returns current activity
Activity CAnimatingEGF::GetCurrentActivity(int slot) const
{
	if (!m_sequenceTimers[slot].seq)
		return ACT_INVALID;

	return m_sequenceTimers[slot].seq->activity;
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
	for (int i = 0; i < m_joints.numElem(); i++)
	{
		if (!CString::CompareCaseIns(m_joints[i].bone->name, boneName))
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

bool CAnimatingEGF::IsTransitionCompleted(int slot) const
{
	return m_transitionTimers[slot].transitionRemainingTime <= 0.0f;
}

int CAnimatingEGF::GetCurrentAnimationFrameCount(int slot) const
{
	const AnimSequence* seq = m_sequenceTimers[slot].seq;
	if (!seq)
		return 0;

	return seq->animations[0]->numFrames;
}

// returns duration time of the current animation
float CAnimatingEGF::GetCurrentAnimationDuration(int slot) const
{
	const AnimSequence* seq = m_sequenceTimers[slot].seq;
	if (!seq)
		return 0.0f;

	return seq->animations[0]->numFrames / seq->desc->framerate;
}

// returns elapsed time of the current animation
float CAnimatingEGF::GetCurrentAnimationTime(int slot) const
{
	const AnimSequence* seq = m_sequenceTimers[slot].seq;
	if (!seq)
		return 0.0f;

	return m_sequenceTimers[slot].seqTime / seq->desc->framerate;
}

// returns remaining duration time of the current animation
float CAnimatingEGF::GetCurrentAnimationRemainingDuration(int slot) const
{
	return GetCurrentAnimationDuration(slot) - GetCurrentAnimationTime(slot);
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

void CAnimatingEGF::SetSequenceBlending(float factor, int slot)
{
	m_sequenceTimers[slot].blendWeight = factor;
}

// advances frame (and computes interpolation between all blended animations)
void CAnimatingEGF::AdvanceFrame(float frameTime)
{
	uint didUpdate = FALSE;

	if (m_sequenceTimers[0].seq)
	{
		const float div_frametime = (frameTime * 30);

		// interpolate pose parameter values
		for(AnimPoseController& ctrl : m_poseControllers)
		{
			const float diff = ctrl.value - ctrl.interpolatedValue;
			if(abs(diff) > F_EPS)
				didUpdate = TRUE;

			ctrl.interpolatedValue = approachValue(ctrl.interpolatedValue, ctrl.value, div_frametime * diff);
		}
	}

	for (AnimSequenceTimer& transitionTimer : m_transitionTimers)
	{
		const float oldTransitionTime = transitionTimer.transitionRemainingTime;
		const float newTransitionTime = max(oldTransitionTime - frameTime, 0.0f);

		if (abs(newTransitionTime - oldTransitionTime) > F_EPS)
			didUpdate = TRUE;

		transitionTimer.transitionRemainingTime = newTransitionTime;
	}

	// update timers and raise events
	for (AnimSequenceTimer& timer : m_sequenceTimers)
	{
		// for savegame purpose resolving sequences
		if (timer.seqId >= 0 && !timer.seq)
			timer.seq = GetSequenceById(timer.seqId);

		//if (timer.active)
			didUpdate = TRUE;

		timer.AdvanceFrame(frameTime);
		RaiseSequenceEvents(timer);
	}

	Atomic::CompareExchange(m_bonesNeedUpdate, FALSE, didUpdate);
}

void CAnimatingEGF::RaiseSequenceEvents(AnimSequenceTimer& timer)
{
	if (!timer.seq)
		return;

	const sequencedesc_t* seqDesc = timer.seq->desc;
	const int numEvents = seqDesc->numEvents;
	const int eventStart = timer.eventCounter;

	for (int i = timer.eventCounter; i < numEvents; i++)
	{
		const sequenceevent_t* evt = timer.seq->events[i];

		if (timer.seqTime < evt->frame)
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
void CAnimatingEGF::SwapSequenceTimers(int slotFrom, int swapTo)
{
	AnimSequenceTimer swap_to = m_sequenceTimers[swapTo];
	m_sequenceTimers[swapTo] = m_sequenceTimers[slotFrom];
	m_sequenceTimers[slotFrom] = swap_to;
}

int CAnimatingEGF::FindPoseController(const char* name) const
{
	const int existingPoseCtrlIdx = arrayFindIndexF(m_poseControllers, [name](const AnimPoseController& ctrl) {
		return CString::CompareCaseIns(ctrl.desc->name, name) == 0;
	});
	return existingPoseCtrlIdx;
}

float CAnimatingEGF::GetPoseControllerValue(int nPoseCtrl) const
{
	if (!m_poseControllers.inRange(nPoseCtrl))
		return 0.0f;

	return m_poseControllers[nPoseCtrl].value;
}

float CAnimatingEGF::GetPoseControllerInterpValue(int nPoseCtrl) const
{
	if (!m_poseControllers.inRange(nPoseCtrl))
		return 0.0f;

	return m_poseControllers[nPoseCtrl].interpolatedValue;
}

void CAnimatingEGF::SetPoseControllerValue(int nPoseCtrl, float value)
{
	if (!m_poseControllers.inRange(nPoseCtrl))
		return;

	m_poseControllers[nPoseCtrl].value = value;
}

void CAnimatingEGF::GetPoseControllerRange(int nPoseCtrl, float& rMin, float& rMax) const
{
	if (!m_poseControllers.inRange(nPoseCtrl))
	{
		rMin = 0.0f;
		rMax = 1.0f;
		return;
	}

	const AnimPoseController& poseCtrl = m_poseControllers[nPoseCtrl];
	rMin = poseCtrl.desc->blendRange[0];
	rMax = poseCtrl.desc->blendRange[1];
}

// updates bones
bool CAnimatingEGF::RecalcBoneTransforms()
{
	if (!m_boneTransforms)
		return false;

	if (Atomic::CompareExchange(m_bonesNeedUpdate, TRUE, FALSE) != TRUE)
		return false;

	// FIXME: do we really need this hack?
	m_sequenceTimers[0].blendWeight = 1.0f;

	// setup each bone's transformation
	for (int boneId = 0; boneId < m_joints.numElem(); boneId++)
	{
		AnimFrame finalBoneFrame;
		for (int i = 0; i < m_sequenceTimers.numElem(); ++i)
		{
			AnimFrame timerFrame;
			CalculateSequenceBoneFrame(m_sequenceTimers[i], boneId, timerFrame);

			const AnimSequenceTimer& transitionTimer = m_transitionTimers[i];
			if (transitionTimer.transitionRemainingTime > 0.0f && transitionTimer.transitionTime > F_EPS)
			{
				// mix in the transition
				AnimFrame transitionFrame;
				CalculateSequenceBoneFrame(transitionTimer, boneId, transitionFrame);

				const float transitionFactor = transitionTimer.transitionRemainingTime / transitionTimer.transitionTime;
				timerFrame = InterpolateFrameTransform(timerFrame, transitionFrame, transitionFactor);
			}

			finalBoneFrame = AddFrameTransform(finalBoneFrame, timerFrame);
		}

		ASSERT(!V3IsNaN(finalBoneFrame.vecBonePosition));
		ASSERT(!V3IsNaN(finalBoneFrame.angBoneAngles));

		// compute transformation
		const Matrix4x4 calculatedFrameMat = CalculateLocalBonematrix(finalBoneFrame);
		m_boneTransforms[boneId] = (calculatedFrameMat * m_joints[boneId].localTrans);
	}

	// setup each bone's transformation
	for (int i = 0; i < m_joints.numElem(); i++)
	{
		const int parentIdx = m_joints[i].parent;
		if (parentIdx != -1)
			m_boneTransforms[i] = m_boneTransforms[i] * m_boneTransforms[parentIdx];
	}

	return true;
}

Matrix4x4 CAnimatingEGF::GetLocalStudioTransformMatrix(int attachmentIdx) const
{
	const studioTransform_t* attach = &m_transforms[attachmentIdx];

	if (attach->attachBoneIdx != EGF_INVALID_IDX)
		return attach->transform * m_boneTransforms[attach->attachBoneIdx];

	return attach->transform;
}

void CAnimatingEGF::DebugRender(const Matrix4x4& worldTransform)
{
#ifdef ENABLE_DEBUG_DRAWING
	if (!r_debugSkeleton.GetBool())
		return;

	// setup each bone's transformation
	for (int i = 0; i < m_joints.numElem(); i++)
	{
		if (r_debugShowBone.GetInt() == i)
		{
			const Matrix4x4& transform = m_boneTransforms[i];
			const Vector3D& localPos = transform.rows[3].xyz();
			const Vector3D pos = inverseTransformPoint(localPos, worldTransform);

			debugoverlay->Text3D(pos, 25, color_white, EqString::Format("%s\npos: [%.2f %.2f %.2f]", m_joints[i].bone->name, localPos.x, localPos.y, localPos.z));
		}

		const Matrix4x4& transform = m_boneTransforms[i];
		const Vector3D pos = inverseTransformPoint(transform.rows[3].xyz(), worldTransform);

		if (m_joints[i].parent != -1)
		{
			const Matrix4x4& parentTransform = m_boneTransforms[m_joints[i].parent];
			const Vector3D parent_pos = inverseTransformPoint(parentTransform.rows[3].xyz(), worldTransform);
			debugoverlay->Line3D(pos, parent_pos, color_white, color_white);
		}

		const Vector3D dX = worldTransform.getRotationComponent() * transform.rows[0].xyz() * 0.25f;
		const Vector3D dY = worldTransform.getRotationComponent() * transform.rows[1].xyz() * 0.25f;
		const Vector3D dZ = worldTransform.getRotationComponent() * transform.rows[2].xyz() * 0.25f;

		// draw axis
		debugoverlay->Line3D(pos, pos + dX, ColorRGBA(1, 0, 0, 1), ColorRGBA(1, 0, 0, 1));
		debugoverlay->Line3D(pos, pos + dY, ColorRGBA(0, 1, 0, 1), ColorRGBA(0, 1, 0, 1));
		debugoverlay->Line3D(pos, pos + dZ, ColorRGBA(0, 0, 1, 1), ColorRGBA(0, 0, 1, 1));
	}

	if (r_debugIK.GetBool())
	{
		for (const AnimIkChain& chain : m_ikChains)
		{
			if (!chain.enable)
				continue;

			Vector3D target_pos = inverseTransformPoint(chain.localTarget, worldTransform);

			debugoverlay->Box3D(target_pos - Vector3D(1), target_pos + Vector3D(1), ColorRGBA(0, 1, 0, 1));

			for (const AnimIkChain::Link& link : chain.links)
			{
				const Vector3D& bone_pos = link.absTrans.rows[3].xyz();
				Vector3D parent_pos = inverseTransformPoint(link.parent->absTrans.rows[3].xyz(), worldTransform);

				Vector3D dX = worldTransform.getRotationComponent() * link.absTrans.rows[0].xyz();
				Vector3D dY = worldTransform.getRotationComponent() * link.absTrans.rows[1].xyz();
				Vector3D dZ = worldTransform.getRotationComponent() * link.absTrans.rows[2].xyz();

				debugoverlay->Line3D(parent_pos, bone_pos, ColorRGBA(1, 1, 0, 1), ColorRGBA(1, 1, 0, 1));
				debugoverlay->Box3D(bone_pos + Vector3D(1), bone_pos - Vector3D(1), ColorRGBA(1, 0, 0, 1));
				debugoverlay->Text3D(bone_pos, 200.0f, color_white, m_joints[link.desc->bone].bone->name);

				// draw axis
				debugoverlay->Line3D(bone_pos, bone_pos + dX, ColorRGBA(1, 0, 0, 1), ColorRGBA(1, 0, 0, 1));
				debugoverlay->Line3D(bone_pos, bone_pos + dY, ColorRGBA(0, 1, 0, 1), ColorRGBA(0, 1, 0, 1));
				debugoverlay->Line3D(bone_pos, bone_pos + dZ, ColorRGBA(0, 0, 1, 1), ColorRGBA(0, 0, 1, 1));
			}
		}
	}
#endif // ENABLE_DEBUG_DRAWING
}

#define IK_DISTANCE_EPSILON 0.05f

static void IKLimitDOF(AnimIkChain::Link* link)
{
	// FIXME: broken here
	// gimbal lock always occurent
	// better to use quaternions...

	Vector3D euler = eulersXYZ(link->quat);

	euler = RAD2DEG(euler);

	// clamp to this limits
	euler = clamp(euler, link->desc->mins, link->desc->maxs);

	//euler = NormalizeAngles180(euler);

	// get all back
	euler = DEG2RAD(euler);

	link->quat = rotateXYZ(euler.x, euler.y, euler.z);
}

// solves Ik chain
static bool SolveIKLinks(AnimIkChain::Link& effector, const Vector3D& target, float fDt, int numIterations = 100)
{
	Vector3D	rootPos, curEnd, targetVector, desiredEnd, curVector, crossResult;

	// start at the last link in the chain
	AnimIkChain::Link* link = effector.parent;

	int nIter = 0;
	do
	{
		rootPos = link->absTrans.rows[3].xyz();
		curEnd = effector.absTrans.rows[3].xyz();

		desiredEnd = target;
		const float dist = distanceSqr(curEnd, desiredEnd);

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
				turnAngle *= fDt * link->desc->damping;

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
	} while (nIter++ < numIterations && distanceSqr(curEnd, desiredEnd) > IK_DISTANCE_EPSILON);

	if (nIter >= numIterations)
		return false;

	return true;
}

// updates inverse kinematics
void CAnimatingEGF::UpdateIK(float fDt, const Matrix4x4& worldTransform)
{
	if (!m_ikChains.numElem())
		return;

	bool ik_enabled_bones[128];
	memset(ik_enabled_bones, 0, sizeof(ik_enabled_bones));

	// run through bones and find enabled bones by IK chain
	for (int boneId = 0; boneId < m_joints.numElem(); boneId++)
	{
		const int chain_id = m_joints[boneId].ikChainId;
		const int link_id = m_joints[boneId].ikLinkId;

		if (link_id != -1 && chain_id != -1 && m_ikChains[chain_id].enable)
		{
			const AnimIkChain& chain = m_ikChains[chain_id];

			for (const AnimIkChain::Link& link : chain.links)
			{
				if (link.desc->bone == boneId)
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
		AnimIkChain& chain = m_ikChains[i];
		if (chain.enable)
		{
			// display target
			if (r_debugIK.GetBool())
			{
				const Vector3D target_pos = inverseTransformPoint(chain.localTarget, worldTransform);
				debugoverlay->Box3D(target_pos - Vector3D(1), target_pos + Vector3D(1), ColorRGBA(0, 1, 0, 1));
			}

			// update chain
			UpdateIkChain(chain, fDt);
		}
		else
		{
			// copy last frames to all links
			for (AnimIkChain::Link& link : chain.links)
			{
				const int boneIdx = link.desc->bone;
				const StudioJoint& joint = m_joints[boneIdx];

				link.quat = Quaternion(m_boneTransforms[boneIdx].getRotationComponent());
				link.position = joint.bone->position;

				link.localTrans = Matrix4x4(link.quat);
				link.localTrans.setTranslation(link.position);

				// fix local transform for animation
				link.localTrans = joint.localTrans * link.localTrans;
				link.absTrans = m_boneTransforms[boneIdx];
			}
		}
	}

	Atomic::Exchange(m_bonesNeedUpdate, TRUE);
}

// solves single ik chain
void CAnimatingEGF::UpdateIkChain(AnimIkChain& chain, float fDt)
{
	for (AnimIkChain::Link& link : chain.links)
	{
		// this is broken
		link.localTrans = Matrix4x4(link.quat);
		link.localTrans.setTranslation(link.position);

		//int boneIdx = link.bone_index;
		//link.localTrans = m_joints[boneIdx].localTrans * link.localTrans;
	}

	for (AnimIkChain::Link& link : chain.links)
	{
		if (link.parent)
		{
			link.absTrans = link.localTrans * link.parent->absTrans;

			//link.absTrans = !link.localTrans * link.absTrans;

			// FIXME:	INVALID CALCULATIONS HAPPENED HERE
			//			REWORK IK!!!
			m_boneTransforms[link.desc->bone] = link.absTrans;
		}
		else // just copy transform
		{
			link.absTrans = m_boneTransforms[link.desc->bone];
		}
	}

	// use last bone for movement
	// TODO: use more points to solve to do correct IK
	SolveIKLinks(chain.links.back(), chain.localTarget, fDt, r_ikIterations.GetInt());
}

// inverse kinematics

void CAnimatingEGF::SetIKWorldTarget(int chain_id, const Vector3D& world_position, const Matrix4x4& worldTransform)
{
	if (chain_id == -1)
		return;

	Matrix4x4 world_to_model = transpose(worldTransform);

	Vector3D local = world_position;
	local = inverseTranslateVector(local, world_to_model); // could be invalid since we've changed inverseTranslateVec
	local = inverseRotateVector(local, world_to_model);

	SetIKLocalTarget(chain_id, local);
}

void CAnimatingEGF::SetIKLocalTarget(int chain_id, const Vector3D& local_position)
{
	if (chain_id == -1)
		return;

	m_ikChains[chain_id].localTarget = local_position;
}

void CAnimatingEGF::SetIKChainEnabled(int chain_id, bool enabled)
{
	if (chain_id == -1)
		return;

	m_ikChains[chain_id].enable = enabled;
}

bool CAnimatingEGF::IsIKChainEnabled(int chain_id)
{
	if (chain_id == -1)
		return false;

	return m_ikChains[chain_id].enable;
}

int CAnimatingEGF::FindIKChain(const char* pszName)
{
	for (int i = 0; i < m_ikChains.numElem(); i++)
	{
		if (!CString::CompareCaseIns(m_ikChains[i].desc->name, pszName))
			return i;
	}

	return -1;
}