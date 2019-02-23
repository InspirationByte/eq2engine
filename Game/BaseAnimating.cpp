//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Base animated entity class
//
//				TODO: REDONE INVERSE KINEMATICS
//////////////////////////////////////////////////////////////////////////////////

#include "BaseAnimating.h"

// declare data table for actor
BEGIN_DATAMAP( BaseAnimating )

	DEFINE_FIELD( m_transitionTime,	VTYPE_FLOAT),
	DEFINE_EMBEDDEDARRAY( m_sequenceTimers,		MAX_SEQUENCE_TIMERS),
	DEFINE_ARRAYFIELD(m_seqBlendWeights,VTYPE_FLOAT, MAX_SEQUENCE_TIMERS),

END_DATAMAP()

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

BaseAnimating::BaseAnimating() : BaseClass(), CAnimatingEGF()
{
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
	DestroyAnimating();

	BaseClass::SetModel( pModel );

	if(!m_pModel)
		return;

	// initialize that shit to use it in future
	InitAnimating(m_pModel);
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

		tempdecal_t* pStudioDecal = m_pModel->MakeTempDecal( dec_info, m_boneTransforms );
		m_pDecals.append(pStudioDecal);
	}
}

void BaseAnimating::OnRemove()
{
	DestroyAnimating();

	BaseClass::OnRemove();
}

void BaseAnimating::UpdateBones()
{
	CAnimatingEGF::UpdateBones(gpGlobals->frametime, m_matWorldTransform);
}

void BaseAnimating::Update(float dt)
{
	// advance frame of animations
	AdvanceFrame(dt);

	// basic class update required
	BaseClass::Update(dt);
}

// finds attachment
int BaseAnimating::FindAttachment(const char* name) const
{
	return Studio_FindAttachmentId(m_pModel->GetHWData()->studio, name);
}

// gets local attachment position
const Vector3D& BaseAnimating::GetLocalAttachmentOrigin(int nAttach) const
{
	static Vector3D _zero(0.0f);

	if(nAttach == -1)
		return _zero;

	if(nAttach >= m_pModel->GetHWData()->studio->numAttachments)
		return _zero;

	studioattachment_t* attach = m_pModel->GetHWData()->studio->pAttachment(nAttach);

	Matrix4x4 matrix = identity4();
	matrix.setRotation(Vector3D(DEG2RAD(attach->angles.x),DEG2RAD(attach->angles.y),DEG2RAD(attach->angles.z)));
	matrix.setTranslation(attach->position);

	Matrix4x4 finalAttachmentTransform = matrix * m_boneTransforms[attach->bone_id];

	// because all dynamic models are left-handed
	return finalAttachmentTransform.rows[3].xyz();//*Vector3D(-1,1,1);
}

// gets local attachment direction
const Vector3D& BaseAnimating::GetLocalAttachmentDirection(int nAttach) const
{
	static Vector3D _zero(0.0f);

	if(nAttach == -1)
		return _zero;

	if(nAttach >= m_pModel->GetHWData()->studio->numAttachments)
		return _zero;

	studioattachment_t* attach = m_pModel->GetHWData()->studio->pAttachment(nAttach);

	Matrix4x4 matrix = identity4();
	matrix.setRotation(Vector3D(DEG2RAD(attach->angles.x),DEG2RAD(attach->angles.y),DEG2RAD(attach->angles.z)));
	matrix.setTranslation(attach->position);

	Matrix4x4 finalAttachmentTransform = matrix * m_boneTransforms[attach->bone_id];

	// because all dynamic models are left-handed
	return finalAttachmentTransform.rows[2].xyz();//*Vector3D(-1,1,1);
}

void BaseAnimating::HandleAnimatingEvent(AnimationEvent nEvent, char* options)
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

// renders model
void BaseAnimating::Render(int nViewRenderFlags)
{
	RenderEGFModel( nViewRenderFlags, m_boneTransforms );
}

void BaseAnimating::AttachIKChain(int chain, int attach_type)
{
	if(chain == -1)
		return;

	int effector_id = m_ikChains[chain]->numLinks - 1;
	giklink_t& link = m_ikChains[chain]->links[effector_id];

	switch(attach_type)
	{
		case IK_ATTACH_WORLD:
		{
			SetIKWorldTarget(chain, GetAbsOrigin(),m_matWorldTransform);
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
}