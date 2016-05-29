//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Base animated entity class
//
//				TODO: REDONE INVERSE KINEMATICS!//////////////////////////////////////////////////////////////////////////////////

#include "BaseAnimating.h"

#define INITIAL_TIME 0.0f


// declare data table for actor
BEGIN_DATAMAP( BaseAnimating )

	DEFINE_FIELD( m_fRemainingTransitionTime,	VTYPE_FLOAT),
	DEFINE_EMBEDDEDARRAY( m_sequenceTimers,		MAX_SEQUENCE_TIMERS),
	DEFINE_ARRAYFIELD( m_sequenceTimerBlendings,VTYPE_FLOAT, MAX_SEQUENCE_TIMERS),

END_DATAMAP()

ConVar r_debug_ik("r_debug_ik", "0", "Draw debug information about Inverse Kinematics", CV_CHEAT);
ConVar r_debug_showskeletons("r_debug_showskeletons", "0", "Draw debug information about skeletons", CV_CHEAT);

ConVar r_ik_iterations("r_ik_iterations", "100", "IK link iterations per update", CV_ARCHIVE);

ConVar r_springanimations("r_springanimations", "0", "Animation springing", CV_CHEAT);
ConVar r_springanimations_velmul("r_springanimations_velmul", "1.0f", "Animation springing", CV_CHEAT | CV_ARCHIVE);
ConVar r_springanimations_damp("r_springanimations_damp", "1.0f", "Animation springing", CV_CHEAT | CV_ARCHIVE);
ConVar r_springanimations_spring("r_springanimations_spring", "1.0f", "Animation springing", CV_CHEAT | CV_ARCHIVE);

/*
int GM_CDECL GM_BaseAnimating_PlayAnimation(gmThread* a_thread)
{
	GM_INT_PARAM(nSlot, 0, 0);	// get one optional

	BaseAnimating* pEntity = (BaseAnimating*)GetThisEntity(a_thread);

	if(!pEntity)
		return GM_EXCEPTION;

	pEntity->PlayAnimation(nSlot);

	return GM_OK;
}

int GM_CDECL GM_BaseAnimating_StopAnimation(gmThread* a_thread)
{
	GM_INT_PARAM(nSlot, 0, 0);	// get one optional

	BaseAnimating* pEntity = (BaseAnimating*)GetThisEntity(a_thread);

	if(!pEntity)
		return GM_EXCEPTION;

	pEntity->StopAnimation(nSlot);

	return GM_OK;
}

int GM_CDECL GM_BaseAnimating_SetActivity(gmThread* a_thread)
{
	GM_CHECK_NUM_PARAMS(1);
	GM_CHECK_STRING_PARAM(name, 0);
	GM_INT_PARAM(nSlot, 1, 0);	// get one optional

	BaseAnimating* pEntity = (BaseAnimating*)GetThisEntity(a_thread);

	if(!pEntity)
		return GM_EXCEPTION;

	Activity act = GetActivityByName(name);

	pEntity->SetActivity( act, nSlot);

	return GM_OK;
}

int GM_CDECL GM_BaseAnimating_GetActivity(gmThread* a_thread)
{
	GM_INT_PARAM(nSlot, 0, 0);	// get one optional

	BaseAnimating* pEntity = (BaseAnimating*)GetThisEntity(a_thread);

	if(!pEntity)
		return GM_EXCEPTION;

	Activity nAct = pEntity->GetCurrentActivity( nSlot );
	const char* pszString = GetActivityName(nAct);

	GM_THREAD_ARG->PushNewString(pszString);

	return GM_OK;
}

int GM_CDECL GM_BaseAnimating_GetRemainingAnimationTime(gmThread* a_thread)
{
	BaseAnimating* pEntity = (BaseAnimating*)GetThisEntity(a_thread);

	if(!pEntity)
		return GM_EXCEPTION;

	GM_THREAD_ARG->PushFloat(pEntity->GetCurrentRemainingAnimationDuration());

	return GM_OK;
}

int GM_CDECL GM_BaseAnimating_GetAnimationTime(gmThread* a_thread)
{
	BaseAnimating* pEntity = (BaseAnimating*)GetThisEntity(a_thread);

	if(!pEntity)
		return GM_EXCEPTION;

	GM_THREAD_ARG->PushFloat(pEntity->GetCurrentAnimationDuration());

	return GM_OK;
}
*/
void BaseAnimating::InitScriptHooks()
{
	BaseClass::InitScriptHooks();
	/*
	EQGMS_REGISTER_FUNCTION( m_pTableObject, "SetActivity", GM_BaseAnimating_SetActivity);
	EQGMS_REGISTER_FUNCTION( m_pTableObject, "GetActivity", GM_BaseAnimating_GetActivity);
	EQGMS_REGISTER_FUNCTION( m_pTableObject, "PlayAnimation", GM_BaseAnimating_PlayAnimation);
	EQGMS_REGISTER_FUNCTION( m_pTableObject, "StopAnimation", GM_BaseAnimating_StopAnimation);

	EQGMS_REGISTER_FUNCTION( m_pTableObject, "GetRemainingAnimationTime", GM_BaseAnimating_GetRemainingAnimationTime);
	EQGMS_REGISTER_FUNCTION( m_pTableObject, "GetAnimationTime", GM_BaseAnimating_GetAnimationTime);
	*/
}

BaseAnimating::BaseAnimating() : BaseClass()
{
	for(int i = 0; i < MAX_SEQUENCE_TIMERS; i++)
	{
		m_sequenceTimers[i].bPlaying = false;
		m_sequenceTimers[i].base_sequence = NULL;
		m_sequenceTimers[i].sequence_time = 0.0f;
		m_sequenceTimers[i].currFrame = 0;
		m_sequenceTimers[i].nextFrame = 0;
		m_sequenceTimers[i].playbackSpeedScale = 1.0f;
	}

	memset(m_sequenceTimerBlendings, 0, sizeof(m_sequenceTimerBlendings));

	m_sequenceTimerBlendings[0] = 1.0f;

	for(int i = 0; i < MAX_SEQUENCE_TIMERS; i++)
	{
		m_sequenceTimers[i].sequence_index = -1;
		m_sequenceTimers[i].base_sequence = NULL;
	}

	m_BoneMatrixList = NULL;
	m_LocalBonematrixList = NULL;
	m_nParentIndexList = NULL;
	m_AnimationBoneMatrixList = NULL;

	m_pTransitionAnimationBoneFrames = NULL;
	m_pLastBoneFrames = NULL;

	m_fRemainingTransitionTime = INITIAL_TIME;

	//m_pBoneVelocities = NULL;
	//m_pBoneSpringingAdd = NULL;
}

