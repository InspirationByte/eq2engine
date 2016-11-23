//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Animated model for EGFMan - supports physics
//////////////////////////////////////////////////////////////////////////////////

#include "CAnimatedModel.h"
#include "materialsystem/MeshBuilder.h"

#include "IDebugOverlay.h"

#define INITIAL_TIME 0.0f

CAnimatedModel::CAnimatedModel()
{
	memset(m_sequenceTimers, 0, sizeof(m_sequenceTimers));
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

	m_pBoneVelocities = NULL;
	m_pBoneSpringingAdd = NULL;

	m_pModel = NULL;
	m_pRagdoll = NULL;
	m_pPhysicsObject = NULL;
	m_bPhysicsEnable = false;
}

// sets model for this entity
void CAnimatedModel::SetModel(IEqModel* pModel)
{
	m_bPhysicsEnable = false;

	if(m_pPhysicsObject)
		physics->DestroyPhysicsObject(m_pPhysicsObject);

	m_pPhysicsObject = NULL;

	// do cleanup
	DestroyAnimationThings();
	DestroyRagdoll( m_pRagdoll );
	m_pRagdoll = NULL;

	m_pModel =  pModel;

	if(!m_pModel)
		return;

	// initialize that shit to use it in future
	InitAnimationThings();

	if(m_pModel->GetHWData()->m_physmodel.modeltype == PHYSMODEL_USAGE_RAGDOLL)
	{
		m_pRagdoll = CreateRagdoll( m_pModel );

		if(m_pRagdoll)
		{
			for(int i = 0; i < m_pRagdoll->m_numParts; i++)
			{
				if(m_pRagdoll->m_pParts[i])
				{
					m_pRagdoll->m_pParts[i]->SetContents( COLLISION_GROUP_RAGDOLLBONES );
					m_pRagdoll->m_pParts[i]->SetActivationState( PS_FROZEN );
					m_pRagdoll->m_pParts[i]->SetCollisionResponseEnabled( false );
				}
			}
		}
	}
	else
	{
		if(m_pModel->GetHWData()->m_physmodel.numobjects)
			m_pPhysicsObject = physics->CreateObject(&m_pModel->GetHWData()->m_physmodel, 0);
	}
}

void CAnimatedModel::TogglePhysicsState()
{
	if(m_pRagdoll || m_pPhysicsObject)
	{
		m_bPhysicsEnable = !m_bPhysicsEnable;

		if(m_bPhysicsEnable)
		{
			if(m_pRagdoll)
			{
				UpdateBones();
				m_pRagdoll->SetBoneTransform( m_BoneMatrixList, identity4() );

				for(int i = 0; i< m_pRagdoll->m_numParts; i++)
				{
					if(m_pRagdoll->m_pParts[i])
					{
						m_pRagdoll->m_pParts[i]->SetContents( COLLISION_GROUP_DEBRIS );
						m_pRagdoll->m_pParts[i]->SetCollisionMask( COLLIDE_DEBRIS );

						m_pRagdoll->m_pParts[i]->SetActivationState(PS_ACTIVE);
						//m_pRagdoll->m_pParts[i]->SetVelocity( m_vPrevPhysicsVelocity );
						m_pRagdoll->m_pParts[i]->SetCollisionResponseEnabled( true );
						//m_pRagdoll->m_pParts[i]->SetFriction(100.0f);
					}
				}

				m_pRagdoll->Wake();
			}
			else if(m_pPhysicsObject)
			{
				m_pPhysicsObject->SetPosition(Vector3D(0,m_pModel->GetAABB().maxPoint.y,0));
				m_pPhysicsObject->SetAngles(vec3_zero);
				//m_pPhysicsObject->SetFriction();
				m_pPhysicsObject->SetActivationState(PS_ACTIVE);
				m_pPhysicsObject->SetCollisionResponseEnabled( true );
				m_pPhysicsObject->SetContents(COLLISION_GROUP_OBJECTS);
				m_pPhysicsObject->SetCollisionMask(COLLIDE_OBJECT);
				m_pPhysicsObject->WakeUp();
			}
		}
		else
		{
			if(m_pRagdoll)
			{
				for(int i = 0; i < m_pRagdoll->m_numParts; i++)
				{
					if(m_pRagdoll->m_pParts[i])
					{
						m_pRagdoll->m_pParts[i]->SetContents( COLLISION_GROUP_RAGDOLLBONES );
						m_pRagdoll->m_pParts[i]->SetActivationState( PS_FROZEN );
						m_pRagdoll->m_pParts[i]->SetCollisionResponseEnabled( false );
						m_pRagdoll->m_pParts[i]->SetVelocity(vec3_zero);
						m_pRagdoll->m_pParts[i]->SetAngularVelocity(Vector3D(1,1,1), 0.0);
						m_pRagdoll->m_pParts[i]->SetFriction(4.0);

					}
				}
				m_pRagdoll->GetVisualBonesTransforms( m_BoneMatrixList );
			}
			else if(m_pPhysicsObject)
			{
				m_pPhysicsObject->SetActivationState(PS_FROZEN);
				m_pPhysicsObject->SetCollisionResponseEnabled( false );
			}
			

			//UpdateBones();
			//m_pRagdoll->SetBoneTransform( m_BoneMatrixList, identity4() );
		}
	}
}

