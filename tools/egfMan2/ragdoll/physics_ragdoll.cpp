//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2025
//////////////////////////////////////////////////////////////////////////////////
// Description: Physics ragdoll on eqPhysics
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "physics_ragdoll.h"

#include "studio/StudioGeom.h"
#include "physics/eqPhysics_World.h"
#include "physics/eqPhysics_Body.h"
#include "physics/eqPhysics_PointConstraint.h"
#include "physics/eqPhysics_AngularLimitConstraint.h"
#include "physics/PhysicsCollisionGroup.h"

static constexpr const float RAGDOLL_DEFAULT_LINEAR_LIMIT = 0.0025f;
static constexpr const float RAGDOLL_JOINT_TIMESCALE = 1.0f / 20.0f;
static constexpr const float RAGDOLL_DEFAULT_FRICTION = 0.8f;
static constexpr const float RAGDOLL_DEFAULT_RESTITUTION = 0.0f;

// body transform in the same convention as bone matrices
static Matrix4x4 GetBodyMatrix(const CEqRigidBody* body)
{
	const Quaternion& orientation = body->GetOrientation();

	Matrix4x4 bodyMatrix = identity4;
	bodyMatrix.rows[0] = Vector4D(rotateVector(vec3_right, orientation), 0.0f);
	bodyMatrix.rows[1] = Vector4D(rotateVector(vec3_up, orientation), 0.0f);
	bodyMatrix.rows[2] = Vector4D(rotateVector(vec3_forward, orientation), 0.0f);
	bodyMatrix.rows[3] = Vector4D(Vector3D(body->GetPosition()), 1.0f);
	return bodyMatrix;
}

CPhysicsRagdoll::CPhysicsRagdoll(CEqPhysicsWorld& physWorld, CEqStudioGeom* model)
	: m_physWorld(physWorld)
{
	ASSERT(model);

	const StudioPhysData& physModel = model->GetPhysData();
	if (physModel.usageType != PHYSMODEL_USAGE_RAGDOLL)
	{
		ASSERT_FAIL("Invalid physics model usage for %s (excepted PHYSMODEL_USAGE_RAGDOLL)", model->GetName());
		return;
	}

	const studioHdr_t& studio = model->GetStudioHdr();
	m_studioBones = model->GetJoints();

	// physics model has one joint per object
	const int numParts = physModel.objects.numElem();
	ASSERT(physModel.joints.numElem() == numParts);

	// build joint remap table
	for (int partIdx = 0; partIdx < numParts; partIdx++)
	{
		for (int boneIdx = 0; boneIdx < studio.numBones; boneIdx++)
		{
			if (!CString::CompareCaseIns(m_studioBones[boneIdx].bone->name, physModel.joints[partIdx].name))
			{
				m_partToBoneIds.insert(partIdx, boneIdx);
				m_boneToPartIds.insert(boneIdx, partIdx);
				break;
			}
		}
	}

	// build far parental table
	for (int boneIdx = 0; boneIdx < studio.numBones; boneIdx++)
	{
		const int parentIdx = CalcFarParent(boneIdx);
		if (parentIdx != -1)
			m_farParents.insert(boneIdx, parentIdx);
	}

	// create bodies of ragdoll in bind pose
	m_parts.setNum(numParts);
	for (int i = 0; i < numParts; i++)
	{
		const StudioPhyObjData& physObj = physModel.objects[i];
		const int boneIdx = GetBoneIdx(i);
		if (boneIdx == -1)
			continue;

		CEqRigidBody* body = PPNew CEqRigidBody();
		body->Initialize(physObj);

		const eqPhysSurfParam* physSurfParam = m_physWorld.FindSurfaceParam(physObj.desc.surfaceprops);
		body->SetFriction(physSurfParam ? physSurfParam->friction : RAGDOLL_DEFAULT_FRICTION);
		body->SetRestitution(physSurfParam ? physSurfParam->restitution : RAGDOLL_DEFAULT_RESTITUTION);
		body->SetMass(physObj.desc.mass);
		body->SetContents(COLLISION_GROUP_DEBRIS);
		body->SetCollideMask(COLLISION_GROUP_WORLD);
		body->SetUserData(reinterpret_cast<void*>(static_cast<intptr_t>(physObj.desc.bodyPartId)));
		body->SetPosition(physObj.desc.offset);

		Part& part = m_parts[i];
		part.body = body;
		part.boneFrame = m_studioBones[boneIdx].absTrans;
		part.boneFrame.rows[3] -= Vector4D(physObj.desc.offset, 0.0f);
	}

	// create joints, part bodies have identity orientation in bind pose
	for (int i = 0; i < numParts; i++)
	{
		const physjoint_t& joint = physModel.joints[i];

		const int boneIdx = GetBoneIdx(i);
		if (boneIdx == -1 || joint.objA == joint.objB)
			continue;

		CEqRigidBody* bodyA = m_parts[joint.objA].body;
		CEqRigidBody* bodyB = m_parts[joint.objB].body;
		if (!bodyA || !bodyB)
			continue;

		const Matrix4x4& boneAbsTrs = m_studioBones[boneIdx].absTrans;
		const Quaternion jointFrame(boneAbsTrs.getRotationComponent());

		const Vector3D linkPosA = boneAbsTrs.rows[3].xyz() - physModel.objects[joint.objA].desc.offset;
		const Vector3D linkPosB = boneAbsTrs.rows[3].xyz() - physModel.objects[joint.objB].desc.offset;

		Part& part = m_parts[i];
		part.pointConstraint = PPNew CEqPointConstraint();
		part.pointConstraint->Init(bodyA, linkPosA, bodyB, linkPosB, RAGDOLL_DEFAULT_LINEAR_LIMIT, RAGDOLL_JOINT_TIMESCALE);

		part.angularConstraint = PPNew CEqAngularLimitConstraint();
		part.angularConstraint->Init(bodyA, jointFrame, bodyB, jointFrame, joint.minLimit, joint.maxLimit, RAGDOLL_JOINT_TIMESCALE);
	}
}