// sets model for this entity
void BaseAnimating::SetModel(const char* pszModelName)
{
	BaseClass::SetModel( pszModelName );
}

// sets model for this entity
void BaseAnimating::SetModel(IEqModel* pModel)
{
	// do cleanup
	DestroyAnimationThings();

	BaseClass::SetModel( pModel );

	if(!m_pModel)
		return;

	// initialize that shit to use it in future
	InitAnimationThings();
}

void BaseAnimating::MakeDecal( const decalmakeinfo_t& info )
{
	if( m_pModel )
	{
		if(!m_numBones)
		{
			BaseClass::MakeDecal( info );
			return;
		}

		// determine which bone was hit

		decalmakeinfo_t dec_info = info;
		dec_info.origin = (!m_matWorldTransform * Vector4D(dec_info.origin, 1.0f)).xyz();

		Matrix3x3 rotation = m_matWorldTransform.getRotationComponent();
		dec_info.normal = !rotation * dec_info.normal;

		studiotempdecal_t* pStudioDecal = m_pModel->MakeTempDecal( dec_info, m_LocalBonematrixList );
		m_pDecals.append(pStudioDecal);
	}
}

void BaseAnimating::DestroyAnimationThings()
{
	m_pSequences.clear();

	if(m_BoneMatrixList)
		PPFree(m_BoneMatrixList);

	m_BoneMatrixList = NULL;

	if(m_AnimationBoneMatrixList)
		PPFree(m_AnimationBoneMatrixList);

	m_AnimationBoneMatrixList = NULL;

	if(m_LocalBonematrixList)
		PPFree(m_LocalBonematrixList);

	m_LocalBonematrixList = NULL;

	if(m_nParentIndexList)
		PPFree(m_nParentIndexList);

	m_nParentIndexList = NULL;

	if(m_pTransitionAnimationBoneFrames)
		PPFree(m_pTransitionAnimationBoneFrames);

	m_pTransitionAnimationBoneFrames = NULL;

	if(m_pLastBoneFrames)
		PPFree(m_pLastBoneFrames);

	m_pLastBoneFrames = NULL;

	for(int i = 0; i < m_IkChains.numElem(); i++)
	{
		PPFree(m_IkChains[i]->links);
		delete m_IkChains[i];
	}

	m_IkChains.clear();

	//if(m_pBoneVelocities)
	//	PPFree(m_pBoneVelocities);

	//m_pBoneVelocities = NULL;

	//if(m_pBoneSpringingAdd)
	//	PPFree(m_pBoneSpringingAdd);

	//m_pBoneSpringingAdd = NULL;

	// stop all sequence timers
	// play animations
	for(int i = 0; i < MAX_SEQUENCE_TIMERS; i++)
	{
		m_sequenceTimers[i].sequence_index = -1;
		m_sequenceTimers[i].base_sequence = NULL;

		m_sequenceTimers[i].ResetPlayback(true);

		m_sequenceTimerBlendings[i] = 1.0f;

		m_sequenceTimers[i].called_events.clear();
		m_sequenceTimers[i].ignore_events.clear();
	}

	m_sequenceTimerBlendings[0] = 1.0f;

	m_fRemainingTransitionTime = 0.0f;

	m_numBones = 0;
}

void BaseAnimating::OnRemove()
{
	DestroyAnimationThings();

	BaseClass::OnRemove();
}

void BaseAnimating::Update(float dt)
{
	// advance frame of animations
	AdvanceFrame(dt);

	// basic class update required
	BaseClass::Update(dt);
}

Activity BaseAnimating::TranslateActivity(Activity act, int slot)
{
	// base class, no translation
	return act;
}