void CAnimatedModel::DestroyAnimationThings()
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
		PPFree(m_IkChains[i]);
	}

	m_IkChains.clear();

	if(m_pBoneVelocities)
		PPFree(m_pBoneVelocities);

	m_pBoneVelocities = NULL;

	if(m_pBoneSpringingAdd)
		PPFree(m_pBoneSpringingAdd);

	m_pBoneSpringingAdd = NULL;

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

void CAnimatedModel::Update(float dt)
{
	if(!m_pModel)
		return;

	// advance frame of animations
	AdvanceFrame(dt);

	// update inverse kinematics
	UpdateIK(dt);
}

Activity CAnimatedModel::TranslateActivity(Activity act, int slot)
{
	// base class, no translation
	return act;
}

void CAnimatedModel::InitAnimationThings()
{
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

		m_pBoneSpringingAdd = PPAllocStructArray(animframe_t, m_numBones);
		memset(m_pBoneSpringingAdd, 0, sizeof(animframe_t)*m_numBones);

		m_pBoneVelocities = PPAllocStructArray(animframe_t, m_numBones);
		memset(m_pBoneVelocities, 0, sizeof(animframe_t)*m_numBones);

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

void CAnimatedModel::PreloadMotionData(studiomotiondata_t* pMotionData)
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

		//DevMsg(DEVMSG_GAME,"%s, %s\n", seqdatadesc->name, seqdatadesc->activity);

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

void CAnimatedModel::PrecacheSoundEvents( studiomotiondata_t* pMotionData )
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
			MsgError("EV_SOUND Unsupported\n");
			//PrecacheScriptSound(pMotionData->events[i].parameter);
		}
	}
}

// finds animation
int CAnimatedModel::FindSequence(const char* name)
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
void CAnimatedModel::SetSequence(int animIndex,int slot)
{
	if(animIndex == -1)
		return;

	m_sequenceTimers[slot].sequence_index = animIndex;

	if(m_sequenceTimers[slot].sequence_index != -1)
		m_sequenceTimers[slot].base_sequence = &m_pSequences[ m_sequenceTimers[slot].sequence_index ];

	// reset playback speed to avoid some errors
	SetPlaybackSpeedScale(1.0f, slot);

	if(slot == 0)
	{
		m_fRemainingTransitionTime = m_sequenceTimers[slot].base_sequence->transitiontime;

		// copy last frames for transition
		memcpy(m_pTransitionAnimationBoneFrames, m_pLastBoneFrames, sizeof(animframe_t)*m_numBones);
	}
}

// sets activity
void CAnimatedModel::SetActivity(Activity act,int slot)
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

	DevMsg(DEVMSG_GAME, "Activity \"%s\" not found!\n", GetActivityName(act));
}

// returns current activity
Activity CAnimatedModel::GetCurrentActivity(int slot)
{
	if(m_sequenceTimers[slot].base_sequence)
		return m_sequenceTimers[slot].base_sequence->activity;

	return ACT_INVALID;
}

// resets animation time, and restarts animation
void CAnimatedModel::ResetAnimationTime(int slot)
{
	m_sequenceTimers[slot].ResetPlayback();
}

// sets new animation time
void CAnimatedModel::SetAnimationTime(float newTime, int slot)
{
	m_sequenceTimers[slot].SetTime(newTime);
}

