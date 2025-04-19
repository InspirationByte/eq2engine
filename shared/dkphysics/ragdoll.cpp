//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Ragdoll utilites, etc
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "ragdoll.h"
#include "studio/StudioGeom.h"

#include "render/IDebugOverlay.h"
#include "animating/BoneSetup.h"
#include "physics/PhysicsCollisionGroup.h"
#include "dkphysics/IDkPhysics.h"

static constexpr float RAGDOLL_LINEAR_LIMIT = 0.0025;
static constexpr int COLLIDE_RAGDOLL = (COLLISION_GROUP_WORLD | COLLISION_GROUP_OBJECTS | COLLISION_GROUP_PROJECTILES);

CPhysRagdollData::~CPhysRagdollData()
{
	// destroy all bones and objects
	for (int i = 0; i < m_numBones; i++)
	{
		if (m_physJoints[i])
			physics->DestroyPhysicsJoint(m_physJoints[i]);
	}

	for (int i = 0; i < m_numParts; i++)
	{
		if (m_partObjs[i])
			physics->DestroyPhysicsObject(m_partObjs[i]);
	}
}

CPhysRagdollData::CPhysRagdollData(CEqStudioGeom* pModel)
{
	ASSERT(pModel);

	memset(m_jointToGeomIds, -1, sizeof(m_jointToGeomIds));
	memset(m_geomToJointIds, -1, sizeof(m_geomToJointIds));
	memset(m_farParents, -1, sizeof(m_farParents));

	const StudioPhysData& physModel = pModel->GetPhysData();
	if (physModel.usageType != PHYSMODEL_USAGE_RAGDOLL)
	{
		ASSERT_FAIL("Invalid physics model usage for %s (excepted PHYSMODEL_USAGE_RAGDOLL)", pModel->GetName());
		return;
	}

	const studioHdr_t& studio = pModel->GetStudioHdr();
	m_studioJoints = pModel->GetJoints();

	const int numPhysJoints = physModel.joints.numElem();
	const int numParts = physModel.objects.numElem();

	m_numBones = numPhysJoints;
	m_numParts = numParts;

	// build joint remap table
	for (int i = 0; i < numPhysJoints; i++)
	{
		for (int j = 0; j < studio.numBones; j++)
		{
			if (!CString::CompareCaseIns(m_studioJoints[j].bone->name, physModel.joints[i].name))
			{
				// assign index
				m_jointToGeomIds[i] = j;
				m_geomToJointIds[j] = i;
				continue;
			}
		}
	}

	// build far parental table
	for (int i = 0; i < studio.numBones; i++)
		m_farParents[i] = ComputeAndGetFarParentOf(i);

	// create objects of ragdoll
	for (int i = 0; i < numParts; i++)
	{
		IPhysicsObject* physObj = physics->CreateObject(&physModel, i);
		m_partObjs[i] = physObj;

		physObj->SetContents(COLLISION_GROUP_DEBRIS);
		physObj->SetCollisionMask(COLLIDE_RAGDOLL | COLLISION_GROUP_DEBRIS);

		physObj->SetSleepTheresholds(20, 20);
		physObj->SetDamping(0.01f, 0.05f);
		physObj->SetFriction(4.0);
		physObj->SetActivationState(PS_ACTIVE);

		const int bodyPartId = physModel.objects[i].desc.bodyPartId;

		physObj->SetUserData(reinterpret_cast<void*>(bodyPartId));
	}

	// create joints
	for (int i = 0; i < numPhysJoints; i++)
	{
		// get a bone transformation
		const Matrix4x4& boneAbsTrs = m_studioJoints[m_jointToGeomIds[i]].absTrans;

		const int objPartIdxA = physModel.joints[i].objA;
		const int objPartIdxB = physModel.joints[i].objB;
		const Vector3D linkPosA = boneAbsTrs.rows[3].xyz() - m_partObjs[objPartIdxA]->GetPosition();
		const Vector3D linkPosB = boneAbsTrs.rows[3].xyz() - m_partObjs[objPartIdxB]->GetPosition();

		Matrix4x4 localTrsA = boneAbsTrs;
		Matrix4x4 localTrsB = boneAbsTrs;

		localTrsA.setTranslation(linkPosA);
		localTrsB.setTranslation(linkPosB);

		IPhysicsObject* partA = m_partObjs[objPartIdxA];
		IPhysicsObject* partB = m_partObjs[objPartIdxB];

		// create constraints
		IPhysicsJoint* physJoint = physics->CreateJoint(partA, partB, localTrsA, localTrsB, true);
		m_physJoints[i] = physJoint;

		// set limits
		physJoint->SetAngularLowerLimit(physModel.joints[i].minLimit);
		physJoint->SetAngularUpperLimit(physModel.joints[i].maxLimit);
		physJoint->SetLinearLowerLimit(Vector3D(-RAGDOLL_LINEAR_LIMIT));
		physJoint->SetLinearUpperLimit(Vector3D(RAGDOLL_LINEAR_LIMIT));
	}

}