void BaseAnimating::InitAnimationThings()
{
	for(int i = 0; i < MAX_SEQUENCE_TIMERS; i++)
	{
		m_sequenceTimers[i].bPlaying = false;
		m_sequenceTimers[i].base_sequence = NULL;
		m_sequenceTimers[i].called_events.clear();
		m_sequenceTimers[i].ignore_events.clear();
		m_sequenceTimers[i].sequence_time = 0.0f;
		m_sequenceTimers[i].currFrame = 0;
		m_sequenceTimers[i].nextFrame = 0;
		m_sequenceTimers[i].playbackSpeedScale = 1.0f;
	}

	if(m_pModel)
	{
		m_numBones = m_pModel->GetHWData()->pStudioHdr->numbones;

		m_BoneMatrixList = PPAllocStructArray(Matrix4x4, m_numBones);
		m_AnimationBoneMatrixList = PPAllocStructArray(Matrix4x4, m_numBones);

		for(int i = 0; i < m_numBones; i++)
		{
			m_BoneMatrixList[i] = identity4();
			m_AnimationBoneMatrixList[i] = identity4();
		}
		
		m_LocalBonematrixList = PPAllocStructArray(Matrix4x4, m_numBones);
		for(int i = 0; i < m_numBones; i++)
		{
			m_LocalBonematrixList[i] = m_pModel->GetHWData()->joints[i].localTrans;
		}
		
		m_nParentIndexList = PPAllocStructArray(int, m_numBones);
		for(int i = 0; i < m_numBones; i++)
		{
			m_nParentIndexList[i] = m_pModel->GetHWData()->joints[i].parentbone;
		}

		
		m_pLastBoneFrames = PPAllocStructArray(animframe_t, m_numBones);
		memset(m_pLastBoneFrames, 0, sizeof(animframe_t)*m_numBones);

		m_pTransitionAnimationBoneFrames = PPAllocStructArray(animframe_t, m_numBones);
		memset(m_pTransitionAnimationBoneFrames, 0, sizeof(animframe_t)*m_numBones);

		//m_pBoneSpringingAdd = PPAllocStructArray(animframe_t, m_numBones);
		//memset(m_pBoneSpringingAdd, 0, sizeof(animframe_t)*m_numBones);

		//m_pBoneVelocities = PPAllocStructArray(animframe_t, m_numBones);
		//memset(m_pBoneVelocities, 0, sizeof(animframe_t)*m_numBones);

		// make standard pose
		StandardPose();

		// load ik chains
		for(int i = 0; i < m_pModel->GetHWData()->pStudioHdr->numikchains; i++)
		{
			studioikchain_t* pStudioChain = m_pModel->GetHWData()->pStudioHdr->pIkChain(i);

			gikchain_t* chain = new gikchain_t;

			strcpy(chain->name, pStudioChain->name);
			chain->numlinks = pStudioChain->numlinks;
			chain->local_target = Vector3D(0,0,10);
			chain->enable = false;

			chain->links = PPAllocStructArray(giklink_t, chain->numlinks);

			for( int j = 0; j < chain->numlinks; j++ )
			{
				giklink_t link;

				studioiklink_t* pLink = pStudioChain->pLink(j);

				link.bone_index = pLink->bone;
				link.damping = pLink->damping;
				link.limits[0] = pLink->mins;
				link.limits[1] = pLink->maxs;

				Vector3D rotation = m_pModel->GetHWData()->pStudioHdr->pBone(link.bone_index)->rotation;

				link.position = m_pModel->GetHWData()->pStudioHdr->pBone(link.bone_index)->position;
				link.quat = Quaternion(rotation.x,rotation.y,rotation.z);

				// initial transform
				link.localTrans = Matrix4x4(link.quat);
				link.localTrans.setTranslation(link.position);
				link.absTrans = identity4();

				link.parent = j-1;

				chain->links[j] = link;

				link.chain = chain;

				// set link for fast access, can be used for multiple instances of base animating entities with single model
				m_pModel->GetHWData()->joints[link.bone_index].link_id = j;
				m_pModel->GetHWData()->joints[link.bone_index].chain_id = i;
			}

			for(int j = 0; j < chain->numlinks; j++)
			{
				int parent = chain->links[j].parent;

				if(parent != -1)
				{
					chain->links[j].absTrans = chain->links[j].localTrans * chain->links[parent].absTrans;
				}
				else
					chain->links[j].absTrans = chain->links[j].localTrans;
			}

			m_IkChains.append(chain);
		}
	}

	// build activity table for loaded model
	if(m_pModel && m_pModel->GetHWData()->numMotionPackages)
	{
		for(int i = 0; i < m_pModel->GetHWData()->numMotionPackages; i++)
		{
			PreloadMotionData( m_pModel->GetHWData()->motiondata[i] );
			PrecacheSoundEvents( m_pModel->GetHWData()->motiondata[i] );
		}
	}
}

void BaseAnimating::PreloadMotionData(studiomotiondata_t* pMotionData)
{
	// create pose controllers
	for(int i = 0; i < pMotionData->numposecontrollers; i++)
	{
		gposecontroller_t controller;
		controller.pDesc = &pMotionData->posecontrollers[i];

		// get center in blending range
		controller.value = lerp(controller.pDesc->blendRange[0], controller.pDesc->blendRange[1], 0.5f);
		controller.interpolatedValue = controller.value;

		m_poseControllers.append(controller);
	}

	// copy sequences
	for(int i = 0 ; i < pMotionData->numsequences; i++)
	{
		sequencedesc_t* seqdatadesc = &pMotionData->sequences[i];

		//DevMsg(1,"%s, %s\n", seqdatadesc->name, seqdatadesc->activity);

		gsequence_t mod_sequence;
		memset(&mod_sequence, 0, sizeof(mod_sequence));

		mod_sequence.activity = GetActivityByName(seqdatadesc->activity);

		strcpy(mod_sequence.name, seqdatadesc->name);

		if(mod_sequence.activity == ACT_INVALID && stricmp(seqdatadesc->activity, "ACT_INVALID"))
			MsgError("MOP Error: Activity '%s' not registered\n", seqdatadesc->activity);

		mod_sequence.framerate = seqdatadesc->framerate;
		mod_sequence.flags = seqdatadesc->flags;
		mod_sequence.transitiontime = seqdatadesc->transitiontime;

		mod_sequence.numEvents = seqdatadesc->numEvents;
		mod_sequence.numAnimations = seqdatadesc->numAnimations;
		mod_sequence.numSequenceBlends = seqdatadesc->numSequenceBlends;

		if(seqdatadesc->posecontroller == -1)
			mod_sequence.posecontroller = NULL;
		else
			mod_sequence.posecontroller = &m_poseControllers[seqdatadesc->posecontroller];

		for(int j = 0; j < mod_sequence.numAnimations; j++)
			mod_sequence.animations[j] = &pMotionData->animations[seqdatadesc->animations[j]];

		for(int j = 0; j < mod_sequence.numEvents; j++)
			mod_sequence.events[j] = &pMotionData->events[seqdatadesc->events[j]];

		for(int j = 0; j < mod_sequence.numSequenceBlends; j++)
			mod_sequence.sequenceblends[j] = &m_pSequences[seqdatadesc->sequenceblends[j]];

		m_pSequences.append( mod_sequence );
	}
}

void BaseAnimating::PrecacheSoundEvents( studiomotiondata_t* pMotionData )
{
	for(int i = 0; i < pMotionData->numevents; i++)
	{
		AnimationEvent event_type = GetEventByName(pMotionData->events[i].command);

		if(event_type == EV_INVALID)
		{
			event_type = (AnimationEvent)atoi(pMotionData->events[i].command);
		}

		if(event_type == EV_SOUND)
		{
			PrecacheScriptSound(pMotionData->events[i].parameter);
		}
	}
}

// finds animation
int BaseAnimating::FindSequence(const char* name)
{
	for(int i = 0; i < m_pSequences.numElem(); i++)
	{
		if(!stricmp(m_pSequences[i].name, name))
		{
			return i;
		}
	}
	return -1;
}