int CAnimatedModel::GetBoneCount()
{
	return m_pModel->GetHWData()->pStudioHdr->numbones;
}

Matrix4x4* CAnimatedModel::GetBoneMatrices()
{
	return m_BoneMatrixList;
}

// makes standard pose
void CAnimatedModel::StandardPose()
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
int CAnimatedModel::FindBone(const char* boneName)
{
	for(int i = 0; i < m_pModel->GetHWData()->pStudioHdr->numbones; i++)
	{
		if(!stricmp(m_pModel->GetHWData()->joints[i].name, boneName))
			return i;
	}

	return -1;
}

// gets absolute bone position
Vector3D CAnimatedModel::GetLocalBoneOrigin(int nBone)
{
	if(nBone == -1)
		return vec3_zero;

	return m_BoneMatrixList[nBone].rows[3].xyz();
}

// gets absolute bone direction
Vector3D CAnimatedModel::GetLocalBoneDirection(int nBone)
{
	return Vector3D(0);
}

// finds attachment
int CAnimatedModel::FindAttachment(const char* name)
{
	return Studio_FindAttachmentID(m_pModel->GetHWData()->pStudioHdr, name);
}

// gets local attachment position
Vector3D CAnimatedModel::GetLocalAttachmentOrigin(int nAttach)
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
Vector3D CAnimatedModel::GetLocalAttachmentDirection(int nAttach)
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

	return finalAttachmentTransform.rows[2].xyz();//*Vector3D(-1,1,1);
}

// returns duration time of the current animation
float CAnimatedModel::GetCurrentAnimationDuration()
{
	if(!m_sequenceTimers[0].base_sequence)
		return 1.0f;

	return (m_sequenceTimers[0].base_sequence->animations[0]->bones[0].numframes - 1) / m_sequenceTimers[0].base_sequence->framerate;
}

// returns duration time of the specific animation
float CAnimatedModel::GetAnimationDuration(int animIndex)
{
	if(animIndex == -1)
		return 1.0f;

	return (m_pSequences[animIndex].animations[0]->bones[0].numframes - 1) / m_pSequences[animIndex].framerate;
}

// returns remaining duration time of the current animation
float CAnimatedModel::GetCurrentRemainingAnimationDuration()
{
	return GetCurrentAnimationDuration() - m_sequenceTimers[0].sequence_time;
}

// plays/resumes animation
void CAnimatedModel::PlayAnimation(int slot)
{
	m_sequenceTimers[slot].bPlaying = true;
}

// stops/pauses animation
void CAnimatedModel::StopAnimation(int slot)
{
	m_sequenceTimers[slot].bPlaying = false;
}

void CAnimatedModel::SetPlaybackSpeedScale(float scale, int slot)
{
	m_sequenceTimers[slot].playbackSpeedScale = scale;
}

void CAnimatedModel::SetSequenceBlending(int slot, float factor)
{
	m_sequenceTimerBlendings[slot] = factor;
}

// updates events
void CAnimatedModel::UpdateEvents(float framerate)
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

void CAnimatedModel::HandleEvent(AnimationEvent nEvent, char* options)
{
	// handle some internal events here
	switch(nEvent)
	{
		case EV_SOUND:
			//EmitSound( options );
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
void CAnimatedModel::AdvanceFrame(float frameTime)
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
void CAnimatedModel::SwapSequenceTimers(int index, int swapTo)
{
	sequencetimer_t swap_to = m_sequenceTimers[swapTo];
	m_sequenceTimers[swapTo] = m_sequenceTimers[index];
	m_sequenceTimers[index] = swap_to;

	float swap_to_time = m_sequenceTimerBlendings[swapTo];
	m_sequenceTimerBlendings[swapTo] = m_sequenceTimerBlendings[index];
	m_sequenceTimerBlendings[index] = swap_to_time;
}

void CAnimatedModel::GetInterpolatedBoneFrame(modelanimation_t* pAnim, int nBone, int firstframe, int lastframe, float interp, animframe_t &out)
{
	InterpolateFrameTransform(pAnim->bones[nBone].keyframes[firstframe], pAnim->bones[nBone].keyframes[lastframe], clamp(interp,0,1), out);
}

void CAnimatedModel::GetInterpolatedBoneFrameBetweenTwoAnimations(modelanimation_t* pAnim1, modelanimation_t* pAnim2, int nBone, int firstframe, int lastframe, float interp, float animTransition, animframe_t &out)
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

void CAnimatedModel::GetSequenceLayerBoneFrame(gsequence_t* pSequence, int nBone, animframe_t &out)
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

int CAnimatedModel::FindPoseController(char *name)
{
	for(int i = 0; i < m_poseControllers.numElem(); i++)
	{
		if(!stricmp(m_poseControllers[i].pDesc->name, name))
			return i;
	}

	return -1;
}

void CAnimatedModel::SetPoseControllerValue(int nPoseCtrl, float value)
{
	if(nPoseCtrl == -1 || m_poseControllers.numElem()-1 < nPoseCtrl)
		return;

	m_poseControllers[nPoseCtrl].value = value;
}

// updates bones
void CAnimatedModel::UpdateBones()
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
		}
	}

	// copy animation frames
	memcpy( m_AnimationBoneMatrixList, m_BoneMatrixList, sizeof(Matrix4x4)*m_numBones );
}

