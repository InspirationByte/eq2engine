//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Animated model for EGFMan - supports physics
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "CAnimatedModel.h"

#include "studio/StudioGeom.h"
#include "animating/anim_events.h"

#include "physics/eqPhysics_World.h"
#include "physics/eqPhysics_Body.h"
#include "physics/PhysicsCollisionGroup.h"
#include "ragdoll/physics_ragdoll.h"
#include "egfman2_physics.h"

#include "materialsystem1/IMaterialSystem.h"
#include "materialsystem1/MeshBuilder.h"

#include "render/IDebugOverlay.h"
#include "render/StudioRenderDefs.h"

CAnimatedModel::CAnimatedModel()
{
	m_model = nullptr;
	m_ragdoll = nullptr;
	m_physObj = nullptr;
	m_physicsEnable = false;

	m_bodyGroupFlags = 0xFFFFFFF;
}

// sets model for this entity
void CAnimatedModel::SetModel(CEqStudioGeom* pModel)
{
	DestroyPhysics();
	DestroyAnimating();

	m_model = pModel;
	if(!m_model)
		return;

	EGF_LOADING_CRITICAL_SECTION(pModel);
	m_model->QueueMaterialsLoading();

	InitAnimating(m_model);

	// the transforms are edited on a copy, the loaded EGF stays as it is
	m_editedTransforms.clear();
	m_editedTransforms.append(m_transforms.ptr(), m_transforms.numElem());
	m_transforms = m_editedTransforms;

	// populate sequences list
	m_sequencesList.clear();
	for (AnimDataProvider& animData : m_animData)
	{
		for (AnimSequence& animSeq : animData.sequences)
			m_sequencesList.append(&animSeq);
	}

	const StudioPhysData& physData = m_model->GetPhysData();
	if(physData.usageType == PHYSMODEL_USAGE_RAGDOLL)
	{
		m_ragdoll = PPNew CPhysicsRagdoll(PhysicsGetWorld(), m_model);
	}
	else if(physData.objects.numElem())
	{
		const StudioPhyObjData& physObj = physData.objects[0];
		const eqPhysSurfParam* physSurfParam = PhysicsGetWorld().FindSurfaceParam(physObj.desc.surfaceprops);

		m_physObj = PPNew CEqRigidBody();
		m_physObj->Initialize(physObj);
		m_physObj->SetMass(physObj.desc.mass);
		m_physObj->SetFriction(physSurfParam ? physSurfParam->friction : 0.8f);
		m_physObj->SetRestitution(physSurfParam ? physSurfParam->restitution : 0.1f);
		m_physObj->SetContents(COLLISION_GROUP_OBJECTS);
		m_physObj->SetCollideMask(COLLISION_GROUP_WORLD);
	}
}

void CAnimatedModel::DestroyPhysics()
{
	if(m_physObj && m_physicsEnable)
		PhysicsGetWorld().RemoveBody(m_physObj);

	m_physicsEnable = false;

	SAFE_DELETE(m_physObj);
	SAFE_DELETE(m_ragdoll);
}

void CAnimatedModel::TogglePhysicsState()
{
	if (!m_ragdoll && !m_physObj)
		return;

	m_physicsEnable = !m_physicsEnable;
	if (m_physicsEnable)
	{
		if (m_ragdoll)
			m_ragdoll->AddToWorld();
		else if (m_physObj)
			PhysicsGetWorld().AddBody(m_physObj);
	}
	else
	{
		if (m_ragdoll)
			m_ragdoll->RemoveFromWorld();
		else if (m_physObj)
			PhysicsGetWorld().RemoveBody(m_physObj);
	}
	ResetPhysics();
}