// sets animation
void BaseAnimating::SetSequence(int animIndex, int slot)
{
	if(animIndex == -1)
		return;

	bool wasEmpty = (m_sequenceTimers[slot].sequence_index == -1);

	m_sequenceTimers[slot].sequence_index = animIndex;

	if(m_sequenceTimers[slot].sequence_index != -1)
		m_sequenceTimers[slot].base_sequence = &m_pSequences[ m_sequenceTimers[slot].sequence_index ];

	// reset playback speed to avoid some errors
	SetPlaybackSpeedScale(1.0f, slot);

	if( slot == 0 )
	{
		m_fRemainingTransitionTime = m_sequenceTimers[slot].base_sequence->transitiontime;

		if(wasEmpty)
			m_fRemainingTransitionTime = 0.0f;

		// copy last frames for transition
		memcpy(m_pTransitionAnimationBoneFrames, m_pLastBoneFrames, sizeof(animframe_t)*m_numBones);
	}
}

// sets activity
void BaseAnimating::SetActivity(Activity act,int slot)
{
	// translate activity
	Activity nTranslatedActivity = TranslateActivity( act, slot );

	for(int i = 0; i < m_pSequences.numElem(); i++)
	{
		if(m_pSequences[i].activity == nTranslatedActivity)
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
Activity BaseAnimating::GetCurrentActivity(int slot)
{
	if(m_sequenceTimers[slot].base_sequence)
		return m_sequenceTimers[slot].base_sequence->activity;

	return ACT_INVALID;
}

// resets animation time, and restarts animation
void BaseAnimating::ResetAnimationTime(int slot)
{
	m_sequenceTimers[slot].ResetPlayback();
}

// sets new animation time
void BaseAnimating::SetAnimationTime(float newTime, int slot)
{
	m_sequenceTimers[slot].SetTime(newTime);
}

int BaseAnimating::GetBoneCount()
{
	return m_pModel->GetHWData()->pStudioHdr->numbones;
}

Matrix4x4* BaseAnimating::GetBoneMatrices()
{
	return m_BoneMatrixList;
}

// makes standard pose
void BaseAnimating::StandardPose()
{
	if(m_pModel && m_pModel->GetHWData() && m_pModel->GetHWData()->joints)
	{
		for(int i = 0; i < m_pModel->GetHWData()->pStudioHdr->numbones; i++)
		{
			m_BoneMatrixList[i] = m_pModel->GetHWData()->joints[i].absTrans;
		}
	}
}

// finds bone
int BaseAnimating::FindBone(const char* boneName)
{
	for(int i = 0; i < m_pModel->GetHWData()->pStudioHdr->numbones; i++)
	{
		if(!stricmp(m_pModel->GetHWData()->joints[i].name, boneName))
			return i;
	}

	return -1;
}

// gets absolute bone position
Vector3D BaseAnimating::GetLocalBoneOrigin(int nBone)
{
	if(nBone == -1)
		return vec3_zero;

	return m_BoneMatrixList[nBone].rows[3].xyz();
}

// gets absolute bone direction
Vector3D BaseAnimating::GetLocalBoneDirection(int nBone)
{
	return Vector3D(0);
}

// finds attachment
int BaseAnimating::FindAttachment(const char* name)
{
	return Studio_FindAttachmentID(m_pModel->GetHWData()->pStudioHdr, name);
}

// gets local attachment position
Vector3D BaseAnimating::GetLocalAttachmentOrigin(int nAttach)
{
	if(nAttach == -1)
		return vec3_zero;

	if(nAttach >= m_pModel->GetHWData()->pStudioHdr->numattachments)
		return vec3_zero;

	studioattachment_t* attach = m_pModel->GetHWData()->pStudioHdr->pAttachment(nAttach);

	Matrix4x4 matrix = identity4();
	matrix.setRotation(Vector3D(DEG2RAD(attach->angles.x),DEG2RAD(attach->angles.y),DEG2RAD(attach->angles.z)));
	matrix.setTranslation(attach->position);

	Matrix4x4 finalAttachmentTransform = matrix * m_BoneMatrixList[attach->bone_id];

	// because all dynamic models are left-handed
	return finalAttachmentTransform.rows[3].xyz();//*Vector3D(-1,1,1);
}

// gets local attachment direction
Vector3D BaseAnimating::GetLocalAttachmentDirection(int nAttach)
{
	if(nAttach == -1)
		return vec3_zero;

	if(nAttach >= m_pModel->GetHWData()->pStudioHdr->numattachments)
		return vec3_zero;

	studioattachment_t* attach = m_pModel->GetHWData()->pStudioHdr->pAttachment(nAttach);

	Matrix4x4 matrix = identity4();
	matrix.setRotation(Vector3D(DEG2RAD(attach->angles.x),DEG2RAD(attach->angles.y),DEG2RAD(attach->angles.z)));
	matrix.setTranslation(attach->position);

	Matrix4x4 finalAttachmentTransform = matrix * m_BoneMatrixList[attach->bone_id];

	// because all dynamic models are left-handed
	return finalAttachmentTransform.rows[2].xyz();//*Vector3D(-1,1,1);
}

// returns duration time of the current animation
float BaseAnimating::GetCurrentAnimationDuration()
{
	if(!m_sequenceTimers[0].base_sequence)
		return 1.0f;

	return (m_sequenceTimers[0].base_sequence->animations[0]->bones[0].numframes - 1) / m_sequenceTimers[0].base_sequence->framerate;
}

// returns duration time of the specific animation
float BaseAnimating::GetAnimationDuration(int animIndex)
{
	if(animIndex == -1)
		return 1.0f;

	return (m_pSequences[animIndex].animations[0]->bones[0].numframes - 1) / m_pSequences[animIndex].framerate;
}

// returns remaining duration time of the current animation
float BaseAnimating::GetCurrentRemainingAnimationDuration()
{
	return GetCurrentAnimationDuration() - m_sequenceTimers[0].sequence_time;
}

// plays/resumes animation
void BaseAnimating::PlayAnimation(int slot)
{
	m_sequenceTimers[slot].bPlaying = true;
}

// stops/pauses animation
void BaseAnimating::StopAnimation(int slot)
{
	m_sequenceTimers[slot].bPlaying = false;
}

void BaseAnimating::SetPlaybackSpeedScale(float scale, int slot)
{
	m_sequenceTimers[slot].playbackSpeedScale = scale;
}

void BaseAnimating::SetSequenceBlending(int slot, float factor)
{
	m_sequenceTimerBlendings[slot] = factor;
}

// updates events
void BaseAnimating::UpdateEvents(float framerate)
{
	// handle events
	for(int i = 0; i < MAX_SEQUENCE_TIMERS; i++)
	{
		for(int j = 0; j < m_sequenceTimers[i].called_events.numElem(); j++)
		{
			AnimationEvent event_type = GetEventByName(m_sequenceTimers[i].called_events[j]->command);

			if(event_type == EV_INVALID)
			{
				event_type = (AnimationEvent)atoi(m_sequenceTimers[i].called_events[j]->command);
			}

			HandleEvent(event_type, m_sequenceTimers[i].called_events[j]->parameter);
		}

		m_sequenceTimers[i].called_events.clear();
	}
}

void BaseAnimating::HandleEvent(AnimationEvent nEvent, char* options)
{
	// handle some internal events here
	switch(nEvent)
	{
		case EV_SOUND:
			EmitSound( options );
			break;

		case EV_MUZZLEFLASH:

			break;

		case EV_IK_WORLDATTACH:
			{
				int ik_chain_id = FindIKChain(options);
				AttachIKChain(ik_chain_id, IK_ATTACH_WORLD);
			}
			break;

		case EV_IK_LOCALATTACH:
			{
				int ik_chain_id = FindIKChain(options);
				AttachIKChain(ik_chain_id, IK_ATTACH_LOCAL);
			}
			break;

		case EV_IK_GROUNDATTACH:
			{
				int ik_chain_id = FindIKChain(options);
				AttachIKChain(ik_chain_id, IK_ATTACH_GROUND);
			}
			break;

		case EV_IK_DETACH:
			{
				int ik_chain_id = FindIKChain(options);
				SetIKChainEnabled(ik_chain_id, false);
			}
			break;
	}
}

// advances frame (and computes interpolation between all blended animations)
void BaseAnimating::AdvanceFrame(float frameTime)
{
	m_sequenceTimerBlendings[0] = 1.0f;

	if(m_sequenceTimers[0].base_sequence)
	{
		float div_frametime = (frameTime*30) / 8;

		// interpolate pose parameter values
		for(int i = 0; i < m_poseControllers.numElem(); i++)
		{
			for(int j = 0; j < 8; j++)
			{
				m_poseControllers[i].interpolatedValue = lerp(m_poseControllers[i].interpolatedValue, m_poseControllers[i].value, div_frametime);
			}
		}
	}

	m_fRemainingTransitionTime -= frameTime;

	if(m_fRemainingTransitionTime < 0)
		m_fRemainingTransitionTime = 0;

	// play animations
	for(int i = 0; i < MAX_SEQUENCE_TIMERS; i++)
	{
		if(m_sequenceTimers[i].sequence_index != -1)
			m_sequenceTimers[i].base_sequence = &m_pSequences[ m_sequenceTimers[i].sequence_index ];

		m_sequenceTimers[i].AdvanceFrame(frameTime);
	}

	// call events.
	UpdateEvents(frameTime);
}

// swaps sequence timers
void BaseAnimating::SwapSequenceTimers(int index, int swapTo)
{
	sequencetimer_t swap_to = m_sequenceTimers[swapTo];
	m_sequenceTimers[swapTo] = m_sequenceTimers[index];
	m_sequenceTimers[index] = swap_to;

	float swap_to_time = m_sequenceTimerBlendings[swapTo];
	m_sequenceTimerBlendings[swapTo] = m_sequenceTimerBlendings[index];
	m_sequenceTimerBlendings[index] = swap_to_time;
}

void BaseAnimating::GetInterpolatedBoneFrame(modelanimation_t* pAnim, int nBone, int firstframe, int lastframe, float interp, animframe_t &out)
{
	InterpolateFrameTransform(pAnim->bones[nBone].keyframes[firstframe], pAnim->bones[nBone].keyframes[lastframe], clamp(interp,0,1), out);
}

void BaseAnimating::GetInterpolatedBoneFrameBetweenTwoAnimations(modelanimation_t* pAnim1, modelanimation_t* pAnim2, int nBone, int firstframe, int lastframe, float interp, float animTransition, animframe_t &out)
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

void BaseAnimating::GetSequenceLayerBoneFrame(gsequence_t* pSequence, int nBone, animframe_t &out)
{
	float blendWeight = 0;
	int blendAnimation1 = 0;
	int blendAnimation2 = 0;
			
	ComputeAnimationBlend(	pSequence->numAnimations, 
							pSequence->posecontroller->pDesc->blendRange, 
							pSequence->posecontroller->value, 
							blendWeight, 
							blendAnimation1, 
							blendAnimation2
							);

	modelanimation_t* pAnim1 = pSequence->animations[blendAnimation1];
	modelanimation_t* pAnim2 = pSequence->animations[blendAnimation2];

	GetInterpolatedBoneFrameBetweenTwoAnimations(	pAnim1,
													pAnim2,
													nBone,
													0,
													0,
													0,
													blendWeight,
													out
													);

}

int BaseAnimating::FindPoseController(char *name)
{
	for(int i = 0; i < m_poseControllers.numElem(); i++)
	{
		if(!stricmp(m_poseControllers[i].pDesc->name, name))
			return i;
	}

	return -1;
}

void BaseAnimating::SetPoseControllerValue(int nPoseCtrl, float value)
{
	if(nPoseCtrl == -1 || m_poseControllers.numElem()-1 < nPoseCtrl)
		return;

	m_poseControllers[nPoseCtrl].value = value;
}

// updates bones
void BaseAnimating::UpdateBones()
{
	for(int i = 0; i < MAX_SEQUENCE_TIMERS; i++)
	{
		if(m_sequenceTimers[i].sequence_index != -1)
			m_sequenceTimers[i].base_sequence = &m_pSequences[ m_sequenceTimers[i].sequence_index ];
	}

	if(!m_sequenceTimers[0].base_sequence || m_pSequences.numElem() == 0)
	{
		StandardPose();
		return;
	}

	// setup each bone's transformation
	for(int boneId = 0; boneId < m_numBones; boneId++)
	{
		animframe_t cComputedFrame;
		ZeroFrameTransform(cComputedFrame);

		for(int j = 0; j < MAX_SEQUENCE_TIMERS; j++)
		{
			// if no animation plays on this timer, continue
			if(!m_sequenceTimers[j].base_sequence)
				continue;

			float blend_weight = m_sequenceTimerBlendings[j];

			if(blend_weight <= 0)
				continue;

			modelanimation_t *curanim = m_sequenceTimers[j].base_sequence->animations[0];

			if(!curanim)
				continue;

			// the computed frame
			animframe_t cTimedFrame;
			ZeroFrameTransform(cTimedFrame);

			float frame_interp = m_sequenceTimers[j].sequence_time - m_sequenceTimers[j].currFrame;

			// blend between many animations in sequence using pose controller
			if(m_sequenceTimers[j].base_sequence->numAnimations > 1 && m_sequenceTimers[j].base_sequence->posecontroller)
			{
				
				float playingBlendWeight = 0;
				int playingBlendAnimation1 = 0;
				int playingBlendAnimation2 = 0;

				// get frame indexes and lerp value of blended animation
				ComputeAnimationBlend(	m_sequenceTimers[j].base_sequence->numAnimations, 
										m_sequenceTimers[j].base_sequence->posecontroller->pDesc->blendRange, 
										m_sequenceTimers[j].base_sequence->posecontroller->value, 
										playingBlendWeight, 
										playingBlendAnimation1, 
										playingBlendAnimation2);

				// get frame pointers
				modelanimation_t* pPlayingAnim1 = m_sequenceTimers[j].base_sequence->animations[playingBlendAnimation1];
				modelanimation_t* pPlayingAnim2 = m_sequenceTimers[j].base_sequence->animations[playingBlendAnimation2];

				// compute blending frame
				GetInterpolatedBoneFrameBetweenTwoAnimations(	pPlayingAnim1,
																pPlayingAnim2,
																boneId,
																m_sequenceTimers[j].currFrame,
																m_sequenceTimers[j].nextFrame,
																frame_interp,
																playingBlendWeight,
																cTimedFrame);
			}
			else
			{
				// simply compute frames
				modelanimation_t *curanim = m_sequenceTimers[j].base_sequence->animations[0];

				GetInterpolatedBoneFrame(curanim, boneId, m_sequenceTimers[j].currFrame, m_sequenceTimers[j].nextFrame, frame_interp, cTimedFrame);
			}

			animframe_t cAddFrame;
			ZeroFrameTransform(cAddFrame);

			// add blended sequences to this
			for(int blend_seq = 0; blend_seq < m_sequenceTimers[j].base_sequence->numSequenceBlends; blend_seq++)
			{
				gsequence_t* pSequence = m_sequenceTimers[j].base_sequence->sequenceblends[blend_seq];

				animframe_t frame;

				// get bone frame of layer
				GetSequenceLayerBoneFrame( pSequence, boneId, frame );

				AddFrameTransform(cAddFrame, frame, cAddFrame);
			}

			AddFrameTransform(cTimedFrame, cAddFrame, cTimedFrame);

			// interpolate or add the slots, this is useful for body part splitting
			if( m_sequenceTimers[j].base_sequence->flags & SEQFLAG_SLOTBLEND )
			{
				cTimedFrame.angBoneAngles *= blend_weight;
				cTimedFrame.vecBonePosition *= blend_weight;
				
				AddFrameTransform(cComputedFrame, cTimedFrame, cComputedFrame);
			}
			else
				InterpolateFrameTransform(cComputedFrame, cTimedFrame, blend_weight, cComputedFrame);
		}

		// transition of the animation frames TODO: additional doubling slots for non-base timers
		if(m_sequenceTimers[0].base_sequence && m_fRemainingTransitionTime > 0)
		{
			float transition_factor = m_fRemainingTransitionTime / m_sequenceTimers[0].base_sequence->transitiontime;

			InterpolateFrameTransform( cComputedFrame, m_pTransitionAnimationBoneFrames[boneId], transition_factor, cComputedFrame );
		}

		/*
		if(r_springanimations.GetBool() && m_sequenceTimers[0].base_sequence)
		{
			animframe_t mulFrame = m_sequenceTimers[0].base_sequence->animations[0]->bones[boneId].keyframes[m_sequenceTimers[0].nextFrame];

			mulFrame.angBoneAngles -= cComputedFrame.angBoneAngles;
			mulFrame.vecBonePosition -= cComputedFrame.vecBonePosition;

			mulFrame.angBoneAngles *= r_springanimations_velmul.GetFloat();
			mulFrame.vecBonePosition *= r_springanimations_velmul.GetFloat();

			AddFrameTransform(m_pBoneVelocities[boneId], mulFrame, m_pBoneVelocities[boneId]);
		}

		
		if(r_springanimations.GetBool() && dot(m_pBoneVelocities[boneId].angBoneAngles, m_pBoneVelocities[boneId].angBoneAngles) > 0.000001f || dot(m_pBoneSpringingAdd[boneId].angBoneAngles, m_pBoneSpringingAdd[boneId].angBoneAngles) > 0.000001f)
		{
			m_pBoneSpringingAdd[boneId].angBoneAngles += m_pBoneVelocities[boneId].angBoneAngles * gpGlobals->frametime;
			float damping = 1 - (r_springanimations_damp.GetFloat() * gpGlobals->frametime);
		
			if ( damping < 0 )
				damping = 0.0f;

			m_pBoneVelocities[boneId].angBoneAngles *= damping;
		 
			// torsional spring
			float springForceMagnitude = r_springanimations_spring.GetFloat() * gpGlobals->frametime;
			springForceMagnitude = clamp(springForceMagnitude, 0, 2 );
			m_pBoneVelocities[boneId].angBoneAngles -= m_pBoneSpringingAdd[boneId].angBoneAngles * springForceMagnitude;

			cComputedFrame.angBoneAngles += m_pBoneSpringingAdd[boneId].angBoneAngles;
		}

		if(r_springanimations.GetBool() && dot(m_pBoneVelocities[boneId].vecBonePosition, m_pBoneVelocities[boneId].vecBonePosition) > 0.000001f || dot(m_pBoneSpringingAdd[boneId].vecBonePosition, m_pBoneSpringingAdd[boneId].vecBonePosition) > 0.000001f)
		{

			m_pBoneSpringingAdd[boneId].vecBonePosition += m_pBoneVelocities[boneId].vecBonePosition * gpGlobals->frametime;
			float damping = 1 - (r_springanimations_damp.GetFloat() * gpGlobals->frametime);
		
			if ( damping < 0 )
				damping = 0.0f;

			m_pBoneVelocities[boneId].vecBonePosition *= damping;
		 
			// torsional spring
			// UNDONE: Per-axis spring constant?
			float springForceMagnitude = r_springanimations_spring.GetFloat() * gpGlobals->frametime;
			springForceMagnitude = clamp(springForceMagnitude, 0, 2 );
			m_pBoneVelocities[boneId].vecBonePosition -= m_pBoneSpringingAdd[boneId].vecBonePosition * springForceMagnitude;

			cComputedFrame.vecBonePosition += m_pBoneSpringingAdd[boneId].vecBonePosition;
		}
		*/
		
		// compute transformation
		Matrix4x4 bone_transform = CalculateLocalBonematrix(cComputedFrame);

		// set last bones
		m_pLastBoneFrames[boneId] = cComputedFrame;

		m_BoneMatrixList[boneId] = (bone_transform*m_LocalBonematrixList[boneId]);
	}

	// setup each bone's transformation
	for(int i = 0; i < m_numBones; i++)
	{
		if(m_nParentIndexList[i] != -1)
		{
			// multiply by parent transform
			m_BoneMatrixList[i] = m_BoneMatrixList[i] * m_BoneMatrixList[m_nParentIndexList[i]];

			//debugoverlay->Line3D(m_BoneMatrixList[i].rows[3].xyz(),m_BoneMatrixList[m_nParentIndexList[i]].rows[3].xyz(), color4_white, color4_white);
		}
	}

	// copy animation frames
	memcpy( m_AnimationBoneMatrixList, m_BoneMatrixList, sizeof(Matrix4x4)*m_numBones );

	// update inverse kinematics
	UpdateIK();

	if(r_debug_showskeletons.GetBool())
	{
		// setup each bone's transformation
		for(int i = 0; i < m_numBones; i++)
		{
			if(m_nParentIndexList[i] != -1)
			{
				Vector3D pos = (m_matWorldTransform*Vector4D(m_BoneMatrixList[i].rows[3].xyz(), 1.0f)).xyz();
				Vector3D parent_pos = (m_matWorldTransform*Vector4D(m_BoneMatrixList[m_nParentIndexList[i]].rows[3].xyz(), 1.0f)).xyz();

				Vector3D dX = m_matWorldTransform.getRotationComponent()*m_BoneMatrixList[i].rows[0].xyz();
				Vector3D dY = m_matWorldTransform.getRotationComponent()*m_BoneMatrixList[i].rows[1].xyz();
				Vector3D dZ = m_matWorldTransform.getRotationComponent()*m_BoneMatrixList[i].rows[2].xyz();

				debugoverlay->Line3D(pos,parent_pos, color4_white, color4_white);

				// draw axis
				debugoverlay->Line3D(pos, pos+dX, ColorRGBA(1,0,0,1), ColorRGBA(1,0,0,1));
				debugoverlay->Line3D(pos, pos+dY, ColorRGBA(0,1,0,1), ColorRGBA(0,1,0,1));
				debugoverlay->Line3D(pos, pos+dZ, ColorRGBA(0,0,1,1), ColorRGBA(0,0,1,1));
			}
		}
	}
}

// renders model
void BaseAnimating::Render(int nViewRenderFlags)
{
	RenderEGFModel( nViewRenderFlags, m_BoneMatrixList );
}

// updates inverse kinematics
void BaseAnimating::UpdateIK()
{
	bool ik_enabled_bones[128];
	memset(ik_enabled_bones,0,sizeof(ik_enabled_bones));

	// run through bones and find enabled bones by IK chain
	for(int boneId = 0; boneId < m_numBones; boneId++)
	{
		bool bone_for_ik = false;

		int chain_id = m_pModel->GetHWData()->joints[boneId].chain_id;
		int link_id = m_pModel->GetHWData()->joints[boneId].link_id;

		if(link_id != -1 && chain_id != -1 && m_IkChains[chain_id]->enable)
		{
			for(int i = 0; i < m_IkChains[chain_id]->numlinks; i++)
			{
				if(m_IkChains[chain_id]->links[i].bone_index == boneId)
				{
					ik_enabled_bones[boneId] = true;

					bone_for_ik = true;
					break;
				}
			}
		}
	}

	// solve ik links or copy frames to disabled links
	for(int i = 0; i < m_IkChains.numElem(); i++)
	{
		if( m_IkChains[i]->enable )
		{
			// display target
			if(r_debug_ik.GetBool())
			{
				Vector3D target_pos = m_IkChains[i]->local_target;
				target_pos = (m_matWorldTransform*Vector4D(target_pos, 1.0f)).xyz();

				debugoverlay->Box3D(target_pos-Vector3D(1), target_pos+Vector3D(1), ColorRGBA(0,1,0,1));
			}

			// update chain
			UpdateIkChain( m_IkChains[i] );
		}
		else
		{
			// copy last frames to all links
			for(int j = 0; j < m_IkChains[i]->numlinks; j++)
			{
				int bone_id = m_IkChains[i]->links[j].bone_index;

				m_IkChains[i]->links[j].quat = Quaternion(m_AnimationBoneMatrixList[bone_id].getRotationComponent());
				m_IkChains[i]->links[j].position = m_pModel->GetHWData()->joints[bone_id].position;

				m_IkChains[i]->links[j].localTrans = Matrix4x4(m_IkChains[i]->links[j].quat);
				m_IkChains[i]->links[j].localTrans.setTranslation(m_IkChains[i]->links[j].position);

				// fix local transform for animation
				m_IkChains[i]->links[j].localTrans = m_LocalBonematrixList[bone_id] * m_IkChains[i]->links[j].localTrans;
				m_IkChains[i]->links[j].absTrans = m_AnimationBoneMatrixList[bone_id];
			}
		}
	}
}

// solves single ik chain
void BaseAnimating::UpdateIkChain( gikchain_t* pIkChain )
{
	for(int i = 0; i < pIkChain->numlinks; i++)
	{
		// this is broken
		pIkChain->links[i].localTrans = Matrix4x4(pIkChain->links[i].quat);
		pIkChain->links[i].localTrans.setTranslation(pIkChain->links[i].position);

		//int bone_id = pIkChain->links[i].bone_index;
		//pIkChain->links[i].localTrans = m_LocalBonematrixList[bone_id] * pIkChain->links[i].localTrans;
	}

	for(int i = 0; i < pIkChain->numlinks; i++)
	{
		int parent = pIkChain->links[i].parent;

		if(parent != -1)
		{
			int bone_id = pIkChain->links[i].bone_index;

			pIkChain->links[i].absTrans = pIkChain->links[i].localTrans * pIkChain->links[parent].absTrans;

			//pIkChain->links[i].absTrans = !m_LocalBonematrixList[bone_id] * pIkChain->links[i].absTrans;

			if(r_debug_ik.GetBool())
			{
				Vector3D bone_pos = pIkChain->links[i].absTrans.rows[3].xyz();
				Vector3D parent_pos = pIkChain->links[parent].absTrans.rows[3].xyz();

				parent_pos = (m_matWorldTransform*Vector4D(parent_pos, 1.0f)).xyz();

				Vector3D dX = m_matWorldTransform.getRotationComponent()*pIkChain->links[i].absTrans.rows[0].xyz();
				Vector3D dY = m_matWorldTransform.getRotationComponent()*pIkChain->links[i].absTrans.rows[1].xyz();
				Vector3D dZ = m_matWorldTransform.getRotationComponent()*pIkChain->links[i].absTrans.rows[2].xyz();

				debugoverlay->Line3D(parent_pos, bone_pos, ColorRGBA(1,1,0,1), ColorRGBA(1,1,0,1));
				debugoverlay->Box3D(bone_pos+Vector3D(1),bone_pos-Vector3D(1), ColorRGBA(1,0,0,1));
				debugoverlay->Text3D(bone_pos, 200.0f, color4_white, "%s", m_pModel->GetHWData()->joints[pIkChain->links[i].bone_index].name);

				// draw axis
				debugoverlay->Line3D(bone_pos, bone_pos+dX, ColorRGBA(1,0,0,1), ColorRGBA(1,0,0,1));
				debugoverlay->Line3D(bone_pos, bone_pos+dY, ColorRGBA(0,1,0,1), ColorRGBA(0,1,0,1));
				debugoverlay->Line3D(bone_pos, bone_pos+dZ, ColorRGBA(0,0,1,1), ColorRGBA(0,0,1,1));
			}

			// FIXME:	INVALID CALCULATIONS HAPPENED HERE
			//			REWORK IK!!!
			m_BoneMatrixList[pIkChain->links[i].bone_index] = pIkChain->links[i].absTrans;
		}
		else // just copy transform
		{
			pIkChain->links[i].absTrans = m_BoneMatrixList[pIkChain->links[i].bone_index];
		}
	}
	
	// use last bone for movement
	// TODO: use more points to solve to do correct IK
	int nEffector = pIkChain->numlinks-1;

	// solve link now
	SolveIKLinks(pIkChain->links, &pIkChain->links[nEffector], pIkChain->local_target, gpGlobals->frametime, r_ik_iterations.GetInt());
}

// inverse kinematics

void BaseAnimating::SetIKWorldTarget(int chain_id, const Vector3D &world_position, Matrix4x4 *externaltransform)
{
	if(chain_id == -1)
		return;

	Matrix4x4 world_to_model = identity4();

	
	if(externaltransform)
	{
		world_to_model = transpose(*externaltransform);
	}
	else
	{
		world_to_model = transpose(m_matWorldTransform);
	}
	

	Vector3D local = world_position;
	local = inverseTranslateVec(local, world_to_model);
	local = inverseRotateVec(local, world_to_model);

	SetIKLocalTarget(chain_id, local);
}

void BaseAnimating::SetIKLocalTarget(int chain_id, const Vector3D &local_position)
{
	if(chain_id == -1)
		return;

	m_IkChains[chain_id]->local_target = local_position;
}

void BaseAnimating::SetIKChainEnabled(int chain_id, bool enabled)
{
	if(chain_id == -1)
		return;

	m_IkChains[chain_id]->enable = enabled;
}

bool BaseAnimating::IsIKChainEnabled(int chain_id)
{
	if(chain_id == -1)
		return false;

	return m_IkChains[chain_id]->enable;
}

int	 BaseAnimating::FindIKChain(char* pszName)
{
	for(int i = 0; i < m_IkChains.numElem(); i++)
	{
		if(!stricmp(m_IkChains[i]->name, pszName))
			return i;
	}

	MsgWarning("IK Chain %s not found in '%s'\n", pszName, m_pModel->GetName());

	return -1;
}

void BaseAnimating::AttachIKChain(int chain, int attach_type)
{
	if(chain == -1)
		return;

	int effector_id = m_IkChains[chain]->numlinks - 1;
	giklink_t* link = &m_IkChains[chain]->links[effector_id];

	switch(attach_type)
	{
		case IK_ATTACH_WORLD:
		{
			SetIKWorldTarget(chain, GetAbsOrigin());
			SetIKChainEnabled(chain, true);
			break;
		}
		case IK_ATTACH_LOCAL:
		{
			SetIKLocalTarget(chain, link->absTrans.rows[3].xyz());
			SetIKChainEnabled(chain, true);
			break;
		}
		case IK_ATTACH_GROUND:
		{
			// don't handle ground
			break;
		}
	}
}