void CAnimatedModel::UpdateRagdollBones()
{
	if(m_pRagdoll)
	{
		if(!m_bPhysicsEnable)
		{
			m_pRagdoll->SetBoneTransform( m_BoneMatrixList, identity4() );
		}
		else
		{
			m_pRagdoll->GetVisualBonesTransforms( m_BoneMatrixList );
		}
	}
}

void RenderPhysModel(IEqModel* pModel)
{
	if(!pModel)
		return;

	if(!pModel->GetHWData())
		return;

	if(pModel->GetHWData()->m_physmodel.numobjects == 0)
		return;

	if(pModel->GetHWData()->m_physmodel.numshapes == 0)
		return;

	materials->SetAmbientColor(1.0f);
	materials->SetDepthStates(true,false);
	materials->SetRasterizerStates(CULL_BACK,FILL_WIREFRAME);
	materials->BindMaterial(materials->GetDefaultMaterial());

	CMeshBuilder meshBuilder(materials->GetDynamicMesh());

	physmodeldata_t& phys_data = pModel->GetHWData()->m_physmodel;

	for(int i = 0; i < phys_data.numobjects; i++)
	{
		for(int j = 0; j < phys_data.objects[i].object.numShapes; j++)
		{

			int nShape = phys_data.objects[i].object.shape_indexes[j];
			if(nShape < 0 || nShape > phys_data.numshapes)
			{
				continue;
			}

			int startIndex = phys_data.shapes[nShape].shape_info.startIndices;
			int moveToIndex = startIndex + phys_data.shapes[nShape].shape_info.numIndices;

			meshBuilder.Begin(PRIM_TRIANGLES);
			for(int k = startIndex; k < moveToIndex; k++)
			{
				meshBuilder.Color4f(0,1,1,1);
				meshBuilder.Position3fv(phys_data.vertices[phys_data.indices[k]] + phys_data.objects[i].object.offset);

				meshBuilder.AdvanceVertex();
			}
			meshBuilder.End();
		}
	}
}


