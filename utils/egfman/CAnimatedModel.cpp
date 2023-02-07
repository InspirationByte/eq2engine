//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Animated model for EGFMan - supports physics
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "CAnimatedModel.h"

#include "egf/StudioGeom.h"
#include "animating/anim_events.h"

#include "dkphysics/ragdoll.h"
#include "dkphysics/IDkPhysics.h"
#include "physics/PhysicsCollisionGroup.h"

#include "materialsystem1/IMaterialSystem.h"
#include "materialsystem1/MeshBuilder.h"

#include "render/IDebugOverlay.h"

#define INITIAL_TIME 0.0f

CAnimatedModel::CAnimatedModel()
{
	m_pModel = nullptr;
	m_pRagdoll = nullptr;
	m_physObj = nullptr;
	m_bPhysicsEnable = false;

	m_bodyGroupFlags = 0xFFFFFFF;
}

// sets model for this entity
void CAnimatedModel::SetModel(CEqStudioGeom* pModel)
{
	m_bPhysicsEnable = false;

	if(m_physObj)
		physics->DestroyPhysicsObject(m_physObj);

	m_physObj = nullptr;

	// do cleanup
	DestroyAnimating();
	DestroyRagdoll( m_pRagdoll );
	m_pRagdoll = nullptr;

	m_pModel =  pModel;

	if(!m_pModel)
		return;

	// initialize that shit to use it in future
	InitAnimating(m_pModel);

	const studioPhysData_t& physData = m_pModel->GetPhysData();
	if(physData.modeltype == PHYSMODEL_USAGE_RAGDOLL)
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
		if(physData.numObjects)
			m_physObj = physics->CreateObject(&physData, 0);
	}
}

void CAnimatedModel::TogglePhysicsState()
{
	if(m_pRagdoll || m_physObj)
	{
		m_bPhysicsEnable = !m_bPhysicsEnable;

		if(m_bPhysicsEnable)
		{
			ResetPhysics();
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
				m_pRagdoll->GetVisualBonesTransforms( m_boneTransforms );
			}
			else if(m_physObj)
			{
				m_physObj->SetActivationState(PS_FROZEN);
				m_physObj->SetCollisionResponseEnabled( false );
			}
			

			//UpdateBones();
			//m_pRagdoll->SetBoneTransform( m_boneTransforms, identity4() );
		}
	}
}

void CAnimatedModel::ResetPhysics()
{
	if(m_pRagdoll)
	{
		RecalcBoneTransforms();
		UpdateIK(0.0f, identity4());

		m_pRagdoll->SetBoneTransform(m_boneTransforms, identity4() );

		for(int i = 0; i< m_pRagdoll->m_numParts; i++)
		{
			if(m_pRagdoll->m_pParts[i])
			{
				m_pRagdoll->m_pParts[i]->SetContents( COLLISION_GROUP_DEBRIS );
				m_pRagdoll->m_pParts[i]->SetCollisionMask( COLLIDE_DEBRIS );

				m_pRagdoll->m_pParts[i]->SetActivationState(PS_ACTIVE);
				m_pRagdoll->m_pParts[i]->SetVelocity( vec3_zero );
				m_pRagdoll->m_pParts[i]->SetAngularVelocity( vec3_zero, 0.0f );
				m_pRagdoll->m_pParts[i]->SetCollisionResponseEnabled( true );
			}
		}

		m_pRagdoll->Wake();
	}
	else if(m_physObj)
	{
		m_physObj->SetPosition(Vector3D(0,m_pModel->GetBoundingBox().maxPoint.y,0));
		m_physObj->SetAngles(vec3_zero);
		//m_physObj->SetFriction();
		m_physObj->SetActivationState(PS_ACTIVE);
		m_physObj->SetCollisionResponseEnabled( true );
		m_physObj->SetContents(COLLISION_GROUP_OBJECTS);
		m_physObj->SetCollisionMask(COLLIDE_OBJECT);
		m_physObj->WakeUp();
	}
}

void CAnimatedModel::Update(float dt)
{
	if(!m_pModel)
		return;

	// advance frame of animations
	AdvanceFrame(dt);

	// update inverse kinematics
	UpdateIK(dt, identity4());
}

// finds attachment
int CAnimatedModel::FindAttachment(const char* name)
{
	return Studio_FindAttachmentId(&m_pModel->GetStudioHdr(), name);
}

// gets local attachment position
Vector3D CAnimatedModel::GetLocalAttachmentOrigin(int nAttach)
{
	if(nAttach == -1)
		return vec3_zero;

	const studiohdr_t& studio = m_pModel->GetStudioHdr();

	if(nAttach >= studio.numAttachments)
		return vec3_zero;

	const studioattachment_t* attach = studio.pAttachment(nAttach);

	Matrix4x4 matrix = identity4();
	matrix.setRotation(Vector3D(DEG2RAD(attach->angles.x),DEG2RAD(attach->angles.y),DEG2RAD(attach->angles.z)));
	matrix.setTranslation(attach->position);

	Matrix4x4 finalAttachmentTransform = matrix * m_boneTransforms[attach->bone_id];

	// because all dynamic models are left-handed
	return finalAttachmentTransform.rows[3].xyz();//*Vector3D(-1,1,1);
}