// finds far parent bone in ragdoll
int CPhysRagdollData::ComputeAndGetFarParentOf(int bone)
{
	const int parentBone = m_studioJoints[bone].parent;
	if (parentBone != -1)
	{
		if (m_geomToJointIds[parentBone] == -1)
		{
			// continue hierarchy
			return ComputeAndGetFarParentOf(parentBone);
		}
		else
		{
			// this is a needed parent with a ragdoll part.
			return parentBone;
		}
	}

	return -1;
}

BoundingBox CPhysRagdollData::GetBoundingBox() const
{
	BoundingBox bbox;
	for (int i = 0; i < m_numParts; i++)
	{
		if (!m_partObjs[i])
			continue;

		Vector3D partAABBMins;
		Vector3D partAABBMaxs;
		m_partObjs[i]->GetBoundingBox(partAABBMins, partAABBMaxs);

		bbox.AddVertex(partAABBMins);
		bbox.AddVertex(partAABBMaxs);
	}
	return bbox;
}

Vector3D CPhysRagdollData::GetPosition() const
{
	return GetBoundingBox().GetCenter();
}

void CPhysRagdollData::GetVisualBonesTransforms(Matrix4x4* bones) const
{
	Matrix4x4 offsetTranslate = identity4;
	offsetTranslate.setTranslation(-GetPosition());

	for (int i = 0; i < m_studioJoints.numElem(); i++)
	{
		const int jointIdx = m_geomToJointIds[i];
		if (jointIdx != -1)
		{
			Matrix4x4 boneGlobalTrs = m_physJoints[jointIdx]->GetGlobalTransformA();
			bones[i] = boneGlobalTrs * offsetTranslate;
		}
		else
		{
			const int parentIdx = m_farParents[i];
			if (parentIdx != -1)
				bones[i] = (m_studioJoints[i].absTrans * m_studioJoints[parentIdx].invAbsTrans) * bones[parentIdx];
		}
	}
}

// sets bone tranformations (useful for animated death, etc)
// you can setup from here a global transform
void CPhysRagdollData::SetBoneTransform(Matrix4x4* bones, const Matrix4x4& translation)
{
	// set part transform
	for (int i = 0; i < m_numParts; i++)
	{
		if (m_partObjs[i])
			m_partObjs[i]->SetTransformFromMatrix((!m_physJoints[i]->GetFrameTransformA() * bones[m_jointToGeomIds[i]]) * translation);
	}

	RefreshRagdollVisuals();
}

void CPhysRagdollData::Translate(const Vector3D& move)
{
	for (int i = 0; i < m_numParts; i++)
	{
		if (m_partObjs[i])
			m_partObjs[i]->SetPosition(m_partObjs[i]->GetPosition() + move);
	}

	RefreshRagdollVisuals();
}

void CPhysRagdollData::RefreshRagdollVisuals()
{
	// refresh joint transform
	for (int i = 0; i < m_numBones; i++)
	{
		if (!m_physJoints[i])
			continue;
		m_physJoints[i]->UpdateTransform();
	}
}

// wakes ragdoll
void CPhysRagdollData::Wake()
{
	// set part transform
	for (int i = 0; i < m_numParts; i++)
	{
		if (m_partObjs[i])
			m_partObjs[i]->WakeUp();
	}

	RefreshRagdollVisuals();
}

void CPhysRagdollData::Freeze()
{
	for (int i = 0; i < m_numParts; i++)
	{
		if (m_partObjs[i])
			m_partObjs[i]->SetActivationState(PS_FROZEN);
	}
}

void CPhysRagdollData::SetActivationState(EPhysicsActivationState state)
{
	for (int i = 0; i < m_numParts; i++)
	{
		if (m_partObjs[i])
			m_partObjs[i]->SetActivationState(state);
	}
}

void CPhysRagdollData::SetContents(int contents)
{
	for (int i = 0; i < m_numParts; i++)
	{
		if (m_partObjs[i])
			m_partObjs[i]->SetContents(contents);
	}
}

void CPhysRagdollData::SetCollisionMask(int mask)
{
	for (int i = 0; i < m_numParts; i++)
	{
		if (m_partObjs[i])
			m_partObjs[i]->SetCollisionMask(mask);
	}
}

void CPhysRagdollData::SetCollisionResponseEnabled(bool enable)
{
	for (int i = 0; i < m_numParts; i++)
	{
		if (m_partObjs[i])
			m_partObjs[i]->SetCollisionResponseEnabled(enable);
	}
}

void CPhysRagdollData::ResetVelocities()
{
	for (int i = 0; i < m_numParts; i++)
	{
		if (m_partObjs[i])
		{
			m_partObjs[i]->SetVelocity(vec3_zero);
			m_partObjs[i]->SetAngularVelocity(vec3_unit, 0.0);
		}
	}
}

const Matrix4x4& CPhysRagdollData::GetJointTransformA(int idx) const
{
	return m_physJoints[idx]->GetFrameTransformA();
}