// renders model
void CAnimatedModel::Render(int nViewRenderFlags, float fDist)
{
	if(!m_pModel)
		return;

	if(m_pRagdoll && m_bPhysicsEnable)
		UpdateRagdollBones();
	else
		UpdateBones();

	Matrix4x4 wvp;

	Matrix4x4 posMatrix = identity4();

	if(m_bPhysicsEnable)
	{
		if(m_pRagdoll)
			posMatrix.translate(m_pRagdoll->GetPosition());
		else if(m_pPhysicsObject)
			posMatrix = m_pPhysicsObject->GetTransformMatrix();
	}

	materials->SetMatrix(MATRIXMODE_WORLD, posMatrix);

	studiohdr_t* pHdr = m_pModel->GetHWData()->pStudioHdr;

	/*
	Vector3D view_vec = g_pViewEntity->GetEyeOrigin() - m_matWorldTransform.getTranslationComponent();

	// select the LOD
	int nLOD = m_pModel->SelectLod( length(view_vec) ); // lod distance check

	// add water reflection lods
	if(nViewRenderFlags & VR_FLAG_WATERREFLECTION)
		nLOD += r_waterModelLod.GetInt();

	*/

	materials->SetAmbientColor( color4_white );

	int nLOD = m_pModel->SelectLod( fDist ); // lod distance check

	for(int i = 0; i < pHdr->numbodygroups; i++)
	{
		int bodyGroupLOD = nLOD;

		// TODO: check bodygroups for rendering

		int nLodModelIdx = pHdr->pBodyGroups(i)->lodmodel_index;
		int nModDescId = pHdr->pLodModel(nLodModelIdx)->lodmodels[ bodyGroupLOD ];

		// get the right LOD model number
		while(nModDescId == -1 && bodyGroupLOD > 0)
		{
			bodyGroupLOD--;
			nModDescId = pHdr->pLodModel(nLodModelIdx)->lodmodels[ bodyGroupLOD ];
		}

		if(nModDescId == -1)
			continue;

		// render model groups that in this body group
		for(int j = 0; j < pHdr->pModelDesc(nModDescId)->numgroups; j++)
		{
			materials->SetSkinningEnabled(true);

			IMaterial* pMaterial = m_pModel->GetMaterial(nModDescId, j);
			materials->BindMaterial(pMaterial, false);

			m_pModel->PrepareForSkinning( m_BoneMatrixList );
			m_pModel->DrawGroup( nModDescId, j );
		}
	}

	if(nViewRenderFlags & RFLAG_PHYSICS)
		RenderPhysModel(m_pModel);

	if( nViewRenderFlags & RFLAG_BONES )
		VisualizeBones();
}

void CAnimatedModel::VisualizeBones()
{
	Matrix4x4 m_matWorldTransform = identity4();

	// setup each bone's transformation
	for(int i = 0; i < m_numBones; i++)
	{
		Vector3D pos = (m_matWorldTransform*Vector4D(m_BoneMatrixList[i].rows[3].xyz(), 1.0f)).xyz();

		if(m_nParentIndexList[i] != -1)
		{
			Vector3D parent_pos = (m_matWorldTransform*Vector4D(m_BoneMatrixList[m_nParentIndexList[i]].rows[3].xyz(), 1.0f)).xyz();
			debugoverlay->Line3D(pos,parent_pos, color4_white, color4_white);
		}

		Vector3D dX = m_matWorldTransform.getRotationComponent()*m_BoneMatrixList[i].rows[0].xyz();
		Vector3D dY = m_matWorldTransform.getRotationComponent()*m_BoneMatrixList[i].rows[1].xyz();
		Vector3D dZ = m_matWorldTransform.getRotationComponent()*m_BoneMatrixList[i].rows[2].xyz();

		// draw axis
		debugoverlay->Line3D(pos, pos+dX*0.1f, ColorRGBA(1,0,0,1), ColorRGBA(1,0,0,1));
		debugoverlay->Line3D(pos, pos+dY*0.1f, ColorRGBA(0,1,0,1), ColorRGBA(0,1,0,1));
		debugoverlay->Line3D(pos, pos+dZ*0.1f, ColorRGBA(0,0,1,1), ColorRGBA(0,0,1,1));

		Vector3D frameAng = m_pLastBoneFrames[i].angBoneAngles;
	}
}

// updates inverse kinematics
void CAnimatedModel::UpdateIK(float frameTime)
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
			//if(r_debug_ik.GetBool())
			//	debugoverlay->Box3D(m_IkChains[i]->local_target-Vector3D(1), m_IkChains[i]->local_target+Vector3D(1), ColorRGBA(0,1,0,1));

			// update chain
			UpdateIkChain( m_IkChains[i], frameTime );
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
				m_IkChains[i]->links[j].localTrans =  m_LocalBonematrixList[bone_id] * m_IkChains[i]->links[j].localTrans;
				m_IkChains[i]->links[j].absTrans = m_AnimationBoneMatrixList[bone_id];
			}

			/*
			for(int j = 0; j < m_IkChains[i]->ref->numlinks; j++)
			{
				int bone_id = m_IkChains[i]->links[j].bone_index;
				m_IkChains[i]->links[j].absTrans = m_AnimationBoneMatrixList[bone_id];

				

				/*
				int parent = m_IkChains[i]->links[j].parent;
				int bone_id = m_IkChains[i]->links[j].bone_index;

				if(parent != -1)
				{
					m_IkChains[i]->links[j].absTrans = m_IkChains[i]->links[j].localTrans * m_IkChains[i]->links[parent].absTrans;
				}
				else
				{
					m_IkChains[i]->links[j].absTrans = m_IkChains[i]->links[j].localTrans;
				}* /
				
			}*/
		}
	}
}

