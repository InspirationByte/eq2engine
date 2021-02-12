//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Animated model for EGFMan - supports physics
//////////////////////////////////////////////////////////////////////////////////

#include "CAnimatedModel.h"
#include "materialsystem1/MeshBuilder.h"

#include "render/IDebugOverlay.h"
#include "physics/PhysicsCollisionGroup.h"

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
void CAnimatedModel::SetModel(IEqModel* pModel)
{
	m_bPhysicsEnable = false;

	if(m_physObj)
		physics->DestroyPhysicsObject(m_physObj);

	m_physObj = NULL;

	// do cleanup
	DestroyAnimating();
	DestroyRagdoll( m_pRagdoll );
	m_pRagdoll = NULL;

	m_pModel =  pModel;

	if(!m_pModel)
		return;

	// initialize that shit to use it in future
	InitAnimating(m_pModel);

	if(m_pModel->GetHWData()->physModel.modeltype == PHYSMODEL_USAGE_RAGDOLL)
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
		if(m_pModel->GetHWData()->physModel.numObjects)
			m_physObj = physics->CreateObject(&m_pModel->GetHWData()->physModel, 0);
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
		m_physObj->SetPosition(Vector3D(0,m_pModel->GetAABB().maxPoint.y,0));
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
	return Studio_FindAttachmentId(m_pModel->GetHWData()->studio, name);
}

// gets local attachment position
Vector3D CAnimatedModel::GetLocalAttachmentOrigin(int nAttach)
{
	if(nAttach == -1)
		return vec3_zero;

	if(nAttach >= m_pModel->GetHWData()->studio->numAttachments)
		return vec3_zero;

	studioattachment_t* attach = m_pModel->GetHWData()->studio->pAttachment(nAttach);

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

	if(nAttach >= m_pModel->GetHWData()->studio->numAttachments)
		return vec3_zero;

	studioattachment_t* attach = m_pModel->GetHWData()->studio->pAttachment(nAttach);

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

void CAnimatedModel::HandleAnimatingEvent(AnimationEvent nEvent, char* options)
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

	if(!m_pModel->GetHWData())
		return;

	const studioPhysData_t& phys_data = m_pModel->GetHWData()->physModel;

	if(phys_data.numObjects == 0)
		return;

	if(phys_data.numShapes == 0)
		return;

	BlendStateParam_t blending;
	blending.srcFactor = BLENDFACTOR_SRC_ALPHA;
	blending.dstFactor = BLENDFACTOR_ONE_MINUS_SRC_ALPHA;

	materials->SetAmbientColor(ColorRGBA(1,0,1,1));
	materials->SetDepthStates(true,false);
	materials->SetRasterizerStates(CULL_BACK,FILL_WIREFRAME);
	materials->SetBlendingStates(blending);
	g_pShaderAPI->SetTexture(NULL, NULL, 0);

	CMeshBuilder meshBuilder(materials->GetDynamicMesh());

	Matrix4x4 worldPosMatrix;
	materials->GetMatrix(MATRIXMODE_WORLD, worldPosMatrix);

	materials->BindMaterial(materials->GetDefaultMaterial());

	for(int i = 0; i < phys_data.numObjects; i++)
	{
		for(int j = 0; j < phys_data.objects[i].object.numShapes; j++)
		{
			int nShape = phys_data.objects[i].object.shape_indexes[j];
			if(nShape < 0 || nShape > phys_data.numShapes)
			{
				continue;
			}

			int startIndex = phys_data.shapes[nShape].shape_info.startIndices;
			int moveToIndex = startIndex + phys_data.shapes[nShape].shape_info.numIndices;

			if(m_boneTransforms != NULL && m_pRagdoll)
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
				meshBuilder.Position3fv(phys_data.vertices[phys_data.indices[k]]);// + phys_data.objects[i].object.offset);

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

	studiohdr_t* pHdr = m_pModel->GetHWData()->studio;

	/*
	Vector3D view_vec = g_pViewEntity->GetEyeOrigin() - m_matWorldTransform.getTranslationComponent();

	// select the LOD
	int nLOD = m_pModel->SelectLod( length(view_vec) ); // lod distance check

	// add water reflection lods
	if(nViewRenderFlags & VR_FLAG_WATERREFLECTION)
		nLOD += r_waterModelLod.GetInt();

	*/

	materials->SetAmbientColor( color4_white );

	int nStartLOD = m_pModel->SelectLod( fDist ); // lod distance check

	if(!overrideLod)
		nStartLOD += startLod;
	else
		nStartLOD = startLod;

	for(int i = 0; i < pHdr->numBodyGroups; i++)
	{
		// check bodygroups for rendering
		if(!(m_bodyGroupFlags & (1 << i)))
			continue;

		int bodyGroupLOD = nStartLOD;
		int nLodModelIdx = pHdr->pBodyGroups(i)->lodModelIndex;
		studiolodmodel_t* lodModel = pHdr->pLodModel(nLodModelIdx);

		int nModDescId = lodModel->modelsIndexes[ bodyGroupLOD ];

		// get the right LOD model number
		while(nModDescId == -1 && bodyGroupLOD > 0)
		{
			bodyGroupLOD--;
			nModDescId = lodModel->modelsIndexes[ bodyGroupLOD ];
		}

		if(nModDescId == -1)
			continue;
	
		studiomodeldesc_t* modDesc = pHdr->pModelDesc(nModDescId);

		// render model groups that in this body group
		for(int j = 0; j < modDesc->numGroups; j++)
		{
			materials->SetSkinningEnabled(true);

			materials->BindMaterial( m_pModel->GetMaterial( modDesc->pGroup(j)->materialIndex ), 0);

			m_pModel->PrepareForSkinning(m_boneTransforms);
			m_pModel->DrawGroup( nModDescId, j );

			materials->SetSkinningEnabled(false);
		}
	}

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
		Vector3D pos = (posMatrix*Vector4D(m_boneTransforms[i].rows[3].xyz(), 1.0f)).xyz();

		if(m_joints[i].parentbone != -1)
		{
			Vector3D parent_pos = (posMatrix*Vector4D(m_boneTransforms[m_joints[i].parentbone].rows[3].xyz(), 1.0f)).xyz();
			debugoverlay->Line3D(pos,parent_pos, color4_white, color4_white);
		}

		Vector3D dX = posMatrix.getRotationComponent() * m_boneTransforms[i].rows[0].xyz();
		Vector3D dY = posMatrix.getRotationComponent() * m_boneTransforms[i].rows[1].xyz();
		Vector3D dZ = posMatrix.getRotationComponent() * m_boneTransforms[i].rows[2].xyz();

		// draw axis
		debugoverlay->Line3D(pos, pos+dX*0.1f, ColorRGBA(1,0,0,1), ColorRGBA(1,0,0,1));
		debugoverlay->Line3D(pos, pos+dY*0.1f, ColorRGBA(0,1,0,1), ColorRGBA(0,1,0,1));
		debugoverlay->Line3D(pos, pos+dZ*0.1f, ColorRGBA(0,0,1,1), ColorRGBA(0,0,1,1));
	}
}

void CAnimatedModel::AttachIKChain(int chain, int attach_type)
{
	if (chain == -1)
		return;

	int effector_id = m_ikChains[chain]->numLinks - 1;
	giklink_t& link = m_ikChains[chain]->links[effector_id];

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