void CAnimatedModel::ResetPhysics()
{
	if(m_ragdoll)
	{
		RecalcBoneTransforms();
		UpdateIK(0.0f, identity4);

		m_ragdoll->ResetVelocities();
		m_ragdoll->SetBoneTransform(m_boneTransforms, identity4);

		const BoundingBox bbox = m_ragdoll->GetBoundingBox();
		m_ragdoll->Translate(Vector3D(0, -bbox.minPoint.y, 0));
		m_ragdoll->Wake();
	}
	else if(m_physObj)
	{
		const BoundingBox bbox = m_model->GetBoundingBox();

		m_physObj->SetTransform(Transform3D(Vector3D(0, -bbox.minPoint.y, 0), qidentity));
		m_physObj->SetLinearVelocity(vec3_zero);
		m_physObj->SetAngularVelocity(vec3_zero);

		m_physObj->Wake();
	}
}

void CAnimatedModel::Update(float dt)
{
	if(!m_model)
		return;

	// advance frame of animations
	AdvanceFrame(dt);

	// update inverse kinematics
	UpdateIK(dt, identity4);
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
	if(m_ragdoll)
	{
		if(!m_physicsEnable)
		{
			m_ragdoll->SetBoneTransform( m_boneTransforms, identity4 );
		}
		else
		{
			m_ragdoll->GetVisualBonesTransforms(m_boneTransforms);
		}
	}
}