// solves single ik chain
void CAnimatedModel::UpdateIkChain( gikchain_t* pIkChain, float frameTime )
{
	for(int i = 0; i < pIkChain->numlinks; i++)
	{
		pIkChain->links[i].localTrans = Matrix4x4(pIkChain->links[i].quat);
		pIkChain->links[i].localTrans.setTranslation(pIkChain->links[i].position);
	}

	for(int i = 0; i < pIkChain->numlinks; i++)
	{
		int parent = pIkChain->links[i].parent;

		if(parent != -1)
		{
			pIkChain->links[i].absTrans = pIkChain->links[i].localTrans * pIkChain->links[parent].absTrans;
			/*
			if(r_debug_ik.GetBool())
			{
				Vector3D bone_pos = pIkChain->links[i].absTrans.rows[3].xyz();
				debugoverlay->Line3D(pIkChain->links[parent].absTrans.rows[3].xyz(), pIkChain->links[i].absTrans.rows[3].xyz(), ColorRGBA(1,1,0,1), ColorRGBA(1,1,0,1));
				debugoverlay->Box3D(bone_pos+Vector3D(1),bone_pos-Vector3D(1), ColorRGBA(1,0,0,1));
				debugoverlay->Text3D(bone_pos, color4_white, "%s", m_pModel->GetHWData()->joints[pIkChain->links[i].bone_index].name);
			}
			*/
			m_BoneMatrixList[pIkChain->links[i].bone_index] = pIkChain->links[i].absTrans;
		}
		else
		{
			pIkChain->links[i].absTrans = m_BoneMatrixList[pIkChain->links[i].bone_index];
		}
	}
	
	// use last bone for movement
	// TODO: use more points to solve to do correct IK
	int nEffector = pIkChain->numlinks-1;

	// solve link now
	SolveIKLinks(pIkChain->links, &pIkChain->links[nEffector], pIkChain->local_target, frameTime, pIkChain->numlinks);
}

// inverse kinematics

void CAnimatedModel::SetIKWorldTarget(int chain_id, const Vector3D &world_position, Matrix4x4 *externaltransform)
{
	if(chain_id == -1)
		return;

	Matrix4x4 world_to_model = identity4();

	/*
	if(externaltransform)
	{
		world_to_model = transpose(*externaltransform);
	}
	else
	{
		world_to_model = transpose(GenerateObjectTransformMatrix(GetAbsOrigin(), GetAbsAngles(), true));
	}
	*/

	Vector3D local = world_position;
	local = inverseTranslateVec(local, world_to_model);
	local = inverseRotateVec(local, world_to_model);

	SetIKLocalTarget(chain_id, local);
}

void CAnimatedModel::SetIKLocalTarget(int chain_id, const Vector3D &local_position)
{
	if(chain_id == -1)
		return;

	m_IkChains[chain_id]->local_target = local_position;
}

void CAnimatedModel::SetIKChainEnabled(int chain_id, bool enabled)
{
	if(chain_id == -1)
		return;

	m_IkChains[chain_id]->enable = enabled;
}

bool CAnimatedModel::IsIKChainEnabled(int chain_id)
{
	if(chain_id == -1)
		return false;

	return m_IkChains[chain_id]->enable;
}

int	 CAnimatedModel::FindIKChain(char* pszName)
{
	for(int i = 0; i < m_IkChains.numElem(); i++)
	{
		if(!stricmp(m_IkChains[i]->name, pszName))
			return i;
	}

	MsgWarning("IK Chain %s not found in '%s'\n", pszName, m_pModel->GetName());

	return -1;
}

void CAnimatedModel::AttachIKChain(int chain, int attach_type)
{
	if(chain == -1)
		return;

	int effector_id = m_IkChains[chain]->numlinks - 1;
	giklink_t* link = &m_IkChains[chain]->links[effector_id];

	switch(attach_type)
	{
		case IK_ATTACH_WORLD:
		{
			SetIKWorldTarget(chain, vec3_zero);
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