CPhysicsRagdoll::~CPhysicsRagdoll()
{
	RemoveFromWorld();

	// constraints are removed from bodies first
	for (Part& part : m_parts)
	{
		delete part.pointConstraint;
		delete part.angularConstraint;
	}

	for (Part& part : m_parts)
		delete part.body;
}

void CPhysicsRagdoll::AddToWorld()
{
	if (m_inWorld)
		return;

	m_inWorld = true;
	for (Part& part : m_parts)
	{
		if (part.body)
			m_physWorld.AddBody(part.body);
	}

	for (Part& part : m_parts)
	{
		if (part.pointConstraint)
		{
			part.pointConstraint->SetEnabled(true);
			m_physWorld.AddConstraint(part.pointConstraint);
		}

		if (part.angularConstraint)
		{
			part.angularConstraint->SetEnabled(true);
			m_physWorld.AddConstraint(part.angularConstraint);
		}
	}
}

void CPhysicsRagdoll::RemoveFromWorld()
{
	if (!m_inWorld)
		return;

	m_inWorld = false;
	for (Part& part : m_parts)
	{
		if (part.pointConstraint)
		{
			part.pointConstraint->SetEnabled(false);
			m_physWorld.RemoveConstraint(part.pointConstraint);
		}

		if (part.angularConstraint)
		{
			part.angularConstraint->SetEnabled(false);
			m_physWorld.RemoveConstraint(part.angularConstraint);
		}
	}

	for (Part& part : m_parts)
	{
		if (part.body)
			m_physWorld.RemoveBody(part.body);
	}
}

// finds far parent bone in ragdoll
int CPhysicsRagdoll::CalcFarParent(int bone)
{
	const int parentBone = m_studioBones[bone].parent;
	if (parentBone == -1)
		return -1;

	if (GetPartIdx(parentBone) == -1)
	{
		// continue hierarchy
		return CalcFarParent(parentBone);
	}

	// this is a needed parent with a ragdoll part.
	return parentBone;
}

BoundingBox CPhysicsRagdoll::GetBoundingBox() const
{
	BoundingBox bbox;
	for (const Part& part : m_parts)
	{
		if (!part.body)
			continue;

		const BoundingBox& partBox = part.body->GetWorldAABB();
		bbox.AddVertex(partBox.minPoint);
		bbox.AddVertex(partBox.maxPoint);
	}
	return bbox;
}

Vector3D CPhysicsRagdoll::GetPosition() const
{
	return GetBoundingBox().GetCenter();
}

void CPhysicsRagdoll::GetVisualBonesTransforms(Matrix4x4* bones) const
{
	Matrix4x4 offsetTranslate = identity4;
	offsetTranslate.setTranslation(-GetPosition());

	for (int boneIdx = 0; boneIdx < m_studioBones.numElem(); boneIdx++)
	{
		const int partIdx = GetPartIdx(boneIdx);
		if (partIdx != -1 && m_parts[partIdx].body)
		{
			const Part& part = m_parts[partIdx];
			bones[boneIdx] = part.boneFrame * GetBodyMatrix(part.body) * offsetTranslate;
		}
		else
		{
			const auto it = m_farParents.find(boneIdx);
			if (!it.atEnd())
				bones[boneIdx] = (m_studioBones[boneIdx].absTrans * m_studioBones[*it].invAbsTrans) * bones[*it];
		}
	}
}

// sets bone tranformations (useful for animated death, etc)
// you can setup from here a global transform
void CPhysicsRagdoll::SetBoneTransform(const Matrix4x4* bones, const Matrix4x4& translation)
{
	for (int i = 0; i < m_parts.numElem(); i++)
	{
		const Part& part = m_parts[i];
		if (!part.body)
			continue;

		const int boneIdx = GetBoneIdx(i);
		const Matrix4x4 bodyMatrix = !part.boneFrame * bones[boneIdx] * translation;

		Quaternion orientation(bodyMatrix.getRotationComponent());
		orientation.normalize();

		part.body->SetTransform(Transform3D(bodyMatrix.rows[3].xyz(), orientation));
	}
}

void CPhysicsRagdoll::Translate(const Vector3D& move)
{
	ForEachBodyPart([&](CEqRigidBody* body) {
		body->SetPosition(body->GetPosition() + move);
	});
}

void CPhysicsRagdoll::Wake()
{
	ForEachBodyPart([](CEqRigidBody* body) {
		body->Wake();
	});
}

void CPhysicsRagdoll::Freeze()
{
	ForEachBodyPart([](CEqRigidBody* body) {
		body->Freeze();
	});
}

void CPhysicsRagdoll::ResetVelocities()
{
	ForEachBodyPart([](CEqRigidBody* body) {
		body->SetLinearVelocity(vec3_zero);
		body->SetAngularVelocity(vec3_zero);
	});
}

const Matrix4x4& CPhysicsRagdoll::GetPartBoneFrame(int partIdx) const
{
	return m_parts[partIdx].boneFrame;
}

int CPhysicsRagdoll::GetBoneIdx(int partIdx) const
{
	const auto it = m_partToBoneIds.find(partIdx);
	return it.atEnd() ? -1 : *it;
}

int CPhysicsRagdoll::GetPartIdx(int boneIdx) const
{
	const auto it = m_boneToPartIds.find(boneIdx);
	return it.atEnd() ? -1 : *it;
}

CEqRigidBody* CPhysicsRagdoll::GetPartBody(int partIdx) const
{
	return m_parts[partIdx].body;
}