// gets local attachment direction
Vector3D CAnimatedModel::GetLocalAttachmentDirection(int nAttach)
{
	if(nAttach == -1)
		return vec3_zero;

	const studiohdr_t& studio = m_pModel->GetStudioHdr();
	if(nAttach >= studio.numAttachments)
		return vec3_zero;

	const studioattachment_t* attach = studio.pAttachment(nAttach);

	Matrix4x4 matrix = identity4();
	matrix.setRotation(Vector3D(DEG2RAD(attach->angles.x),DEG2RAD(attach->angles.y),DEG2RAD(attach->angles.z)));
	matrix.setTranslation(attach->position);

	Matrix4x4 finalAttachmentTransform = matrix * m_boneTransforms[attach->bone_id];

	return finalAttachmentTransform.rows[2].xyz();//*Vector3D(-1,1,1);
}

enum EIKAttachType
{
	IK_ATTACH_WORLD = 0,
	IK_ATTACH_LOCAL,	//= 1,
	IK_ATTACH_GROUND,	//= 2,
};

void CAnimatedModel::HandleAnimatingEvent(AnimationEvent nEvent, const char* options)
{
	// handle some internal events here
	switch(nEvent)
	{
		case EV_SOUND:
			// TODO: callback!
			// EmitSound( options );
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

void CAnimatedModel::UpdateRagdollBones()
{
	if(m_pRagdoll)
	{
		if(!m_bPhysicsEnable)
		{
			m_pRagdoll->SetBoneTransform( m_boneTransforms, identity4() );
		}
		else
		{
			m_pRagdoll->GetVisualBonesTransforms(m_boneTransforms);
		}
	}
}

void CAnimatedModel::RenderPhysModel()
{
	if(!m_pModel)
		return;

	const studioPhysData_t& physData = m_pModel->GetPhysData();

	if(physData.numObjects == 0)
		return;

	if(physData.numShapes == 0)
		return;

	BlendStateParam_t blending;
	blending.srcFactor = BLENDFACTOR_SRC_ALPHA;
	blending.dstFactor = BLENDFACTOR_ONE_MINUS_SRC_ALPHA;

	materials->SetAmbientColor(ColorRGBA(1,0,1,1));
	materials->SetDepthStates(true,false);
	materials->SetRasterizerStates(CULL_BACK,FILL_WIREFRAME);
	materials->SetBlendingStates(blending);
	MatTextureProxy(materials->FindGlobalMaterialVar(StringToHashConst("basetexture"))).Set(nullptr);

	CMeshBuilder meshBuilder(materials->GetDynamicMesh());

	Matrix4x4 worldPosMatrix;
	materials->GetMatrix(MATRIXMODE_WORLD, worldPosMatrix);

	materials->BindMaterial(materials->GetDefaultMaterial());

	for(int i = 0; i < physData.numObjects; i++)
	{
		for(int j = 0; j < physData.objects[i].object.numShapes; j++)
		{
			int nShape = physData.objects[i].object.shape_indexes[j];
			if(nShape < 0 || nShape > physData.numShapes)
			{
				continue;
			}

			int startIndex = physData.shapes[nShape].shape_info.startIndices;
			int moveToIndex = startIndex + physData.shapes[nShape].shape_info.numIndices;

			if(m_boneTransforms != nullptr && m_pRagdoll)
			{
				int visualMatrixIdx = m_pRagdoll->m_pBoneToVisualIndices[i];
				Matrix4x4 boneFrame = m_pRagdoll->m_pJoints[i]->GetFrameTransformA();

				materials->SetMatrix(MATRIXMODE_WORLD, worldPosMatrix*transpose(!boneFrame*m_boneTransforms[visualMatrixIdx]));
				materials->BindMaterial(materials->GetDefaultMaterial());
			}

			meshBuilder.Begin(PRIM_TRIANGLES);
			for(int k = startIndex; k < moveToIndex; k++)
			{
				meshBuilder.Color4f(1,0,1,1);
				meshBuilder.Position3fv(physData.vertices[physData.indices[k]]);// + physData.objects[i].object.offset);

				meshBuilder.AdvanceVertex();
			}
			meshBuilder.End();
		}
	}
}

int CAnimatedModel::GetCurrentAnimationFrame() const
{
	return m_sequenceTimers[0].currFrame;
}

int CAnimatedModel::GetCurrentAnimationDurationInFrames() const
{
	if (!m_sequenceTimers[0].seq)
		return 1;

	return m_sequenceTimers[0].seq->animations[0]->bones[0].numFrames - 1;
}

int	CAnimatedModel::GetNumSequences() const
{
	return m_seqList.numElem();
}

int	CAnimatedModel::GetNumPoseControllers() const
{
	return m_poseControllers.numElem();
}

const gsequence_t& CAnimatedModel::GetSequence(int seq) const
{
	return m_seqList[seq];
}

const gposecontroller_t& CAnimatedModel::GetPoseController(int pc) const
{
	return m_poseControllers[pc];
}

// renders model
void CAnimatedModel::Render(int nViewRenderFlags, float fDist, int startLod, bool overrideLod, float dt)
{
	if(!m_pModel)
		return;

	if (m_pRagdoll && m_bPhysicsEnable)
	{
		UpdateRagdollBones();
	}
	else
	{
		RecalcBoneTransforms();
		UpdateIK(dt, identity4());
	}

	Matrix4x4 wvp;

	Matrix4x4 posMatrix = identity4();

	if(m_bPhysicsEnable)
	{
		if(m_pRagdoll)
			posMatrix.translate(m_pRagdoll->GetPosition());
		else if(m_physObj)
			posMatrix = m_physObj->GetTransformMatrix();
	}

	materials->SetMatrix(MATRIXMODE_WORLD, posMatrix);

	const studiohdr_t& studio = m_pModel->GetStudioHdr();

	/*
	Vector3D view_vec = g_pViewEntity->GetEyeOrigin() - m_matWorldTransform.getTranslationComponent();

	// select the LOD
	int nLOD = m_pModel->SelectLod( length(view_vec) ); // lod distance check

	// add water reflection lods
	if(nViewRenderFlags & VR_FLAG_WATERREFLECTION)
		nLOD += r_waterModelLod.GetInt();

	*/

	materials->SetAmbientColor( color_white );

	int startLOD = m_pModel->SelectLod( fDist ); // lod distance check

	if(!overrideLod)
		startLOD += startLod;
	else
		startLOD = startLod;

	CEqStudioGeom::DrawProps drawProperties;
	drawProperties.boneTransforms = m_boneTransforms;
	drawProperties.lod = startLOD;
	drawProperties.bodyGroupFlags = m_bodyGroupFlags;
	m_pModel->Draw(drawProperties);

	if(nViewRenderFlags & RFLAG_PHYSICS)
		RenderPhysModel();

	if( nViewRenderFlags & RFLAG_BONES )
		VisualizeBones();
}

void CAnimatedModel::VisualizeBones()
{
	Matrix4x4 posMatrix = identity4();

	if(m_bPhysicsEnable)
	{
		if(m_pRagdoll)
			posMatrix.translate(m_pRagdoll->GetPosition());
		else if(m_physObj)
			posMatrix = m_physObj->GetTransformMatrix();
	}

	// setup each bone's transformation
	for(int i = 0; i < m_numBones; i++)
	{
		const Vector3D pos = inverseTransformPoint(m_boneTransforms[i].rows[3].xyz(), posMatrix);

		if(m_joints[i].parent != -1)
		{
			const Vector3D parent_pos = inverseTransformPoint(m_boneTransforms[m_joints[i].parent].rows[3].xyz(), posMatrix);
			debugoverlay->Line3D(pos,parent_pos, color_white, color_white);
		}

		const Vector3D dX = posMatrix.getRotationComponent() * m_boneTransforms[i].rows[0].xyz();
		const Vector3D dY = posMatrix.getRotationComponent() * m_boneTransforms[i].rows[1].xyz();
		const Vector3D dZ = posMatrix.getRotationComponent() * m_boneTransforms[i].rows[2].xyz();

		// draw axis
		debugoverlay->Line3D(pos, pos+dX*0.1f, ColorRGBA(1,0,0,1), ColorRGBA(1,0,0,1));
		debugoverlay->Line3D(pos, pos+dY*0.1f, ColorRGBA(0,1,0,1), ColorRGBA(0,1,0,1));
		debugoverlay->Line3D(pos, pos+dZ*0.1f, ColorRGBA(0,0,1,1), ColorRGBA(0,0,1,1));

		debugoverlay->Line3D(pos, pos + dX * 0.1f, ColorRGBA(1, 0, 0, 1), ColorRGBA(1, 0, 0, 1));
		debugoverlay->Text3D(pos, 100.0f, color_white, m_joints[i].bone->name);
	}
}

void CAnimatedModel::AttachIKChain(int chain, int attach_type)
{
	if (chain == -1)
		return;

	int effector_id = m_ikChains[chain].numLinks - 1;
	giklink_t& link = m_ikChains[chain].links[effector_id];

	switch (attach_type)
	{
		case IK_ATTACH_WORLD:
		{
			SetIKWorldTarget(chain, vec3_zero, identity4());
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