void CAnimatedModel::RenderPhysModel(IGPURenderPassRecorder* rendPassRecorder)
{
	if(!m_model)
		return;

	const StudioPhysData& physData = m_model->GetPhysData();

	if(physData.objects.isEmpty())
		return;

	if(physData.shapes.isEmpty())
		return;

	CMeshBuilder meshBuilder(g_matSystem->GetDynamicMesh());

	RenderDrawCmd drawCmd;
	drawCmd.SetMaterial(g_matSystem->GetDefaultMaterial());

	MatSysDefaultRenderPass defaultRenderPass;
	defaultRenderPass.blendMode = SHADER_BLEND_TRANSLUCENT;
	defaultRenderPass.drawColor = MColor(1.0f, 0.0f, 1.0f, 0.5f);
	defaultRenderPass.depthTest = true;

	Matrix4x4 worldPosMatrix;
	g_matSystem->GetMatrix(MATRIXMODE_WORLD, worldPosMatrix);

	for (int i = 0; i < physData.objects.numElem(); ++i)
	{
		if (m_boneTransforms != nullptr && m_ragdoll)
		{
			Transform3D trs = m_ragdoll->GetPartBody(i)->GetTransform();
			g_matSystem->SetMatrix(MATRIXMODE_WORLD, trs.toMatrix());
		}
		defaultRenderPass.drawColor.setHSV(float(i) / float(physData.objects.numElem()), 1.0f, 1.0f);
		if (m_selectedPhysObject != -1 && m_selectedPhysObject != i)
			defaultRenderPass.drawColor.a = 0.15f;

		const StudioPhyObjData& physObj = physData.objects[i];
		for(int j = 0; j < physObj.desc.numShapes; j++)
		{
			const int nShape = physObj.desc.shapeIndex[j];
			const StudioPhyShapeData& physShape = physData.shapes[nShape];

			const int startIndex = physShape.desc.startIndices;
			const int moveToIndex = startIndex + physShape.desc.numIndices;


			meshBuilder.Begin(PRIM_TRIANGLES);
			for(int k = startIndex; k < moveToIndex; k++)
			{
				meshBuilder.Color4f(1,0,1,1);
				meshBuilder.Position3fv(physData.vertices[physData.indices[k]]);// + physData.objects[i].object.offset);

				meshBuilder.AdvanceVertex();
			}
			if (meshBuilder.End(drawCmd))
			{
				RenderPassContext defaultPassContext(rendPassRecorder, &defaultRenderPass);
				g_matSystem->SetupDrawCommand(drawCmd, defaultPassContext);
			}
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

	return m_sequenceTimers[0].seq->animations[0]->numFrames;
}

// renders model
void CAnimatedModel::Render(int nViewRenderFlags, float fDist, int startLod, bool overrideLod, float dt, IGPURenderPassRecorder* rendPassRecorder)
{
	if(!m_model)
		return;

	if (m_ragdoll && m_physicsEnable)
	{
		UpdateRagdollBones();
	}
	else
	{
		RecalcBoneTransforms();
		UpdateIK(dt, identity4);
	}

	g_matSystem->SetMatrix(MATRIXMODE_WORLD, GetWorldMatrix());

	const studioHdr_t& studio = m_model->GetStudioHdr();

	/*
	Vector3D view_vec = g_pViewEntity->GetEyeOrigin() - m_matWorldTransform.getTranslationComponent();

	// select the LOD
	int nLOD = m_model->SelectLod( length(view_vec) );

	// add water reflection lods
	if(nViewRenderFlags & VR_FLAG_WATERREFLECTION)
		nLOD += r_waterModelLod.GetInt();

	*/

	int startLOD = m_model->SelectLod( fDist );

	if(!overrideLod)
		startLOD += startLod;
	else
		startLOD = startLod;

	startLOD = max(startLOD, 0);
	m_drawnLod = startLOD;

	RenderBoneTransform boneTransforms[128];
	const int numBones = m_model->ConvertBoneMatricesToQuaternions(m_boneTransforms, boneTransforms);
	
	MeshInstanceData instData;
	instData.count = 1;

	CEqStudioGeom::DrawProps drawProperties;
	if(numBones)
		drawProperties.boneTransforms = g_matSystem->GetTransientUniformBuffer(boneTransforms, numBones * sizeof(RenderBoneTransform));
	drawProperties.lod = startLOD;
	drawProperties.bodyGroupFlags = m_bodyGroupFlags;
	drawProperties.skinIdx = clamp(m_skinIdx, 0, max(0, m_model->GetSkinCount() - 1));
	m_model->Draw(drawProperties, instData, RenderPassContext(rendPassRecorder, nullptr));

	if(nViewRenderFlags & RFLAG_PHYSICS)
		RenderPhysModel(rendPassRecorder);

	// the selected bone and attachment are drawn also when the others are not
	VisualizeBones((nViewRenderFlags & RFLAG_BONES) != 0);
	VisualizeAttachments((nViewRenderFlags & RFLAG_ATTACHMENTS) != 0);
}

Matrix4x4 CAnimatedModel::GetTransformParentMatrix(int transformIdx) const
{
	const int boneIdx = m_transforms[transformIdx].attachBoneIdx;
	return boneIdx != EGF_INVALID_IDX ? m_boneTransforms[boneIdx] : identity4;
}

void CAnimatedModel::SetModelSpaceTransform(int transformIdx, const Matrix4x4& transform)
{
	m_editedTransforms[transformIdx].transform = transform * !GetTransformParentMatrix(transformIdx);
}

bool CAnimatedModel::IsTransformEdited(int transformIdx) const
{
	return memcmp(&m_editedTransforms[transformIdx].transform, &m_model->GetStudioHdr().pTransform(transformIdx)->transform, sizeof(Matrix4x4)) != 0;
}

void CAnimatedModel::ResetTransform(int transformIdx)
{
	m_editedTransforms[transformIdx].transform = m_model->GetStudioHdr().pTransform(transformIdx)->transform;
}

Matrix4x4 CAnimatedModel::GetWorldMatrix() const
{
	Matrix4x4 posMatrix = identity4;
	if (m_physicsEnable)
	{
		if (m_ragdoll)
			posMatrix.translate(m_ragdoll->GetPosition());
		else if (m_physObj)
			posMatrix = m_physObj->GetTransform().toMatrix();
	}
	return posMatrix;
}

// a wire octahedral bone from head to tail, widest at a tenth of its length; the parts behind the model hatched
static void AnimatedModelDrawBone(const Vector3D& head, const Vector3D& tail, const MColor& color)
{
	const Vector3D dir = tail - head;
	const float boneLength = length(dir);
	if (boneLength < F_EPS)
		return;

	Vector3D right, up;
	VectorVectors(dir / boneLength, right, up);
	const Vector3D center = head + dir * 0.1f;
	const float width = boneLength * 0.1f;

	const Vector3D ring[4] = { center + right * width, center + up * width, center - right * width, center - up * width };
	for (int i = 0; i < 4; ++i)
	{
		DbgLine().Start(head).End(ring[i]).Color(color).Occluded();
		DbgLine().Start(tail).End(ring[i]).Color(color).Occluded();
		DbgLine().Start(ring[i]).End(ring[(i + 1) & 3]).Color(color).Occluded();
	}
}

// the axes of a transform in world space
static void AnimatedModelDrawAxes(const Vector3D& position, const Matrix3x3& rotation, float size)
{
	DbgLine().Start(position).End(position + rotation.rows[0] * size).Color(MColor(1.0f, 0.0f, 0.0f)).Occluded();
	DbgLine().Start(position).End(position + rotation.rows[1] * size).Color(MColor(0.0f, 1.0f, 0.0f)).Occluded();
	DbgLine().Start(position).End(position + rotation.rows[2] * size).Color(MColor(0.0f, 0.0f, 1.0f)).Occluded();
}

void CAnimatedModel::VisualizeBones(bool all)
{
	const Matrix4x4 posMatrix = GetWorldMatrix();
	const Matrix3x3 posRotation = posMatrix.getRotationComponent();
	const auto bonePosition = [&](int boneIdx) {
		return transformPointTransposed(m_boneTransforms[boneIdx].rows[3].xyz(), posMatrix);
	};

	for (int i = 0; i < m_joints.numElem(); i++)
	{
		const bool selected = i == m_selectedBone;
		if (!all && !selected)
			continue;

		const MColor color = selected ? MColor(1.0f, 0.85f, 0.1f) : color_white;
		const Vector3D head = bonePosition(i);
		const StudioJoint& joint = m_joints[i];

		// the tail is a child; a bone without children continues its parent by a third
		float boneLength = 0.1f;
		if (joint.parent != -1)
			boneLength = length(head - bonePosition(joint.parent));

		if (joint.childs.numElem())
		{
			for (const int childIdx : joint.childs)
				AnimatedModelDrawBone(head, bonePosition(childIdx), color);
			boneLength = length(bonePosition(joint.childs[0]) - head);
		}
		else
		{
			const Vector3D dir = joint.parent != -1 ? normalize(head - bonePosition(joint.parent)) : posRotation * m_boneTransforms[i].rows[0].xyz();
			AnimatedModelDrawBone(head, head + dir * boneLength * 0.3f, color);
		}

		const Matrix3x3 boneRotation(posRotation * m_boneTransforms[i].rows[0].xyz(), posRotation * m_boneTransforms[i].rows[1].xyz(), posRotation * m_boneTransforms[i].rows[2].xyz());
		AnimatedModelDrawAxes(head, boneRotation, selected ? max(boneLength * 0.5f, 0.05f) : boneLength * 0.2f);
		DbgText3D().Position(head).Distance(100.0f).Color(color).Text("%s", joint.bone->name);
	}
}

void CAnimatedModel::VisualizeAttachments(bool all)
{
	const Matrix4x4 posMatrix = GetWorldMatrix();
	const Matrix3x3 posRotation = posMatrix.getRotationComponent();

	for (int i = 0; i < m_transforms.numElem(); ++i)
	{
		const bool selected = i == m_selectedAttachment;
		if (!all && !selected)
			continue;

		const Matrix4x4 attachTransform = GetLocalStudioTransformMatrix(i);
		const Vector3D pos = transformPointTransposed(attachTransform.rows[3].xyz(), posMatrix);
		const Matrix3x3 rotation(posRotation * attachTransform.rows[0].xyz(), posRotation * attachTransform.rows[1].xyz(), posRotation * attachTransform.rows[2].xyz());

		AnimatedModelDrawAxes(pos, rotation, selected ? 0.3f : 0.1f);
		DbgText3D().Position(pos).Distance(100.0f).Color(selected ? MColor(1.0f, 0.85f, 0.1f) : color_white).Text("%s", m_transforms[i].name);
	}
}

void CAnimatedModel::AttachIKChain(int chain, int attach_type)
{
	if (chain == -1)
		return;

	AnimIkChain::Link& link = m_ikChains[chain].links.back();
	switch (attach_type)
	{
		case IK_ATTACH_WORLD:
		{
			SetIKWorldTarget(chain, vec3_zero, identity4);
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